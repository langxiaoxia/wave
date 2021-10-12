/*
 *  Copyright (c) 2020 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "modules/desktop_capture/win/screen_border_window.h"

#include "rtc_base/logging.h"
#include "rtc_base/win32.h"

namespace webrtc {

namespace {

const WCHAR kWindowClass[] = L"ScreenBorderWindowClass";
const COLORREF kHighlightColor = RGB(255, 0, 0);
const COLORREF kTransparentColor = RGB(0, 0, 0);
const int kBorderWidth = 3;
const HWND kScreenWindow = HWND_TOP;

HWND kHwnd = nullptr;

bool Is64BitOS() {
  typedef VOID (WINAPI *LPFN_GetNativeSystemInfo)( __out LPSYSTEM_INFO lpSystemInfo );
  LPFN_GetNativeSystemInfo fnGetNativeSystemInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandleW(L"kernel32"), "GetNativeSystemInfo");
  if (NULL == fnGetNativeSystemInfo) {
    return false;
  }

  SYSTEM_INFO stInfo = {};
  fnGetNativeSystemInfo(&stInfo);
  if (stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 || stInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
    return true;
  }

  return false;
}

bool Is64BitPorcess(DWORD dwProcessID) {
  if (!Is64BitOS()) {
    return false;
  }

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessID);
  if (NULL == hProcess) {
    return false;
  }

  typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
  LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
  if (NULL == fnIsWow64Process) {
    CloseHandle(hProcess);
    return false;
  }

  BOOL bIsWow64 = FALSE;
  fnIsWow64Process(hProcess, &bIsWow64);
  CloseHandle(hProcess);
  if (bIsWow64) {
    return false;
  }

  return true;
}

#if 0
void PaintScreenBorderWindow(HWND hwnd) {
  PAINTSTRUCT paint_struct;
  HDC hDC = ::BeginPaint(hwnd, &paint_struct);
  RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << " PaintRect=(" << paint_struct.rcPaint.left << ", " << paint_struct.rcPaint.top << ")-(" << paint_struct.rcPaint.right << ", " << paint_struct.rcPaint.bottom << ")";
  RECT rect;
  ::GetClientRect(hwnd, &rect);
  RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << " ClientRect=(" << rect.left << ", " << rect.top << ")-(" << rect.right << ", " << rect.bottom << ")";

  HPEN hPen = ::CreatePen(PS_SOLID, kBorderWidth, kHighlightColor);
  HGDIOBJ hOldPen = ::SelectObject(hDC, hPen);

//  ::FrameRect(hDC, &paint_struct.rcPaint, ::CreateSolidBrush(kHighlightColor));
  ::FrameRect(hDC, &rect, ::CreateSolidBrush(kHighlightColor));

  ::SelectObject(hDC, hOldPen);
  ::DeleteObject(hPen);

  ::EndPaint(hwnd, &paint_struct);
}
#endif

void UpdateScreenBorderWindow(HWND hwnd_border) {
  DesktopRect window_rect;
  if (!GetWindowRect(hwnd_border, &window_rect)) {
    RTC_LOG(LS_WARNING) << "UpdateScreenBorderWindow: Failed to get window info: " << GetLastError();
    return;
  }
  RTC_LOG(LS_INFO) << "UpdateScreenBorderWindow hwnd=" << hwnd_border << " WindowRect=(" 
    << window_rect.left() << ", " << window_rect.top() << ")-(" << window_rect.right() << ", " << window_rect.bottom() << ")" 
    << ", " << window_rect.width() << "x" << window_rect.height();

  POINT ptDst = {window_rect.left(), window_rect.top()};
  SIZE sizeDst = {window_rect.width(), window_rect.height()};
  POINT ptSrc = {0, 0};

  HDC hDC = ::GetDC(NULL);
  HDC hMemDC = ::CreateCompatibleDC(hDC);
  HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, sizeDst.cx, sizeDst.cy);
  HGDIOBJ hOldBitmap = ::SelectObject(hMemDC, hMemBitmap);

  RECT rect;
  ::GetClientRect(hwnd_border, &rect);
  RTC_LOG(LS_INFO) << "UpdateScreenBorderWindow hwnd=" << hwnd_border << " ClientRect=(" << rect.left << ", " << rect.top << ")-(" << rect.right << ", " << rect.bottom << ")";
  ::FillRect(hMemDC, &rect, ::CreateSolidBrush(kHighlightColor));
  ::InflateRect(&rect, -kBorderWidth, -kBorderWidth);
  RTC_LOG(LS_INFO) << "UpdateScreenBorderWindow hwnd=" << hwnd_border << " TransparentClientRect=(" << rect.left << ", " << rect.top << ")-(" << rect.right << ", " << rect.bottom << ")";
  ::FillRect(hMemDC, &rect, ::CreateSolidBrush(kTransparentColor));

  if (::UpdateLayeredWindow(hwnd_border, hDC, &ptDst, &sizeDst, hMemDC, &ptSrc, kTransparentColor, NULL, ULW_COLORKEY)) {
    RTC_LOG(LS_INFO) << "UpdateLayeredWindow OK hwnd=" << hwnd_border;
  } else {
    RTC_LOG(LS_WARNING) << "UpdateLayeredWindow failed hwnd=" << hwnd_border;
  }

  ::SelectObject(hMemDC, hOldBitmap);
  ::DeleteObject(hMemBitmap);
  ::DeleteDC(hMemDC);
  ::ReleaseDC(NULL, hDC);
}

const char* MsgString(UINT msg) {
  switch (msg) {
    case WM_NULL:
      return "WM_NULL";
    case WM_CREATE:
      return "WM_CREATE";
    case WM_DESTROY:
      return "WM_DESTROY";
    case WM_MOVE:
      return "WM_MOVE";
    case WM_SIZE:
      return "WM_SIZE";
    case WM_MOVING:
      return "WM_MOVING";
    case WM_SIZING:
      return "WM_SIZING";
    case WM_NCPAINT:
      return "WM_NCPAINT";
    case WM_PAINT:
      return "WM_PAINT";
    case WM_CLOSE:
      return "WM_CLOSE";
    case WM_QUIT:
      return "WM_QUIT";

    case WM_ACTIVATE:
      return "WM_ACTIVATE";
    case WM_ACTIVATEAPP:
      return "WM_ACTIVATEAPP";
    case WM_NCACTIVATE:
      return "WM_NCACTIVATE";
    case WM_MOUSEACTIVATE:
      return "WM_MOUSEACTIVATE";
    case WM_SETFOCUS:
      return "WM_SETFOCUS";

    case WM_WINDOWPOSCHANGING:
      return "WM_WINDOWPOSCHANGING";
    case WM_WINDOWPOSCHANGED:
      return "WM_WINDOWPOSCHANGED";

    default:
      return "WM_XXX";
  }
}

void FlagString(UINT uFlags, char *sFlags) {
  sprintf(sFlags, "0x%X", uFlags);

  if (uFlags & SWP_NOSIZE) {
    strcat(sFlags, "|NOSIZE");
  }
  if (uFlags & SWP_NOMOVE) {
    strcat(sFlags, "|NOMOVE");
  }
  if (uFlags & SWP_NOZORDER) {
    strcat(sFlags, "|NOZORDER");
  }

  if (uFlags & SWP_NOREDRAW) {
    strcat(sFlags, "|NOREDRAW");
  }
  if (uFlags & SWP_NOACTIVATE) {
    strcat(sFlags, "|NOACTIVATE");
  }
  if (uFlags & SWP_FRAMECHANGED) {
    strcat(sFlags, "|FRAMECHANGED");
  }

  if (uFlags & SWP_SHOWWINDOW) {
    strcat(sFlags, "|SHOW");
  }
  if (uFlags & SWP_HIDEWINDOW) {
    strcat(sFlags, "|HIDE");
  }

  if (uFlags & SWP_NOCOPYBITS) {
    strcat(sFlags, "|NOCOPYBITS");
  }
  if (uFlags & SWP_NOOWNERZORDER) {
    strcat(sFlags, "|NOOWNERZORDER");
  }
  if (uFlags & SWP_NOSENDCHANGING) {
    strcat(sFlags, "|NOSENDCHANGING");
  }

#if(WINVER >= 0x0400)
  if (uFlags & SWP_DEFERERASE) {
    strcat(sFlags, "|DEFERERASE");
  }
  if (uFlags & SWP_ASYNCWINDOWPOS) {
    strcat(sFlags, "|ASYNCWINDOWPOS");
  }
#endif /* WINVER >= 0x0400 */
}

