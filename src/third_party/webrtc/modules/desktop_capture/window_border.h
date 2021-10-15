// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_
#define MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_

#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_geometry.h"

namespace webrtc {

class WindowBorder {
 public:
  WindowBorder() = default;
  virtual ~WindowBorder() = default;

  virtual bool CreateForWindow(DesktopCapturer::SourceId source_id) = 0;
  virtual bool CreateForScreen(const DesktopRect &window_rect) = 0;
  virtual bool IsCreated() = 0;
  virtual void Destroy() = 0;
};

}  // namespace webrtc

#endif  // MODULES_DESKTOP_CAPTURE_WINDOW_BORDER_H_
