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
  void Init(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num, bool handle_event) override;
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
  void allow_input_passthrough();
  void draw();

  rtc::scoped_refptr<SharedXDisplay> x_display_;
  int screen_num_ = 0;
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

void WindowBorderLinux::Init(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num, bool handle_event) {
  if (x_display_) {
    RTC_LOG(LS_WARNING) << "Init(" << this << "): Thread=" << rtc::CurrentThreadId();
    return;
  }

  RTC_LOG(LS_INFO) << "Init(" << this << "): Thread=" << rtc::CurrentThreadId();
  x_display_ = x_display;
  screen_num_ = screen_num;

  if (handle_event) {
    x_display_->AddEventHandler(MapNotify, this);
    x_display_->AddEventHandler(ConfigureNotify, this);
    x_display_->AddEventHandler(Expose, this);
  }
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
  Window source_window = RootWindow(display(), screen_num_);
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
    if (border_window_ == source_window_) {
      XClearArea(display(), border_window_, 0, 0, border_rect_.width(), border_rect_.height(), True);
    } else {
      XUnmapWindow(display(), border_window_);
      XDestroyWindow(display(), border_window_);
    }
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
    x_display_->RemoveEventHandler(MapNotify, this);
    x_display_->RemoveEventHandler(ConfigureNotify, this);
    x_display_->RemoveEventHandler(Expose, this);
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

  if (!border_rect_.equals(screen_rect)) {
    RTC_LOG(LS_INFO) << "OnScreenRectChanged: ("
                     << border_rect_.left() << ", "
                     << border_rect_.top() << ", "
                     << border_rect_.right() << ", "
                     << border_rect_.bottom() << ") => ("
                     << screen_rect.left() << ", "
                     << screen_rect.top() << ", "
                     << screen_rect.right() << ", "
                     << screen_rect.bottom() << ")";
    if (screen_rect.is_empty()) {
      Destroy();
      return;
    }

    border_rect_ = screen_rect;
    if (border_window_ == source_window_) {
      GetFrameExtents(display(), source_window_, &frame_extents_);
      if (prepare()) {
        draw();
      }
    } else {
      RTC_LOG(LS_INFO) << "XMoveResizeWindow";
      XMoveResizeWindow(display(), border_window_, border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height());
      XFlush(display());
    }
  } else {
    if (border_window_ == source_window_) {
      GetFrameExtents(display(), source_window_, &frame_extents_);
      draw();
    }
  }
}

bool WindowBorderLinux::HandleXEvent(const XEvent &event) {
  if (!IsCreated()) {
    return false;
  }

  if (event.xany.window != border_window_) {
    return false;
  }

  switch (event.type) {
    case MapNotify:
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=MapNotify";
      if (prepare()) {
        draw();
      } else {
        Destroy();
      }
      break;

    case Expose:
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=Expose";
      draw();
      break;

    case ConfigureNotify:
    {
      XConfigureEvent xe = event.xconfigure;
      RTC_LOG(LS_INFO) << "HandleXEvent(" << this << "): Thread=" << rtc::CurrentThreadId() << ", type=ConfigureNotify"
          << ", rect(" << xe.x << ", " << xe.y << ") " << xe.width << "x" << xe.height;
      if (prepare()) {
        draw();
      } else {
        Destroy();
      }
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
    RTC_LOG(LS_ERROR) << "Create failed: match visual info failed";
    return false;
  }

  Window root_window = RootWindow(display(), screen_num_);
  if (source_window == root_window) { // screen capture
    // create border window
    XSetWindowAttributes attr = { 0 };
    attr.override_redirect = True;
    attr.colormap = XCreateColormap(display(), root_window, vinfo_.visual, AllocNone);
    attr.border_pixel = 0;
    attr.background_pixel = 0;
    border_window_ = XCreateWindow(display(), root_window,
        border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height(), 0,
        vinfo_.depth, InputOutput, vinfo_.visual,
        CWOverrideRedirect | CWColormap | CWBorderPixel | CWBackPixel, &attr);
    if (!border_window_) {
      RTC_LOG(LS_ERROR) << "Create failed: create window failed";
      return false;
    }

    XSelectInput(display(), border_window_, StructureNotifyMask | ExposureMask);
    XMapWindow(display(), border_window_);
    XFlush(display());
  } else { // window capture
    border_window_ = source_window_;
    if (prepare()) {
      draw();
    } else {
      Destroy();
      return false;
    }
  }

  RTC_LOG(LS_INFO) << "Create(" << this << "): Thread=" << rtc::CurrentThreadId()
                   << ", screen_num=" << screen_num_
                   << ", root_window=" << root_window
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

  if (border_window_ != source_window_) {
    allow_input_passthrough();
  }
  return true;
}

void WindowBorderLinux::allow_input_passthrough() {
  if (!IsCreated()) {
    return;
  }

  RTC_LOG(LS_INFO) << "allow_input_passthrough";
  XserverRegion region = XFixesCreateRegion(display(), nullptr, 0);
  XFixesSetWindowShapeRegion(display(), border_window_, ShapeBounding, 0, 0, 0);
  XFixesSetWindowShapeRegion(display(), border_window_, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display(), region);

  XFlush(display());
}

void WindowBorderLinux::draw() {
  if (!IsCreated() || cairo_ == nullptr) {
    return;
  }

  RTC_LOG(LS_INFO) << "draw";
  cairo_set_line_width(cairo_, kBorderWidth);
  cairo_set_source_rgba(cairo_, (double)kBorderColorR / 0xff, (double)kBorderColorG / 0xff, (double)kBorderColorB / 0xff, 1.0);
  cairo_rectangle(cairo_,
      frame_extents_.left() + kBorderWidth / 2.0,
      frame_extents_.top() + kBorderWidth / 2.0,
      border_rect_.width() - frame_extents_.left() - frame_extents_.right() - kBorderWidth,
      border_rect_.height() - frame_extents_.top() - frame_extents_.bottom() - kBorderWidth);
  cairo_stroke(cairo_);

  XFlush(display());
}

}  // namespace

// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderLinux());
}

}  // namespace webrtc