LRESULT CALLBACK HandleHookMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_MOVE:
    case WM_SIZE:
    case WM_MOVING:
    case WM_SIZING:
    {
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", hook_msg=" << MsgString(msg);
      DesktopRect window_rect;
      if (GetCroppedWindowRect(kHwnd, true, &window_rect, nullptr)) {
        if (::MoveWindow(hwnd, window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height(), FALSE)) {
          RTC_LOG(LS_INFO) << "MoveWindow OK: x=" << window_rect.left() << ", y=" << window_rect.top() << ", cx=" << window_rect.width() << ", cy=" << window_rect.height();
        } else {
          RTC_LOG(LS_INFO) << "MoveWindow Failed: error=" << GetLastError() << ", x=" << window_rect.left() << ", y=" << window_rect.top() << ", cx=" << window_rect.width() << ", cy=" << window_rect.height();
        }
        UpdateScreenBorderWindow(hwnd);
      }
      break;
    }

    case WM_NCACTIVATE:
    case WM_MOUSEACTIVATE:
    case WM_SETFOCUS:
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", hook_msg=" << MsgString(msg);
      break;

    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", hook_msg=" << MsgString(msg) << ", wParam=" << wParam;
      if (wParam) {
        if (::SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW)) {
          RTC_LOG(LS_INFO) << "SetWindowPos(SHOW) OK";
        } else {
          RTC_LOG(LS_INFO) << "SetWindowPos(SHOW) Failed: error=" << GetLastError();
        }
      }
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
      HWND hwndInsertAfter = (HWND)wParam;
      UINT flags = lParam;
      char sFlags[256];
      FlagString(flags, sFlags);
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", hook_msg=" << MsgString(msg) << ", hwndInsertAfter=" << hwndInsertAfter << ", flags=" << sFlags;

      if (msg == WM_WINDOWPOSCHANGED) {
        break;
      }

      DesktopRect window_rect;
      if (GetCroppedWindowRect(kHwnd, true, &window_rect, nullptr)) {
        UINT uFlags = 0;
        if (flags & SWP_NOSIZE) uFlags |= SWP_NOSIZE;
        if (flags & SWP_NOMOVE) uFlags |= SWP_NOMOVE;
        if (flags & SWP_NOZORDER) uFlags |= SWP_NOZORDER;
        if (flags & SWP_SHOWWINDOW) uFlags |= SWP_SHOWWINDOW;
        if (flags & SWP_HIDEWINDOW) uFlags |= SWP_HIDEWINDOW;
        FlagString(uFlags, sFlags);
        if (::SetWindowPos(hwnd, hwndInsertAfter, window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height(), uFlags)) {
          RTC_LOG(LS_INFO) << "SetWindowPos OK: x=" << window_rect.left() << ", y=" << window_rect.top() << ", cx=" << window_rect.width() << ", cy=" << window_rect.height() << ", flags=" << sFlags;
        } else {
          RTC_LOG(LS_INFO) << "SetWindowPos Failed: error=" << GetLastError() << ", x=" << window_rect.left() << ", y=" << window_rect.top() << ", cx=" << window_rect.width() << ", cy=" << window_rect.height() << ", flags=" << sFlags;
        }
        if ((uFlags & SWP_SHOWWINDOW) || (uFlags & SWP_HIDEWINDOW)) {
          UpdateScreenBorderWindow(hwnd);
        }
      }
      break;
    }

    default:
      if (msg != WM_SETCURSOR && msg != WM_NCHITTEST &&
          (msg <= WM_GETDLGCODE || msg > WM_NCMOUSEMOVE)) {
        RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd <<  ", hook_msg=" << msg;
      }
      break;
  }

  return 0;
}

