/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/desktop_capture/linux/x_error_trap.h"

#include <stddef.h>

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread_types.h"

namespace webrtc {

namespace {

#if !defined(TOOLKIT_GTK)

// TODO(sergeyu): This code is not thread safe. Fix it. Bug 2202.
static bool g_xserver_error_trap_enabled = false;
static int g_last_xserver_error_code = 0;

int XServerErrorHandler(Display* display, XErrorEvent* error_event) {
//  RTC_DCHECK(g_xserver_error_trap_enabled);
  if (g_xserver_error_trap_enabled) {
    g_last_xserver_error_code = error_event->error_code;
  } else {
    RTC_LOG(LS_ERROR) << "XServerErrorHandler(" << display << "): Thread=" << rtc::CurrentThreadId()
        << ", resourceid=" << error_event->resourceid
        << ", request_code=" << int(error_event->request_code)
        << ", error_code=" << int(error_event->error_code);
  }
  return 0;
}

#endif  // !defined(TOOLKIT_GTK)

}  // namespace

XErrorTrap::XErrorTrap(Display* display)
    : original_error_handler_(NULL), enabled_(true) {
//  RTC_LOG(LS_INFO) << "XErrorTrap(" << display << "): Thread=" << rtc::CurrentThreadId();
  RTC_DCHECK(!g_xserver_error_trap_enabled);
  original_error_handler_ = XSetErrorHandler(&XServerErrorHandler);
  g_xserver_error_trap_enabled = true;
  g_last_xserver_error_code = 0;
}

int XErrorTrap::GetLastErrorAndDisable() {
  enabled_ = false;
//  RTC_LOG(LS_INFO) << "GetLastErrorAndDisable(): Thread=" << rtc::CurrentThreadId();
  RTC_DCHECK(g_xserver_error_trap_enabled);
  XSetErrorHandler(original_error_handler_);
  g_xserver_error_trap_enabled = false;
  return g_last_xserver_error_code;
}

XErrorTrap::~XErrorTrap() {
  if (enabled_)
    GetLastErrorAndDisable();
}

}  // namespace webrtc
