// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "modules/desktop_capture/window_border.h"
#include "modules/desktop_capture/win/window_capture_utils.h"
#include "rtc_base/constructor_magic.h"

#include "rtc_base/logging.h"
#include "rtc_base/win32.h"

namespace webrtc {

namespace {

class WindowBorderWin : public WindowBorder {
 public:
  explicit WindowBorderWin() = default;
  ~WindowBorderWin() override;

  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;

  HWND GetSourceWindow();

 private:
  bool Create(const DesktopRect &window_rect, HWND hwnd);

  HINSTANCE window_instance_ = nullptr;
  ATOM window_class_ = 0;
  HWND border_hwnd_ = nullptr;
  HWND source_hwnd_ = nullptr;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderWin);
};

#define ID_UPDATE_TIMER 100

const WCHAR kWindowClass[] = L"BorderWindowClass";
const COLORREF kHighlightColor = RGB(255, 0, 0);
const COLORREF kTransparentColor = RGB(0, 0, 0);
const int kBorderWidth = 2;
const HWND kScreenWindow = HWND_TOP;
const UINT kUpdateScreenInterval = 250; // ms
const UINT kUpdateWindowInterval = 15; // ms

void UpdateBorderWindow(HWND border_hwnd) {
  DesktopRect window_rect;
  if (!GetWindowRect(border_hwnd, &window_rect)) {
    RTC_LOG(LS_WARNING) << "UpdateBorderWindow: Failed to get window info: " << GetLastError();
    return;
  }
  RTC_LOG(LS_INFO) << "UpdateBorderWindow border_hwnd=" << border_hwnd << " WindowRect=(" 
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
  ::GetClientRect(border_hwnd, &rect);
  RTC_LOG(LS_INFO) << "UpdateBorderWindow border_hwnd=" << border_hwnd << " ClientRect=(" << rect.left << ", " << rect.top << ")-(" << rect.right << ", " << rect.bottom << ")";
  ::FillRect(hMemDC, &rect, ::CreateSolidBrush(kHighlightColor));
  ::InflateRect(&rect, -kBorderWidth, -kBorderWidth);
  RTC_LOG(LS_INFO) << "UpdateBorderWindow border_hwnd=" << border_hwnd << " TransparentClientRect=(" << rect.left << ", " << rect.top << ")-(" << rect.right << ", " << rect.bottom << ")";
  ::FillRect(hMemDC, &rect, ::CreateSolidBrush(kTransparentColor));

  if (::UpdateLayeredWindow(border_hwnd, hDC, &ptDst, &sizeDst, hMemDC, &ptSrc, kTransparentColor, NULL, ULW_COLORKEY)) {
    RTC_LOG(LS_INFO) << "UpdateLayeredWindow OK border_hwnd=" << border_hwnd;
  } else {
    RTC_LOG(LS_WARNING) << "UpdateLayeredWindow failed border_hwnd=" << border_hwnd;
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
    case WM_NCCREATE:
      return "WM_NCCREATE";
    case WM_DESTROY:
      return "WM_DESTROY";
    case WM_NCDESTROY:
      return "WM_NCDESTROY";
    case WM_MOVE:
      return "WM_MOVE";
    case WM_SIZE:
      return "WM_SIZE";
    case WM_MOVING:
      return "WM_MOVING";
    case WM_SIZING:
      return "WM_SIZING";
    case WM_NCCALCSIZE:
      return "WM_NCCALCSIZE";
    case WM_NCHITTEST:
      return "WM_NCHITTEST";
    case WM_PAINT:
      return "WM_PAINT";
    case WM_NCPAINT:
      return "WM_NCPAINT";
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
      return "";
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

LRESULT CALLBACK BorderWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_MOVE:
      RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", x=" << LOWORD(lParam) << ", y=" << HIWORD(lParam);
      break;

    case WM_SIZE:
      RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", type=" << wParam << ", width=" << LOWORD(lParam) << ", height=" << HIWORD(lParam);
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
      WINDOWPOS wp = *(WINDOWPOS*)lParam;
      char sFlags[256];
      FlagString(wp.flags, sFlags);
      RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", hwndInsertAfter=" << wp.hwndInsertAfter
                       << ", x=" << wp.x << ", y=" << wp.y << ", cx=" << wp.cx << ", cy=" << wp.cy << ", flags=" << sFlags;
      break;
    }

    default:
      if (msg != WM_NULL && msg != WM_GETTEXT && msg != WM_GETTEXTLENGTH) {
        if (strlen(MsgString(msg)) > 0) {
          RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd <<  ", msg=" << MsgString(msg);
        } else {
          RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd <<  ", msg=" << msg;
        }
      }
      break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

VOID CALLBACK UpdateScreenTimerProc(HWND border_hwnd, UINT message, UINT idTimer, DWORD dwTime) {
  UINT uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
  char sFlags[256];
  FlagString(uFlags, sFlags);
  if (!::SetWindowPos(border_hwnd, kScreenWindow, 0, 0, 0, 0, uFlags)) {
    RTC_LOG(LS_INFO) << "SetWindowPos Failed: error=" << GetLastError() << ", hInsertAfter=" << kScreenWindow << ", flags=" << sFlags;
  }
}

VOID CALLBACK UpdateWindowTimerProc(HWND border_hwnd, UINT message, UINT idTimer, DWORD dwTime) {
  WindowBorderWin *pThis = (WindowBorderWin *)::GetWindowLongPtr(border_hwnd, GWLP_USERDATA);
  if (NULL == pThis) {
    RTC_LOG(LS_ERROR) << "GWLP_USERDATA is NULL";
    return;
  }
  HWND source_hwnd = pThis->GetSourceWindow();
  if (NULL == source_hwnd) {
    RTC_LOG(LS_ERROR) << "source_hwnd is NULL";
    return;
  }

  HWND hInsertAfter = ::GetWindow(source_hwnd, GW_HWNDPREV);
  if (hInsertAfter != border_hwnd) {
    UINT uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
    char sFlags[256];
    FlagString(uFlags, sFlags);
    if (!::SetWindowPos(border_hwnd, hInsertAfter, 0, 0, 0, 0, uFlags)) {
      RTC_LOG(LS_INFO) << "SetWindowPos Failed: error=" << GetLastError() << ", hInsertAfter=" << hInsertAfter << ", flags=" << sFlags;
    }
  }

  DesktopRect window_rect, cropped_rect;
  if (!GetWindowRect(border_hwnd, &window_rect)) {
    RTC_LOG(LS_INFO) << "GetWindowRect Failed: error=" << GetLastError();
  }
  if (!GetCroppedWindowRect(source_hwnd, true, &cropped_rect, nullptr)) {
    RTC_LOG(LS_INFO) << "GetCroppedWindowRect Failed: error=" << GetLastError();
  }
  if (!window_rect.equals(cropped_rect)) {
    if (::MoveWindow(border_hwnd, cropped_rect.left(), cropped_rect.top(), cropped_rect.width(), cropped_rect.height(), FALSE)) {
      RTC_LOG(LS_INFO) << "MoveWindow OK: x=" << cropped_rect.left() << ", y=" << cropped_rect.top() << ", cx=" << cropped_rect.width() << ", cy=" << cropped_rect.height();
    } else {
      RTC_LOG(LS_INFO) << "MoveWindow Failed: error=" << GetLastError() << ", x=" << cropped_rect.left() << ", y=" << cropped_rect.top() << ", cx=" << cropped_rect.width() << ", cy=" << cropped_rect.height();
    }
    UpdateBorderWindow(border_hwnd);
  }
}

WindowBorderWin::~WindowBorderWin() {
  Destroy();
}

bool WindowBorderWin::CreateForWindow(DesktopCapturer::SourceId source_id) {
  HWND hwnd = reinterpret_cast<HWND>(source_id);
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

  ::SetTimer(border_hwnd_, ID_UPDATE_TIMER, kUpdateWindowInterval, (TIMERPROC)UpdateWindowTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On : source_hwnd=" << source_hwnd_ << ", border_hwnd=" << border_hwnd_ << ", Elapse=" << kUpdateWindowInterval;
  return true;
}

bool WindowBorderWin::CreateForScreen(const DesktopRect &window_rect) {
  if (!Create(window_rect, kScreenWindow)) {
    return false;
  }

  ::SetTimer(border_hwnd_, ID_UPDATE_TIMER, kUpdateScreenInterval, (TIMERPROC)UpdateScreenTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On : source_hwnd=" << source_hwnd_ << ", border_hwnd=" << border_hwnd_ << ", Elapse=" << kUpdateScreenInterval;
  return true;
}

bool WindowBorderWin::IsCreated() {
  if (nullptr != border_hwnd_) {
    return true;
  } else {
    return false;
  }
}

void WindowBorderWin::Destroy() {
  if (nullptr != border_hwnd_) {
    ::KillTimer(border_hwnd_, ID_UPDATE_TIMER);
    RTC_LOG(LS_WARNING) << "Timer Off";

    RTC_LOG(LS_INFO) << "DestroyWindow border_hwnd=" << border_hwnd_;
    ::DestroyWindow(border_hwnd_);
    border_hwnd_ = nullptr;
  }

  if (0 != window_class_) {
    ::UnregisterClass(MAKEINTATOM(window_class_), window_instance_);
    window_instance_ = nullptr;
    window_class_ = 0;
  }

  source_hwnd_ = nullptr;
}

HWND WindowBorderWin::GetSourceWindow() {
  return source_hwnd_;
}

bool WindowBorderWin::Create(const DesktopRect &window_rect, HWND source_hwnd) {
  // check created
  if (IsCreated()) {
    RTC_LOG(LS_WARNING) << "Create failed : already created";
    return false;
  }

  // check parameter
  if (window_rect.is_empty()) {
    RTC_LOG(LS_WARNING) << "Create failed : empty rect";
    return false;
  }

  // save source hwnd
  source_hwnd_ = source_hwnd;

  // create border window
  ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&BorderWindowProc),
                       &window_instance_);

  WNDCLASSW wc;
  memset(&wc, 0, sizeof(wc));
  wc.lpfnWndProc = &BorderWindowProc;
  wc.hInstance = window_instance_;
  wc.lpszClassName = kWindowClass;
  window_class_ = ::RegisterClassW(&wc);
  if (0 == window_class_) {
    RTC_LOG(LS_WARNING) << "RegisterClass failed : " << GetLastError();
    return false;
  }

  DWORD dwExStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST;
  DWORD dwStyle = WS_POPUP | WS_DISABLED; // create border window without WS_BORDER style
  border_hwnd_ = ::CreateWindowExW(dwExStyle, kWindowClass, L"", dwStyle,
                              window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height(), 
                              /*parent_window=*/nullptr, /*menu_bar=*/nullptr, window_instance_,
                              /*additional_params=*/nullptr);
  if (nullptr == border_hwnd_) {
    RTC_LOG(LS_WARNING) << "CreateWindowEx failed : " << GetLastError();
    Destroy();
    return false;
  }

  RTC_LOG(LS_INFO) << "CreateWindowEx border_hwnd=" << border_hwnd_ << " (" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height();
  ::ShowWindow(border_hwnd_, SW_SHOWNA);
  UpdateBorderWindow(border_hwnd_);

  ::SetWindowLongPtr(border_hwnd_, GWLP_USERDATA, (LONG_PTR)this);
  return true;
}

}  // namespace


// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderWin());
}

}  // namespace webrtc