LRESULT CALLBACK HandleMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE:
    case WM_DESTROY:
    case WM_NCPAINT:
    case WM_PAINT:
    case WM_CLOSE:
    case WM_QUIT:
    case WM_MOVING:
    case WM_SIZING:
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd <<  ", msg=" << MsgString(msg);
      break;

    case WM_MOVE:
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", x=" << LOWORD(lParam) << ", y=" << HIWORD(lParam);
      break;

    case WM_SIZE:
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", type=" << wParam << ", width=" << LOWORD(lParam) << ", height=" << HIWORD(lParam);
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS wp = *(WINDOWPOS*)lParam;
      char sFlags[256];
      FlagString(wp.flags, sFlags);
      RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", hwndInsertAfter=" << wp.hwndInsertAfter
                       << ", x=" << wp.x << ", y=" << wp.y << ", cx=" << wp.cx << ", cy=" << wp.cy << ", flags=" << sFlags;
      break;
    }

    default:
      if (msg != WM_GETTEXT) {
        RTC_LOG(LS_INFO) << "ScreenBorderWindowProc hwnd=" << hwnd <<  ", msg=" << msg;
      }
      break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ScreenBorderWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg >= WM_USER) {
    return HandleHookMsg(hwnd, msg - WM_USER, wParam, lParam);
  } else {
    return HandleMsg(hwnd, msg, wParam, lParam);
  }
}

