// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "modules/desktop_capture/window_border.h"
#include "modules/desktop_capture/win/window_capture_utils.h"
#include "rtc_base/win32.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread_types.h"

#define USE_GDIPLUS

#ifdef USE_GDIPLUS
namespace Gdiplus {
  using std::max;
  using std::min;
}
#include <gdiplus.h>
#pragma comment (lib, "Gdiplus.lib")
#endif // USE_GDIPLUS

namespace webrtc {

namespace {

class WindowBorderWin : public WindowBorder {
 public:
  explicit WindowBorderWin();
  ~WindowBorderWin() override;

  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;
  WindowId GetBorderId() override;

  HWND GetSourceWindow();
  bool GetFrameRect(HWND hwnd, DesktopRect *frame_rect, DesktopRect* original_rect);

 private:
  bool Create(const DesktopRect &window_rect, HWND hwnd);

#ifdef USE_GDIPLUS
  ULONG_PTR gdiplusToken_ = 0;
#endif // USE_GDIPLUS
  HINSTANCE window_instance_ = nullptr;
  ATOM window_class_ = 0;
  HWND border_hwnd_ = nullptr;
  HWND source_hwnd_ = nullptr;
  WindowCaptureHelperWin window_capture_helper_;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderWin);
};

#define ID_UPDATE_TIMER 100

#ifdef USE_GDIPLUS
#else // USE_GDIPLUS
const COLORREF kHighlightColor = RGB(WindowBorder::kBorderColorR, WindowBorder::kBorderColorG, WindowBorder::kBorderColorB);
const COLORREF kTransparentColor = RGB(0, 0, 0);
#endif // USE_GDIPLUS

const WCHAR kWindowClass[] = L"BorderWindowClass";
const HWND kScreenWindow = HWND_TOP;
const UINT kUpdateScreenInterval = 250; // ms
const UINT kUpdateWindowInterval = 15; // ms

void UpdateBorderWindow(HWND border_hwnd, const DesktopRect &window_rect) {
  if (window_rect.is_empty()) {
    RTC_LOG(LS_ERROR) << "UpdateBorderWindow: window_rect is empty";
    return;
  }

  POINT ptDst = {window_rect.left(), window_rect.top()};
  SIZE sizeDst = {window_rect.width(), window_rect.height()};
  POINT ptSrc = {0, 0};

#ifdef USE_GDIPLUS
//  RTC_LOG(LS_INFO) << "UpdateBorderWindow(gdi+) border_hwnd=" << border_hwnd << " window_rect=(" 
//    << window_rect.left() << ", " << window_rect.top() << ")-(" << window_rect.right() << ", " << window_rect.bottom() << ")" 
//    << ", " << window_rect.width() << "x" << window_rect.height();

  // create bitmap
  Gdiplus::Bitmap bitmap(sizeDst.cx, sizeDst.cy, PixelFormat32bppARGB);

  // draw on bitmap
  Gdiplus::Graphics graphics(&bitmap);
  Gdiplus::Pen pen(Gdiplus::Color(255, WindowBorder::kBorderColorR, WindowBorder::kBorderColorG, WindowBorder::kBorderColorB));
  pen.SetWidth((Gdiplus::REAL)WindowBorder::kBorderWidth);
  pen.SetAlignment(Gdiplus::PenAlignment::PenAlignmentInset);
//  RTC_LOG(LS_INFO) << "UpdateBorderWindow(gdi+) PageUnit=" << graphics.GetPageUnit() << ", PageScale=" << graphics.GetPageScale() << ", Alignment=" << pen.GetAlignment();
  graphics.DrawRectangle(&pen, 0, 0, sizeDst.cx, sizeDst.cy);

  // select bitmap to dc
  HDC hMemDC = ::CreateCompatibleDC(NULL);
  HBITMAP hBitmap = NULL;
  bitmap.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
  HGDIOBJ hOldBitmap = ::SelectObject(hMemDC, hBitmap);

  // blend dc
  BLENDFUNCTION blend;
  blend.BlendOp = AC_SRC_OVER;
  blend.BlendFlags = 0;
  blend.SourceConstantAlpha = 255;
  blend.AlphaFormat = AC_SRC_ALPHA;

  UPDATELAYEREDWINDOWINFO info;
  info.cbSize = sizeof(info);
  info.hdcDst = NULL;
  info.pptDst = &ptDst;
  info.psize = &sizeDst;
  info.hdcSrc = hMemDC;
  info.pptSrc = &ptSrc;
  info.crKey = 0;
  info.pblend = &blend;
  info.dwFlags = ULW_ALPHA;
  info.prcDirty = NULL;

  if (!::UpdateLayeredWindowIndirect(border_hwnd, &info)) {
    RTC_LOG(LS_ERROR) << "UpdateLayeredWindowIndirect Failed: error=" << GetLastError();
  }
#else // USE_GDIPLUS
  RTC_LOG(LS_INFO) << "UpdateBorderWindow(gdi) border_hwnd=" << border_hwnd << " window_rect=(" 
    << window_rect.left() << ", " << window_rect.top() << ")-(" << window_rect.right() << ", " << window_rect.bottom() << ")" 
    << ", " << window_rect.width() << "x" << window_rect.height();

  HDC hDC = ::GetDC(NULL);
  HDC hMemDC = ::CreateCompatibleDC(hDC);
  HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, sizeDst.cx, sizeDst.cy);
  HGDIOBJ hOldBitmap = ::SelectObject(hMemDC, hBitmap);

