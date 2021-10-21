// Copyright (c) 2021 xxlang@grandstream.cn.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "modules/desktop_capture/window_border.h"

#import <Cocoa/Cocoa.h>

#include "modules/desktop_capture/mac/window_list_utils.h"
#include "rtc_base/constructor_magic.h"
#include "rtc_base/logging.h"
#include "rtc_base/platform_thread_types.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/mac/coordinate_conversion.h"


@interface BorderView : NSView {
  
}

- (id)initWithFrame:(NSRect)frameRect;
- (void) drawRect: (NSRect)rect;

@end

@implementation BorderView

- (id)initWithFrame:(NSRect)frameRect {
  RTC_LOG(LS_INFO) << "BorderView initWithFrame frameRect=(" << frameRect.origin.x << ", " << frameRect.origin.y << ") " << frameRect.size.width << "x" << frameRect.size.height;
  self = [super initWithFrame:frameRect];
  return self;
}

- (void)drawRect:(NSRect)dirtyRect {
  RTC_LOG(LS_INFO) << "BorderView drawRect dirtyRect=(" << dirtyRect.origin.x << ", " << dirtyRect.origin.y << ") " << dirtyRect.size.width << "x" << dirtyRect.size.height;
  NSRect borderRect = NSInsetRect(self.frame, 1.5, 1.5);
  RTC_LOG(LS_INFO) << "BorderView drawRect borderRect=(" << borderRect.origin.x << ", " << borderRect.origin.y << ") " << borderRect.size.width << "x" << borderRect.size.height;
  [NSBezierPath setDefaultLineWidth:3.0];
  NSBezierPath *path = [NSBezierPath bezierPathWithRect:borderRect];
  [[NSColor redColor] set];
  [path stroke];
}

@end


namespace webrtc {

namespace {

class WindowBorderMac : public WindowBorder {
 public:
  explicit WindowBorderMac() = default;
  ~WindowBorderMac() override;

  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;

  CGWindowID GetSourceWindow();

 private:
  bool Create(const DesktopRect &window_rect, CGWindowID hwnd);

  __strong NSWindow *hwnd_border_ = nil;
  CGWindowID hwnd_ = 0;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderMac);
};

WindowBorderMac::~WindowBorderMac() {
  Destroy();
}

bool WindowBorderMac::CreateForWindow(DesktopCapturer::SourceId source_id) {
  RTC_LOG(LS_INFO) << "CreateForWindow: Thread=" << rtc::CurrentThreadId();
  DesktopRect window_rect = GetWindowBounds(source_id);
  dispatch_async(dispatch_get_main_queue(), ^(void) {
    Create(window_rect, source_id);
  });
  return true;
}

bool WindowBorderMac::CreateForScreen(const DesktopRect &window_rect) {
  RTC_LOG(LS_INFO) << "CreateForScreen: Thread=" << rtc::CurrentThreadId();
  dispatch_async(dispatch_get_main_queue(), ^(void) {
    Create(window_rect, 0);
  });
  return true;
}

bool WindowBorderMac::IsCreated() {
  if (hwnd_border_) {
    return true;
  } else {
    return false;
  }
}

void WindowBorderMac::Destroy() {
  if (hwnd_border_) {
    RTC_LOG(LS_WARNING) << "Destroy hwnd_border=" << hwnd_border_;
    [hwnd_border_ close];
    hwnd_border_ = nil;
  }

  hwnd_ = 0;
}

CGWindowID WindowBorderMac::GetSourceWindow() {
  return hwnd_;
}

bool WindowBorderMac::Create(const DesktopRect &window_rect, CGWindowID hwnd) {
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

  // save hwnd
  hwnd_ = hwnd;

  // create border window
  RTC_LOG(LS_INFO) << "Create: Thread=" << rtc::CurrentThreadId() << ", hwnd_source=" << hwnd << " (" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height();
  gfx::Rect screen_rect(window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height());
  NSRect rect = gfx::ScreenRectToNSRect(screen_rect);
  hwnd_border_ =
      [[NSWindow alloc] initWithContentRect:rect
                                styleMask:NSBorderlessWindowMask
                                  backing:NSBackingStoreBuffered
                                    defer:NO];
  if (nil == hwnd_border_) {
    RTC_LOG(LS_WARNING) << "Create border window failed";
    Destroy();
    return false;
  }
  NSRect wndRect = [hwnd_border_ frame];
  RTC_LOG(LS_INFO) << "BorderWindow frameRect=(" << wndRect.origin.x << ", " << wndRect.origin.y << ") " << wndRect.size.width << "x" << wndRect.size.height;

  [hwnd_border_ setReleasedWhenClosed:NO];
  [hwnd_border_ setHasShadow:NO];
  [hwnd_border_ setOpaque:NO];
  [hwnd_border_ setAlphaValue:1.0];
  [hwnd_border_ setBackgroundColor:[NSColor clearColor]];
  [hwnd_border_ orderFrontRegardless];
  [hwnd_border_ setLevel:kCGCursorWindowLevel];

  BorderView *borderView =
      [[BorderView alloc] initWithFrame:rect];
  if (nil == borderView) {
    RTC_LOG(LS_WARNING) << "Create border view failed";
    Destroy();
    return false;
  }
  NSRect viewRect = [borderView frame];
  RTC_LOG(LS_INFO) << "BorderView frameRect=(" << viewRect.origin.x << ", " << viewRect.origin.y << ") " << viewRect.size.width << "x" << viewRect.size.height;
  [hwnd_border_ setContentView:borderView];

  return true;
}

}  // namespace


// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderMac());
}

}  // namespace webrtc