#define ID_CHECK_TIMER 100
const UINT kCheckTimer = 250; // ms
VOID CALLBACK CheckTimerProc(HWND hwnd_border, UINT message, UINT idTimer, DWORD dwTime) {
  if (kHwnd == kScreenWindow) {
    ::SetWindowPos(hwnd_border, kHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    return;
  }

  DesktopRect window_rect, cropped_rect;
  if (!GetWindowRect(hwnd_border, &window_rect)) {
    RTC_LOG(LS_INFO) << "GetWindowRect Failed: error=" << GetLastError();
  }
  if (!GetCroppedWindowRect(kHwnd, true, &cropped_rect, nullptr)) {
    RTC_LOG(LS_INFO) << "GetCroppedWindowRect Failed: error=" << GetLastError();
  }
  if (!window_rect.equals(cropped_rect)) {
    if (::MoveWindow(hwnd_border, cropped_rect.left(), cropped_rect.top(), cropped_rect.width(), cropped_rect.height(), FALSE)) {
      RTC_LOG(LS_INFO) << "MoveWindow OK: x=" << cropped_rect.left() << ", y=" << cropped_rect.top() << ", cx=" << cropped_rect.width() << ", cy=" << cropped_rect.height();
    } else {
      RTC_LOG(LS_INFO) << "MoveWindow Failed: error=" << GetLastError() << ", x=" << cropped_rect.left() << ", y=" << cropped_rect.top() << ", cx=" << cropped_rect.width() << ", cy=" << cropped_rect.height();
    }
    UpdateScreenBorderWindow(hwnd_border);
  }

  HWND hInsertAfter = ::GetWindow(kHwnd, GW_HWNDPREV);
  if (hInsertAfter != hwnd_border) {
    UINT uFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW;
    char sFlags[256];
    FlagString(uFlags, sFlags);
    if (::SetWindowPos(hwnd_border, hInsertAfter, 0, 0, 0, 0, uFlags)) {
      RTC_LOG(LS_INFO) << "SetWindowPos OK: hInsertAfter=" << hInsertAfter << ", flags=" << sFlags;
    } else {
      RTC_LOG(LS_INFO) << "SetWindowPos Failed: error=" << GetLastError() << ", hInsertAfter=" << hInsertAfter << ", flags=" << sFlags;
    }
  }
}

}  // namespace