  RECT client_rect = {0, 0, sizeDst.cx, sizeDst.cy};
  ::FillRect(hMemDC, &client_rect, ::CreateSolidBrush(kHighlightColor));
  ::InflateRect(&client_rect, -WindowBorder::kBorderWidth, -WindowBorder::kBorderWidth);
  ::FillRect(hMemDC, &client_rect, ::CreateSolidBrush(kTransparentColor));

  if (!::UpdateLayeredWindow(border_hwnd, hDC, &ptDst, &sizeDst, hMemDC, &ptSrc, kTransparentColor, NULL, ULW_COLORKEY)) {
    RTC_LOG(LS_ERROR) << "UpdateLayeredWindow Failed: error=" << GetLastError();
  }
#endif // USE_GDIPLUS

  ::SelectObject(hMemDC, hOldBitmap);
  ::DeleteObject(hBitmap);
  ::DeleteDC(hMemDC);
#ifdef USE_GDIPLUS
#else // USE_GDIPLUS
  ::ReleaseDC(NULL, hDC);
#endif // USE_GDIPLUS
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
    case WM_ERASEBKGND:
      return "WM_ERASEBKGND";

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
      RTC_LOG(LS_INFO) << "Window Proc(" << ::GetWindowLongPtr(hwnd, GWLP_USERDATA) << "): hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", x=" << LOWORD(lParam) << ", y=" << HIWORD(lParam);
      break;

    case WM_SIZE:
      RTC_LOG(LS_INFO) << "Window Proc(" << ::GetWindowLongPtr(hwnd, GWLP_USERDATA) << "): hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", type=" << wParam << ", width=" << LOWORD(lParam) << ", height=" << HIWORD(lParam);
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
/*
      WINDOWPOS wp = *(WINDOWPOS*)lParam;
      char sFlags[256];
      FlagString(wp.flags, sFlags);
      RTC_LOG(LS_INFO) << "BorderWindowProc hwnd=" << hwnd << ", msg=" << MsgString(msg)
                       << ", hwndInsertAfter=" << wp.hwndInsertAfter
                       << ", x=" << wp.x << ", y=" << wp.y << ", cx=" << wp.cx << ", cy=" << wp.cy << ", flags=" << sFlags;
*/
      break;
    }

    default:
      if (msg != WM_NULL && msg != WM_GETTEXT && msg != WM_GETTEXTLENGTH) {
        if (strlen(MsgString(msg)) > 0) {
          RTC_LOG(LS_INFO) << "Window Proc(" << ::GetWindowLongPtr(hwnd, GWLP_USERDATA) << "): Thread=" << rtc::CurrentThreadId() << ", hwnd=" << hwnd <<  ", msg=" << MsgString(msg);
        } else {
          RTC_LOG(LS_INFO) << "Window Proc(" << ::GetWindowLongPtr(hwnd, GWLP_USERDATA) << "): hwnd=" << hwnd <<  ", msg=" << msg;
        }
      }
      break;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

