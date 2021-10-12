/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MODULES_DESKTOP_CAPTURE_WIN_SCREEN_BORDER_WINDOW_H_
#define MODULES_DESKTOP_CAPTURE_WIN_SCREEN_BORDER_WINDOW_H_

#include <windows.h>

#include "modules/desktop_capture/win/window_capture_utils.h"
#include "rtc_base/constructor_magic.h"

namespace webrtc {

class ScreenBorderWindow {
 public:
  ScreenBorderWindow() = default;
  ~ScreenBorderWindow();

  bool CreateForWindow(HWND hwnd);
  bool CreateForScreen(DesktopRect window_rect);
  bool IsCreated();
  void Destroy();

 private:
  bool Create(DesktopRect window_rect, HWND hwnd);
  void Hook();
  void Unhook();

  HINSTANCE window_instance_ = nullptr;
  ATOM window_class_ = 0;
  HWND hwnd_border_ = nullptr;
  HWND hwnd_ = nullptr;

  HMODULE hMod_ = nullptr;

  RTC_DISALLOW_COPY_AND_ASSIGN(ScreenBorderWindow);
};

}  // namespace webrtc

#endif  // MODULES_DESKTOP_CAPTURE_WIN_SCREEN_BORDER_WINDOW_H_