ScreenBorderWindow::~ScreenBorderWindow() {
  Destroy();
}

bool ScreenBorderWindow::CreateForWindow(HWND hwnd) {
  const int frame_width = GetSystemMetrics(SM_CXSIZEFRAME);
  const int frame_height = GetSystemMetrics(SM_CYSIZEFRAME);
  const int border_width = GetSystemMetrics(SM_CXBORDER);
  const int border_height = GetSystemMetrics(SM_CYBORDER);
  RTC_LOG(LS_INFO) << "SM_CXSIZEFRAME=" << frame_width << ", SM_CYSIZEFRAME=" << frame_height
                   << ", SM_CXBORDER=" << border_width << ", SM_CYBORDER=" << border_height;
  DesktopRect cropped_rect, original_rect;
  if (!GetCroppedWindowRect(hwnd, true, &cropped_rect, &original_rect)) {
    RTC_LOG(LS_WARNING) << "Failed to get window info : " << GetLastError();
    return false;
  }
  RTC_LOG(LS_WARNING) << "cropped_rect : " << cropped_rect.left()
                      << ", " << cropped_rect.top()
                      << ", " << cropped_rect.right()
                      << ", " << cropped_rect.bottom();
  RTC_LOG(LS_WARNING) << "original_rect : " << original_rect.left()
                      << ", " << original_rect.top()
                      << ", " << original_rect.right()
                      << ", " << original_rect.bottom();
  if (!Create(cropped_rect, hwnd)) {
    return false;
  }

#ifdef USE_HOOK
  Hook();
#else
  ::SetTimer(hwnd_border_, ID_CHECK_TIMER, kCheckTimer, (TIMERPROC)CheckTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On : hwnd=" << hwnd_ << ", hwnd_border=" << hwnd_border_;
#endif
  return true;
}

bool ScreenBorderWindow::CreateForScreen(DesktopRect window_rect) {
  if (!Create(window_rect, kScreenWindow)) {
    return false;
  }

  ::SetTimer(hwnd_border_, ID_CHECK_TIMER, kCheckTimer, (TIMERPROC)CheckTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On : hwnd=" << hwnd_ << ", hwnd_border=" << hwnd_border_;
  return true;
}

bool ScreenBorderWindow::IsCreated() {
  if (hwnd_border_) {
    return true;
  } else {
    return false;
  }
}

void ScreenBorderWindow::Destroy() {
  if (hwnd_border_) {
    ::KillTimer(hwnd_border_, ID_CHECK_TIMER);
    RTC_LOG(LS_WARNING) << "Timer Off";
    Unhook();

    RTC_LOG(LS_INFO) << "DestroyWindow hwnd_border=" << hwnd_border_;
    ::DestroyWindow(hwnd_border_);
    hwnd_border_ = nullptr;
  }

  hwnd_ = nullptr;
  kHwnd = hwnd_;

  if (window_class_) {
    ::UnregisterClass(MAKEINTATOM(window_class_), window_instance_);
    window_instance_ = nullptr;
    window_class_ = 0;
  }
}

bool ScreenBorderWindow::Create(DesktopRect window_rect, HWND hwnd) {
  if (IsCreated()) {
    RTC_LOG(LS_WARNING) << "Create failed : already created";
    return false;
  }

  if (window_rect.is_empty()) {
    RTC_LOG(LS_WARNING) << "Create failed : empty rect";
    return false;
  }

  ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&ScreenBorderWindowProc),
                       &window_instance_);

  WNDCLASSEXW wcex;
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.style = 0;
  wcex.hInstance = window_instance_;
  wcex.lpfnWndProc = &ScreenBorderWindowProc;
  wcex.lpszClassName = kWindowClass;
  window_class_ = ::RegisterClassExW(&wcex);
  if (0 == window_class_) {
    RTC_LOG(LS_WARNING) << "RegisterClass failed : " << GetLastError();
    return false;
  }

  hwnd_ = hwnd;
  kHwnd = hwnd_;

  DWORD dwExStyle = WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED  | WS_EX_NOACTIVATE;
  if (hwnd == kScreenWindow) {
    dwExStyle |= WS_EX_TOPMOST;
  }
  DWORD dwStyle = WS_POPUP | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS;
  hwnd_border_ = ::CreateWindowExW(dwExStyle, kWindowClass, L"", dwStyle,
                              window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height(), 
                              /*parent_window=*/nullptr, /*menu_bar=*/nullptr, window_instance_,
                              /*additional_params=*/nullptr);
  if (NULL == hwnd_border_) {
    RTC_LOG(LS_WARNING) << "CreateWindow failed : " << GetLastError();
    Destroy();
    return false;
  }

  RTC_LOG(LS_INFO) << "CreateWindow hwnd_border=" << hwnd_border_ << " (" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height();
  UpdateScreenBorderWindow(hwnd_border_);
  return true;
}

