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
//  RTC_LOG(LS_INFO) << "BorderView drawRect dirtyRect=(" << dirtyRect.origin.x << ", " << dirtyRect.origin.y << ") " << dirtyRect.size.width << "x" << dirtyRect.size.height;
  NSRect borderRect = NSInsetRect(self.frame, 1.5, 1.5);
//  RTC_LOG(LS_INFO) << "BorderView drawRect borderRect=(" << borderRect.origin.x << ", " << borderRect.origin.y << ") " << borderRect.size.width << "x" << borderRect.size.height;
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

  CGWindowID GetSourceId();
  NSWindow *GetBorderWindow();

 private:
  bool Create(const DesktopRect &window_rect, CGWindowID source_id);

  CGWindowID source_id_ = kCGNullWindowID;
  __strong NSWindow *border_window_ = nil;
  __strong dispatch_source_t window_timer_ = nil;

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
    Create(window_rect, kCGNullWindowID);
  });
  return true;
}

bool WindowBorderMac::IsCreated() {
  if (border_window_ != nil) {
    return true;
  } else {
    return false;
  }
}

void WindowBorderMac::Destroy() {
  if (window_timer_ != nil) {
    RTC_LOG(LS_WARNING) << "Timer Off";
    dispatch_source_cancel(window_timer_);
    window_timer_ = nil;
  }

  if (border_window_ != nil) {
    RTC_LOG(LS_WARNING) << "Destroy border_window=" << border_window_;
    [border_window_ close];
    border_window_ = nil;
  }

  source_id_ = kCGNullWindowID;
}

CGWindowID WindowBorderMac::GetSourceId() {
  return source_id_;
}

NSWindow *WindowBorderMac::GetBorderWindow() {
  return border_window_;
}

bool WindowBorderMac::Create(const DesktopRect &window_rect, CGWindowID source_id) {
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

  gfx::Rect screen_rect(window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height());
  NSRect content_rect = gfx::ScreenRectToNSRect(screen_rect);
  NSWindowLevel border_level = kCGMaximumWindowLevel;
  if (source_id != kCGNullWindowID) {
    NSWindowLevel source_level = GetWindowLevel(source_id);
    RTC_LOG(LS_INFO) << "Create Window Border: Thread=" << rtc::CurrentThreadId() << ", source_id=" << source_id << ", source_level=" << source_level << ", screen_rect(" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height()<< ", ns_rect(" << content_rect.origin.x << ", " << content_rect.origin.y << ") " << content_rect.size.width << "x" << content_rect.size.height;
    border_level = source_level;
  } else {
    RTC_LOG(LS_INFO) << "Create Screen Border: Thread=" << rtc::CurrentThreadId() << ", source_id=" << source_id << ", source_level=screen, screen_rect(" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height()<< ", ns_rect(" << content_rect.origin.x << ", " << content_rect.origin.y << ") " << content_rect.size.width << "x" << content_rect.size.height;
  }

  // save source id
  source_id_ = source_id;

  // create border window
  border_window_ =
      [[NSWindow alloc] initWithContentRect:content_rect
                                styleMask:NSBorderlessWindowMask
                                  backing:NSBackingStoreBuffered
                                    defer:NO];
  if (nil == border_window_) {
    RTC_LOG(LS_WARNING) << "Create border window failed";
    Destroy();
    return false;
  }

  [border_window_ setReleasedWhenClosed:NO];
  [border_window_ setHasShadow:NO];
  [border_window_ setOpaque:NO];
  [border_window_ setAlphaValue:1.0];
  [border_window_ setBackgroundColor:[NSColor clearColor]];
  [border_window_ setLevel:border_level];
  [border_window_ orderWindow:NSWindowAbove relativeTo:source_id];

  NSInteger border_id = [border_window_ windowNumber];
  NSInteger border_order = [border_window_ orderedIndex];
  NSRect frame_rect =  [border_window_ frame];
  RTC_LOG(LS_INFO) << "Create Border OK: window=" << border_window_ << ", id=" << border_id << ", order=" << border_order << ", frame=(" << frame_rect.origin.x << ", " << frame_rect.origin.y << ") " << frame_rect.size.width << "x" << frame_rect.size.height;

  BorderView *borderView =
      [[BorderView alloc] initWithFrame:content_rect];
  if (nil == borderView) {
    RTC_LOG(LS_WARNING) << "Create border view failed";
    Destroy();
    return false;
  }
  [border_window_ setContentView:borderView];

  if (source_id != kCGNullWindowID) {
    dispatch_queue_t queue = dispatch_get_main_queue();
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, queue);
    uint64_t intervalInNanoSecs = (uint64_t)(15 * USEC_PER_SEC);
    uint64_t leewayInNanoSecs = (uint64_t)(0 * NSEC_PER_SEC);
    dispatch_source_set_timer(timer, DISPATCH_TIME_NOW, intervalInNanoSecs, leewayInNanoSecs);
    dispatch_set_context(timer, this);
    dispatch_source_set_event_handler(timer, ^{
      WindowBorderMac *pThis = (WindowBorderMac *)dispatch_get_context(timer);
      if (pThis != nil) {
        if (!IsWindowOnScreen(pThis->GetSourceId())) {
//          RTC_LOG(LS_WARNING) << "border window hide";
          [pThis->GetBorderWindow() orderOut:nil];
          return;
        }

        NSWindowLevel border_level = [pThis->GetBorderWindow() level];
        NSWindowLevel source_level = GetWindowLevel(pThis->GetSourceId());
        if (border_level != source_level) {
          RTC_LOG(LS_WARNING) << "border level change: " << border_level << " => " << source_level;
          [pThis->GetBorderWindow() setLevel:source_level];
        }

        NSInteger border_order_old = [pThis->GetBorderWindow() orderedIndex];
        [pThis->GetBorderWindow() orderWindow:NSWindowAbove relativeTo:pThis->GetSourceId()];
        NSInteger border_order_new = [pThis->GetBorderWindow() orderedIndex];
        if (border_order_old != border_order_new) {
          RTC_LOG(LS_WARNING) << "border order change: " << border_order_old << " => " << border_order_new;
        }
        
        NSRect border_rect =  [pThis->GetBorderWindow() frame];
        DesktopRect window_rect = GetWindowBounds(pThis->GetSourceId());
        gfx::Rect screen_rect(window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height());
        NSRect source_rect = gfx::ScreenRectToNSRect(screen_rect);
        if (!NSEqualRects(source_rect, border_rect)) {
//          RTC_LOG(LS_WARNING) << "border_rect=" << " (" << border_rect.origin.x << ", " << border_rect.origin.y << ") " << border_rect.size.width << "x" << border_rect.size.height << ", source_rect=" << source_level<< " (" << source_rect.origin.x << ", " << source_rect.origin.y << ") " << source_rect.size.width << "x" << source_rect.size.height;
          [pThis->GetBorderWindow() setFrame:source_rect display:YES];
        }
      }
    });
    dispatch_resume(timer);
    window_timer_ = timer;
  }
  return true;
}

}  // namespace


// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderMac());
}

}  // namespace webrtc