// set hTarget before hSource.
void SetWindowBefore(HWND hTarget, HWND hSource) {
  HWND hInsertAfter = ::GetWindow(hSource, GW_HWNDPREV);
  if (NULL == hInsertAfter) {
    RTC_LOG(LS_ERROR) << "GetWindow Failed: error=" << GetLastError() << ", hWnd=" << hSource;
    return;
  }

  if (hInsertAfter != hTarget) {
    UINT uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW;
    char sFlags[256];
    FlagString(uFlags, sFlags);
    if (!::SetWindowPos(hTarget, hInsertAfter, 0, 0, 0, 0, uFlags)) {
      return SetWindowBefore(hTarget, hInsertAfter); // find next valid window
    }
  }
}

VOID CALLBACK UpdateScreenTimerProc(HWND border_hwnd, UINT message, UINT idTimer, DWORD dwTime) {
  UINT uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
  char sFlags[256];
  FlagString(uFlags, sFlags);
  if (!::SetWindowPos(border_hwnd, kScreenWindow, 0, 0, 0, 0, uFlags)) {
    RTC_LOG(LS_ERROR) << "SetWindowPos Failed: error=" << GetLastError() << ", hInsertAfter=" << kScreenWindow << ", flags=" << sFlags;
  }
}

VOID CALLBACK UpdateWindowTimerProc(HWND border_hwnd, UINT message, UINT idTimer, DWORD dwTime) {
  WindowBorderWin *pThis = (WindowBorderWin *)::GetWindowLongPtr(border_hwnd, GWLP_USERDATA);
  if (NULL == pThis) {
    RTC_LOG(LS_ERROR) << "Timer Proc: GWLP_USERDATA is NULL";
    return;
  }

  HWND source_hwnd = pThis->GetSourceWindow();
  if (NULL == source_hwnd) {
    RTC_LOG(LS_ERROR) << "Timer Proc: source_hwnd is NULL";
    return;
  }

  if (!IsWindowValidAndVisible(source_hwnd)) {
    if (::IsWindowVisible(border_hwnd)) {
      UINT uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW;
      char sFlags[256];
      FlagString(uFlags, sFlags);
      if (!::SetWindowPos(border_hwnd, NULL, 0, 0, 0, 0, uFlags)) {
        RTC_LOG(LS_ERROR) << "SetWindowPos Failed: error=" << GetLastError() << ", flags=" << sFlags;
      } else {
        RTC_LOG(LS_INFO) << "Timer Proc(" << pThis << "): Thread=" << rtc::CurrentThreadId() << ": border show => hide";
      }
    }
    return;
  }

  if (!::IsWindowVisible(border_hwnd)) {
    RTC_LOG(LS_INFO) << "Timer Proc(" << pThis << "): Thread=" << rtc::CurrentThreadId() << ": border hide => show";
  }

  SetWindowBefore(border_hwnd, source_hwnd);

  DesktopRect border_rect;
  if (!GetWindowRect(border_hwnd, &border_rect)) {
    RTC_LOG(LS_ERROR) << "GetWindowRect Failed: error=" << GetLastError();
    return;
  }

  DesktopRect source_rect;
  if (!pThis->GetFrameRect(source_hwnd, &source_rect, nullptr)) {
    return;
  }

  if (!border_rect.equals(source_rect)) { // move or resize
    UpdateBorderWindow(border_hwnd, source_rect);
  }
}

WindowBorderWin::WindowBorderWin() {
#ifdef USE_GDIPLUS
  // Initialize GDI+.
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken_, &gdiplusStartupInput, NULL);
#endif // USE_GDIPLUS
}

WindowBorderWin::~WindowBorderWin() {
  Destroy();

#ifdef USE_GDIPLUS
  Gdiplus::GdiplusShutdown(gdiplusToken_);
#endif // USE_GDIPLUS
}