void ScreenBorderWindow::Hook() {
  if (hMod_) {
    RTC_LOG(LS_WARNING) << "Hook On Already";
    return;
  }

  DWORD dwPid = GetCurrentProcessId();
  bool b64Bit = Is64BitPorcess(dwPid);
#if defined(_WIN64)
  RTC_LOG(LS_WARNING) << "PID " << dwPid << ", build as 64bit, run as " << (b64Bit ? "64bit" : "32bit");
  hMod_ = LoadLibrary(L"hook64.dll");
#elif defined(_WIN32)
  RTC_LOG(LS_WARNING) << "PID " << dwPid << ", build as 32bit, run as " << (b64Bit ? "64bit" : "32bit");
  hMod_ = LoadLibrary(L"hook32.dll");
#else
  RTC_LOG(LS_WARNING) << "PID " << dwPid << ", run as " << (b64Bit ? "64bit" : "32bit");
#endif
  if (NULL == hMod_) {
    RTC_LOG(LS_WARNING) << "LoadLibrary Failed: error=" << GetLastError();
    return;
  }

  VOID(WINAPI * fhook)(HWND, HWND);
  fhook = (VOID (WINAPI *)(HWND, HWND))GetProcAddress(hMod_, "SetHookOn");
  if (NULL == fhook) {
    RTC_LOG(LS_WARNING) << "Not found SetHookOn";
    FreeLibrary(hMod_);
    hMod_= nullptr;
    return;
  }

  fhook(hwnd_, hwnd_border_);
  RTC_LOG(LS_WARNING) << "Hook On : hwnd=" << hwnd_ << ", hwnd_border=" << hwnd_border_;
}

void ScreenBorderWindow::Unhook() {
  if (NULL == hMod_) {
    RTC_LOG(LS_WARNING) << "Hook Off Already";
    return;
  }

  VOID(WINAPI * funhook)();
  funhook = (VOID (WINAPI *)())GetProcAddress(hMod_, "SetHookOff");
  if (NULL == funhook) {
    RTC_LOG(LS_WARNING) << "Not found SetHookOff";
    FreeLibrary(hMod_);
    hMod_= nullptr;
    return;
  }

  funhook();
  FreeLibrary(hMod_);
  hMod_= nullptr;
  RTC_LOG(LS_WARNING) << "Hook Off";
}

}  // namespace webrtc
