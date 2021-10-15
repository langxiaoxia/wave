/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_
#define MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_

#include <d3dcommon.h>

#include <memory>
#include <vector>

#include "api/scoped_refptr.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_region.h"
#include "modules/desktop_capture/screen_capture_frame_queue.h"
#include "modules/desktop_capture/win/dxgi_duplicator_controller.h"
#include "modules/desktop_capture/win/dxgi_frame.h"
#include "modules/desktop_capture/window_border.h" //+by xxlang@2021-10-15
#include "rtc_base/system/rtc_export.h"

namespace webrtc {

// ScreenCapturerWinDirectx captures 32bit RGBA using DirectX.
class RTC_EXPORT ScreenCapturerWinDirectx : public DesktopCapturer {
 public:
  using D3dInfo = DxgiDuplicatorController::D3dInfo;

  // Whether the system supports DirectX based capturing.
  static bool IsSupported();

  // Returns a most recent D3dInfo composed by
  // DxgiDuplicatorController::Initialize() function. This function implicitly
  // calls DxgiDuplicatorController::Initialize() if it has not been
  // initialized. This function returns false and output parameter is kept
  // unchanged if DxgiDuplicatorController::Initialize() failed.
  // The D3dInfo may change based on hardware configuration even without
  // restarting the hardware and software. Refer to https://goo.gl/OOCppq. So
  // consumers should not cache the result returned by this function.
  static bool RetrieveD3dInfo(D3dInfo* info);

  // Whether current process is running in a Windows session which is supported
  // by ScreenCapturerWinDirectx.
  // Usually using ScreenCapturerWinDirectx in unsupported sessions will fail.
  // But this behavior may vary on different Windows version. So consumers can
  // always try IsSupported() function.
  static bool IsCurrentSessionSupported();

  // Maps |device_names| with the result from GetScreenList() and creates a new
  // SourceList to include only the ones in |device_names|. If this function
  // returns true, consumers can always assume |device_names|.size() equals to
  // |screens|->size(), meanwhile |device_names|[i] and |screens|[i] indicate
  // the same monitor on the system.
  // Public for test only.
  static bool GetScreenListFromDeviceNames(
      const std::vector<std::string>& device_names,
      DesktopCapturer::SourceList* screens);

  // Maps |id| with the result from GetScreenListFromDeviceNames() and returns
  // the index of the entity in |device_names|. This function returns -1 if |id|
  // cannot be found.
  // Public for test only.
  static int GetIndexFromScreenId(ScreenId id,
                                  const std::vector<std::string>& device_names);

  explicit ScreenCapturerWinDirectx(bool enable_border);

  ~ScreenCapturerWinDirectx() override;

  // DesktopCapturer implementation.
  void Start(Callback* callback) override;
  void SetSharedMemoryFactory(
      std::unique_ptr<SharedMemoryFactory> shared_memory_factory) override;
  void CaptureFrame() override;
  bool GetSourceList(SourceList* sources) override;
  bool SelectSource(SourceId id) override;

 private:
  const rtc::scoped_refptr<DxgiDuplicatorController> controller_;
  ScreenCaptureFrameQueue<DxgiFrame> frames_;
  std::unique_ptr<SharedMemoryFactory> shared_memory_factory_;
  Callback* callback_ = nullptr;
  SourceId current_screen_id_ = kFullDesktopScreenId;

  //+by xxlang@2021-09-28 {
  bool enable_border_;
  bool first_capture_;
  std::unique_ptr<WindowBorder> window_border_;
//+by xxlang@2021-09-28 }

  RTC_DISALLOW_COPY_AND_ASSIGN(ScreenCapturerWinDirectx);
};

}  // namespace webrtc

#endif  // MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_DIRECTX_H_
