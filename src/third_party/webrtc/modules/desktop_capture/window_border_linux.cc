// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "api/scoped_refptr.h"
#include "modules/desktop_capture/window_border.h"
#include "modules/desktop_capture/linux/shared_x_display.h"
#include "modules/desktop_capture/linux/window_list_utils.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread_types.h"

namespace webrtc {

namespace {

class WindowBorderLinux : public WindowBorder,
                          public SharedXDisplay::XEventHandler {
 public:
  explicit WindowBorderLinux();
  ~WindowBorderLinux() override;

  // WindowBorder interface.
  void Init(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num) override;
  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;
  WindowId GetBorderId() override;
  void OnScreenRectChanged(const DesktopRect &screen_rect) override;

  // SharedXDisplay::XEventHandler interface.
  bool HandleXEvent(const XEvent& event) override;

 private:
  bool Create(const DesktopRect &window_rect, Window source_window);
  void Deinit();
  Display* display() { return x_display_->display(); }
  bool prepare();
  void set_shape_bounding();
  void allow_input_passthrough();
  void draw(bool resize);

  rtc::scoped_refptr<SharedXDisplay> x_display_;
  int screen_num_ = 0;
  Window root_window_ = 0;
  XVisualInfo vinfo_ = { 0 };
  DesktopRect border_rect_;
  Window border_window_ = 0;
  Window source_window_ = 0;
  DesktopRect frame_extents_;
  cairo_surface_t* surface_ = nullptr;
  cairo_t* cairo_ = nullptr;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderLinux);
};

WindowBorderLinux::WindowBorderLinux() {
}

WindowBorderLinux::~WindowBorderLinux() {
  Destroy();
  Deinit();
}

void WindowBorderLinux::Init(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num) {
  if (x_display_) {
    RTC_LOG(LS_WARNING) << "Init(" << this << "): Thread=" << rtc::CurrentThreadId();
    return;
  }

  RTC_LOG(LS_INFO) << "Init(" << this << "): Thread=" << rtc::CurrentThreadId();
  x_display_ = x_display;
  screen_num_ = screen_num;
  root_window_ = RootWindow(display(), screen_num_);

  // ExposureMask
  x_display_->AddEventHandler(Expose, this);

  // StructureNotifyMask
  x_display_->AddEventHandler(MapNotify, this);
  x_display_->AddEventHandler(ConfigureNotify, this);
}

bool WindowBorderLinux::CreateForWindow(DesktopCapturer::SourceId source_id) {
  Window source_window = static_cast<Window>(source_id);
  DesktopRect window_rect;
  if (!GetWindowRect(display(), source_window, &window_rect, nullptr)) {
    RTC_LOG(LS_ERROR) << "Failed to get source window rect.";
    return false;
  }

  GetFrameExtents(display(), source_window, &frame_extents_);

  if (!Create(window_rect, source_window)) {
    return false;
  }

  return true;
}

bool WindowBorderLinux::CreateForScreen(const DesktopRect &window_rect) {
  Window source_window = root_window_;
  if (!Create(window_rect, source_window)) {
    return false;
  }

  return true;
}

bool WindowBorderLinux::IsCreated() {
  if (border_window_) {
    return true;
  } else {
    return false;
  }
}

void WindowBorderLinux::Destroy() {
  if (!IsCreated()) {
    return;
  }

  if (border_window_) {
    RTC_LOG(LS_INFO) << "Destroy(" << this << "): Thread=" << rtc::CurrentThreadId() << ", border_window=" << border_window_;
    XUnmapWindow(display(), border_window_);
    XDestroyWindow(display(), border_window_);
    border_window_ = 0;
  }

  if (cairo_) {
    cairo_destroy(cairo_);
    cairo_ = nullptr;
  }

  if (surface_) {
    cairo_surface_destroy(surface_);
    surface_ = nullptr;
  }

  XFlush(display());
}

void WindowBorderLinux::Deinit() {
  if (x_display_) {
    RTC_LOG(LS_INFO) << "Deinit(" << this << "): Thread=" << rtc::CurrentThreadId();
    // ExposureMask
    x_display_->RemoveEventHandler(Expose, this);

    // StructureNotifyMask
    x_display_->RemoveEventHandler(MapNotify, this);
    x_display_->RemoveEventHandler(ConfigureNotify, this);

    x_display_ = nullptr;
  }
}

WindowId WindowBorderLinux::GetBorderId() {
  return border_window_;
}

void WindowBorderLinux::OnScreenRectChanged(const DesktopRect &screen_rect) {
  if (!IsCreated()) {
    return;
  }

  if (border_rect_.equals(screen_rect)) {
    return;
  }

  RTC_LOG(LS_INFO) << "OnScreenRectChanged: ("
                   << border_rect_.left() << ", "
                   << border_rect_.top() << ") "
                   << border_rect_.width() << "x"
                   << border_rect_.height() << " => ("
                   << screen_rect.left() << ", "
                   << screen_rect.top() << ") "
                   << screen_rect.width() << "x"
                   << screen_rect.height();
  if (screen_rect.is_empty()) {
    Destroy();
    return;
  }

  border_rect_ = screen_rect;
  if (root_window_ == source_window_) {
    XMoveResizeWindow(display(), border_window_, border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height());
  } else {
    GetFrameExtents(display(), source_window_, &frame_extents_);
    XMoveResizeWindow(display(), border_window_, 0, 0, border_rect_.width(), border_rect_.height());
  }
  XFlush(display());
}

bool WindowBorderLinux::HandleXEvent(const XEvent &event) {
  if (!IsCreated()) {
    return false;
  }

  // skip other windows
  if (event.xany.window != border_window_) {
    return false;
  }

  switch (event.type) {
    case MapNotify:
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=MapNotify";
      break;

    case Expose:
    {
      XExposeEvent xe = event.xexpose;
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=Expose"
          << ", x=" << xe.x << ", y=" << xe.y << ", width=" << xe.width << ", height=" << xe.height;
      draw(false);
      break;
    }

    case ConfigureNotify:
    {
      XConfigureEvent xe = event.xconfigure;
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=ConfigureNotify"
          << ", x=" << xe.x << ", y=" << xe.y << ", width=" << xe.width << ", height=" << xe.height;
      draw(true);
      break;
    }

    default:
      RTC_LOG(LS_INFO) << "IgnoreXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=" << event.type;
      break;
  }

  return true;
}

bool WindowBorderLinux::Create(const DesktopRect &window_rect, Window source_window) {
  // check created
  if (IsCreated()) {
    RTC_LOG(LS_ERROR) << "Create failed: already created";
    return false;
  }

  // check parameter
  if (window_rect.is_empty()) {
    RTC_LOG(LS_ERROR) << "Create failed: empty rect";
    return false;
  }

  // save border rect
  border_rect_ = window_rect;

  // save source window
  source_window_ = source_window;

  if (!XMatchVisualInfo(display(), screen_num_, 32, TrueColor, &vinfo_)) {
    RTC_LOG(LS_ERROR) << "Create failed: no matched visual info";
    return false;
  }

  XSetWindowAttributes attr = { 0 };
  attr.override_redirect = True;
  attr.colormap = XCreateColormap(display(), source_window_, vinfo_.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = 0;
  attr.event_mask = ExposureMask | StructureNotifyMask;

  // create border window
  if (source_window_ == root_window_) { // screen capture
    border_window_ = XCreateWindow(display(), source_window_,
        border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height(), 0,
        vinfo_.depth, InputOutput, vinfo_.visual,
        CWOverrideRedirect | CWColormap | CWBorderPixel | CWBackPixel | CWEventMask, &attr);
  } else { // window capture
    border_window_ = XCreateWindow(display(), source_window_,
        0, 0, border_rect_.width(), border_rect_.height(), 0, // (0, 0)
        vinfo_.depth, InputOutput, vinfo_.visual,
        CWOverrideRedirect | CWColormap | CWBorderPixel | CWEventMask, &attr); // without CWBackPixel to make border window transparent
  }

  if (!border_window_) {
    RTC_LOG(LS_ERROR) << "Create failed: create window failed";
    return false;
  }

  if (!prepare()) {
    Destroy();
    return false;
  }

  XStoreName(display(), border_window_, "wave_linux_border_window");
  XMapWindow(display(), border_window_);
  XFlush(display());

  RTC_LOG(LS_INFO) << "Create(" << this << "): Thread=" << rtc::CurrentThreadId()
                   << ", screen_num=" << screen_num_
                   << ", root_window=" << root_window_
                   << ", source_window=" << source_window_
                   << ", border_window=" << border_window_
                   << ", rect(" << border_rect_.left() << ", " << border_rect_.top()
                   << ") " << border_rect_.width() << "x" << border_rect_.height();
  return true;
}

bool WindowBorderLinux::prepare() {
  if (!IsCreated()) {
    return false;
  }

  if (cairo_) {
    cairo_destroy(cairo_);
    cairo_ = nullptr;
  }

  if (surface_) {
    cairo_surface_destroy(surface_);
    surface_ = nullptr;
  }

  RTC_LOG(LS_INFO) << "prepare surface: depth=" << vinfo_.depth << ", width=" << border_rect_.width() << ", height=" << border_rect_.height();
  surface_ = cairo_xlib_surface_create(display(), border_window_, vinfo_.visual, border_rect_.width(), border_rect_.height());
  if (surface_ == nullptr) {
    RTC_LOG(LS_ERROR) << "cairo_xlib_surface_create failed";
    return false;
  }

  cairo_ = cairo_create(surface_);
  if (cairo_ == nullptr) {
    RTC_LOG(LS_ERROR) << "cairo_create failed";
    return false;
  }

  set_shape_bounding();
  allow_input_passthrough();

  return true;
}

void WindowBorderLinux::allow_input_passthrough() {
  if (!IsCreated()) {
    return;
  }

  RTC_LOG(LS_INFO) << "allow_input_passthrough";
  XserverRegion region = XFixesCreateRegion(display(), nullptr, 0);
  XFixesSetWindowShapeRegion(display(), border_window_, ShapeInput, 0, 0, region); // clear to empty
  XFixesDestroyRegion(display(), region);

  XFlush(display());
}

void WindowBorderLinux::set_shape_bounding() {
  if (!IsCreated()) {
    return;
  }

  if (source_window_ == root_window_) {
    XFixesSetWindowShapeRegion(display(), border_window_, ShapeBounding, 0, 0, 0); // default is whole
  } else {
    RTC_LOG(LS_INFO) << "set_shape_bounding: left=" << frame_extents_.left() << ", top=" << frame_extents_.top() << ", width=" << border_rect_.width() << ", height=" << border_rect_.height();
    XRectangle xrect_out;
    xrect_out.x = frame_extents_.left();
    xrect_out.y = frame_extents_.top();
    xrect_out.width = border_rect_.width() - frame_extents_.left() - frame_extents_.right();
    xrect_out.height = border_rect_.height() - frame_extents_.top() - frame_extents_.bottom();
    XserverRegion region_out = XFixesCreateRegion(display(), &xrect_out, 1);

    XRectangle xrect_in;
    xrect_in.x = frame_extents_.left() + kBorderWidth;
    xrect_in.y = frame_extents_.top() + kBorderWidth;
    xrect_in.width = border_rect_.width() - frame_extents_.left() - frame_extents_.right() - kBorderWidth * 2;
    xrect_in.height = border_rect_.height() - frame_extents_.top() - frame_extents_.bottom() - kBorderWidth * 2;
    XserverRegion region_in = XFixesCreateRegion(display(), &xrect_in, 1);

    XserverRegion region_border = XFixesCreateRegion (display(), nullptr, 0);
    XFixesSubtractRegion(display(), region_border, region_out, region_in);
    XFixesSetWindowShapeRegion(display(), border_window_, ShapeBounding, 0, 0, region_border);

    XFixesDestroyRegion(display(), region_border);
    XFixesDestroyRegion(display(), region_out);
    XFixesDestroyRegion(display(), region_in);
  }

  XFlush(display());
}

void WindowBorderLinux::draw(bool resize) {
  if (!IsCreated() || cairo_ == nullptr) {
    return;
  }

  if (resize) {
    RTC_LOG(LS_INFO) << "resize surface: width=" << border_rect_.width() << ", height=" << border_rect_.height();
    cairo_xlib_surface_set_size(surface_, border_rect_.width(), border_rect_.height());
    set_shape_bounding();
  }

  RTC_LOG(LS_INFO) << "draw border: left=" << frame_extents_.left() << ", top=" << frame_extents_.top() << ", width=" << border_rect_.width() << ", height=" << border_rect_.height();
  cairo_set_operator(cairo_, CAIRO_OPERATOR_OVER); // default is over
  cairo_set_line_width(cairo_, kBorderWidth);
  cairo_set_source_rgba(cairo_, (double)kBorderColorR / 0xff, (double)kBorderColorG / 0xff, (double)kBorderColorB / 0xff, 1.0); // border is opaque
  cairo_rectangle(cairo_,
    frame_extents_.left() + kBorderWidth / 2.0,
    frame_extents_.top() + kBorderWidth / 2.0,
    border_rect_.width() - frame_extents_.left() - frame_extents_.right() - kBorderWidth,
    border_rect_.height() - frame_extents_.top() - frame_extents_.bottom() - kBorderWidth);
  cairo_stroke(cairo_);
}

}  // namespace

// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderLinux());
}

}  // namespace webrtc
