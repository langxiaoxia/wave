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
#include "modules/desktop_capture/rgba_color.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread_types.h"
#include "rtc_base/checks.h"

namespace webrtc {

namespace {

class WindowBorderLinux : public WindowBorder {
 public:
  explicit WindowBorderLinux();
  ~WindowBorderLinux() override;

  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num, const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;
  WindowId GetBorderId() override;
  void OnScreenRectChanged(const DesktopRect &screen_rect) override;

 private:
  Display* display() { return x_display_->display(); }
  void allow_input_passthrough();
  void draw();

  rtc::scoped_refptr<SharedXDisplay> x_display_;
  XVisualInfo vinfo_ = { 0 };
  DesktopRect border_rect_;
  Window window_ = 0;
  Window root_ = 0;
  cairo_surface_t* surf_ = nullptr;
  cairo_t *cr_ = nullptr;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderLinux);
};

WindowBorderLinux::WindowBorderLinux() {
}

WindowBorderLinux::~WindowBorderLinux() {
  Destroy();
}

bool WindowBorderLinux::CreateForWindow(DesktopCapturer::SourceId source_id) {
  if (IsCreated()) {
    return false;
  }

  return true;
}

bool WindowBorderLinux::CreateForScreen(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num, const DesktopRect &window_rect) {
  if (IsCreated()) {
    return false;
  }

  if (window_rect.is_empty()) {
    return false;
  }

  x_display_ = x_display;
  root_ = RootWindow(display(), screen_num);
  if (!root_) {
    return false;
  }

  if (!XMatchVisualInfo(display(), screen_num, 32, TrueColor, &vinfo_)) {
    return false;
  }

  XSetWindowAttributes attr = { 0 };
  attr.override_redirect = True;
  attr.colormap = XCreateColormap(display(), root_, vinfo_.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = 0;

  border_rect_ = window_rect;
  window_ = XCreateWindow(display(), root_,
      border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height(), 0,
      vinfo_.depth, InputOutput, vinfo_.visual,
      CWOverrideRedirect | CWColormap | CWBorderPixel | CWBackPixel, &attr);
  if (!window_) {
    Destroy();
    return false;
  }

  allow_input_passthrough();

  surf_ = cairo_xlib_surface_create(display(), window_, vinfo_.visual, border_rect_.width(), border_rect_.height());
  if (surf_ == nullptr) {
    Destroy();
    return false;
  }

  cr_ = cairo_create(surf_);
  if (cr_ == nullptr) {
    Destroy();
    return false;
  }

  RTC_LOG(LS_INFO) << "Create border window OK: window=" << window_ <<
                      ", screen=" << screen_num <<
                      ", root=" << root_ <<
                      ", rect=(" << border_rect_.left() << ", " << border_rect_.top() << ") " << border_rect_.width() << "x" << border_rect_.height();

  XMapWindow(display(), window_);
  draw();
  XFlush(display());
  return true;
}

bool WindowBorderLinux::IsCreated() {
  if (window_) {
    return true;
  } else {
    return false;
  }
}

void WindowBorderLinux::Destroy() {
  if (!IsCreated()) {
    return;
  }

  if (window_) {
    XUnmapWindow(display(), window_);
    XDestroyWindow(display(), window_);
    window_ = 0;
  }

  if (cr_) {
    cairo_destroy(cr_);
    cr_ = nullptr;
  }

  if (surf_) {
    cairo_surface_destroy(surf_);
    surf_ = nullptr;
  }

  XFlush(display());
}

WindowId WindowBorderLinux::GetBorderId() {
  return window_;
}

void WindowBorderLinux::OnScreenRectChanged(const DesktopRect &screen_rect) {
  if (!IsCreated()) {
    return;
  }

  RTC_LOG(LS_INFO) << "OnScreenRectChanged: ("
                   << border_rect_.left() << ", "
                   << border_rect_.top() << ", "
                   << border_rect_.right() << ", "
                   << border_rect_.bottom() << ") => ("
                   << screen_rect.left() << ", "
                   << screen_rect.top() << ", "
                   << screen_rect.right() << ", "
                   << screen_rect.bottom() << ")";

  if (border_rect_.equals(screen_rect)) {
    return;
  }

  if (screen_rect.is_empty()) {
    Destroy();
    return;
  }

  border_rect_ = screen_rect;
  XMoveResizeWindow(display(), window_, border_rect_.left(), border_rect_.top(), border_rect_.width(), border_rect_.height());

  allow_input_passthrough();
  cairo_xlib_surface_set_size(surf_, border_rect_.width(), border_rect_.height());

  draw();
  XFlush(display());
}

void WindowBorderLinux::allow_input_passthrough() {
  XserverRegion region = XFixesCreateRegion(display(), nullptr, 0);
  XFixesSetWindowShapeRegion(display(), window_, ShapeBounding, 0, 0, 0);
  XFixesSetWindowShapeRegion(display(), window_, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display(), region);
}

void WindowBorderLinux::draw() {
  cairo_set_line_width (cr_, kBorderWidth);
  cairo_set_source_rgba(cr_, (double)kBorderColorR / 0xff, (double)kBorderColorG / 0xff, (double)kBorderColorB / 0xff, 1);
  cairo_rectangle(cr_, kBorderWidth / 2, kBorderWidth / 2, border_rect_.width() - kBorderWidth, border_rect_.height() - kBorderWidth);
  cairo_stroke(cr_);
}

}  // namespace

// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderLinux());
}

}  // namespace webrtc
