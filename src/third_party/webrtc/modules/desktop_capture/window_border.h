// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_
#define MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_

#if defined(WEBRTC_USE_X11)
#include "modules/desktop_capture/linux/shared_x_display.h"
#endif // WEBRTC_USE_X11

#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_geometry.h"

namespace webrtc {

class WindowBorder {
 public:
  WindowBorder() = default;
  virtual ~WindowBorder() = default;

#if defined(WEBRTC_USE_X11)
  virtual void Init(rtc::scoped_refptr<SharedXDisplay> x_display, int screen_num) = 0;
#endif // WEBRTC_USE_X11
  virtual bool CreateForWindow(DesktopCapturer::SourceId source_id) = 0;
  virtual bool CreateForScreen(const DesktopRect &window_rect) = 0;
  virtual bool IsCreated() = 0;
  virtual void Destroy() = 0;
  virtual WindowId GetBorderId() = 0;
  virtual void OnScreenRectChanged(const DesktopRect &screen_rect) = 0;

  static const int kBorderWidth = 4;
  static const unsigned char kBorderColorR = 0xEB;
  static const unsigned char kBorderColorG = 0x4C;
  static const unsigned char kBorderColorB = 0x46;
};

}  // namespace webrtc

#endif  // MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_