bool WindowBorderWin::CreateForWindow(DesktopCapturer::SourceId source_id) {
  HWND source_hwnd = reinterpret_cast<HWND>(source_id);
  DesktopRect frame_rect, original_rect;
  if (!GetFrameRect(source_hwnd, &frame_rect, &original_rect)) {
    return false;
  }

  RTC_LOG(LS_INFO) << "frame_rect: " << frame_rect.left()
                      << ", " << frame_rect.top()
                      << ", " << frame_rect.right()
                      << ", " << frame_rect.bottom();
  RTC_LOG(LS_INFO) << "original_rect: " << original_rect.left()
                      << ", " << original_rect.top()
                      << ", " << original_rect.right()
                      << ", " << original_rect.bottom();

  if (!Create(frame_rect, source_hwnd)) {
    return false;
  }

  ::SetTimer(border_hwnd_, ID_UPDATE_TIMER, kUpdateWindowInterval, (TIMERPROC)UpdateWindowTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On: source_hwnd=" << source_hwnd_ << ", border_hwnd=" << border_hwnd_ << ", Elapse=" << kUpdateWindowInterval;
  return true;
}

bool WindowBorderWin::CreateForScreen(const DesktopRect &window_rect) {
  if (!Create(window_rect, kScreenWindow)) {
    return false;
  }

  ::SetTimer(border_hwnd_, ID_UPDATE_TIMER, kUpdateScreenInterval, (TIMERPROC)UpdateScreenTimerProc);
  RTC_LOG(LS_WARNING) << "Timer On: source_hwnd=" << source_hwnd_ << ", border_hwnd=" << border_hwnd_ << ", Elapse=" << kUpdateScreenInterval;
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
    RTC_LOG(LS_INFO) << "Destroy(" << this << "): Thread=" << rtc::CurrentThreadId() << ": border_hwnd=" << border_hwnd_;
    ::KillTimer(border_hwnd_, ID_UPDATE_TIMER);
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

WindowId WindowBorderWin::GetBorderId() {
  return reinterpret_cast<WindowId>(border_hwnd_);
}

HWND WindowBorderWin::GetSourceWindow() {
  return source_hwnd_;
}

bool WindowBorderWin::GetFrameRect(HWND hwnd, DesktopRect* frame_rect, DesktopRect* original_rect) {
  if (!GetCroppedWindowRect(hwnd, true, frame_rect, nullptr)) {
    RTC_LOG(LS_ERROR) << "GetCroppedWindowRect Failed: error=" << GetLastError();
    return false;
  }

  DesktopRect dwm_rect;
  if (!window_capture_helper_.GetExtendedFrameBounds(hwnd, &dwm_rect, original_rect)) {
    return true;
  }

  if (frame_rect->equals(dwm_rect)) {
    return true;
  }

  if (original_rect) {
    RTC_LOG(LS_INFO) << "cropped_rect: " << frame_rect->left()
                        << ", " << frame_rect->top()
                        << ", " << frame_rect->right()
                        << ", " << frame_rect->bottom();
  }
  *frame_rect = dwm_rect;
  return true;
}

bool WindowBorderWin::Create(const DesktopRect &window_rect, HWND source_hwnd) {
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
    RTC_LOG(LS_ERROR) << "RegisterClass Failed: error=" << GetLastError();
    return false;
  }

  DWORD dwExStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST;
  DWORD dwStyle = WS_POPUP | WS_DISABLED; // create border window without WS_BORDER style
  border_hwnd_ = ::CreateWindowExW(dwExStyle, kWindowClass, L"", dwStyle,
                              window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height(), 
                              /*parent_window=*/nullptr, /*menu_bar=*/nullptr, window_instance_,
                              /*additional_params=*/nullptr);
  if (nullptr == border_hwnd_) {
    RTC_LOG(LS_ERROR) << "CreateWindowEx Failed: error=" << GetLastError();
    Destroy();
    return false;
  }

  RTC_LOG(LS_INFO) << "Create(" << this << "): Thread=" << rtc::CurrentThreadId() << ": border_hwnd=" << border_hwnd_ << 
                      " (" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height();
  ::ShowWindow(border_hwnd_, SW_SHOWNA);

  window_capture_helper_.SetExcludedFromPeek(border_hwnd_, TRUE);

  UpdateBorderWindow(border_hwnd_, window_rect);

  ::SetWindowLongPtr(border_hwnd_, GWLP_USERDATA, (LONG_PTR)this);
  return true;
}

}  // namespace

// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderWin());
}

}  // namespace webrtc
