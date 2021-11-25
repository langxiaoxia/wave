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
  NSRect borderRect = NSInsetRect(self.frame, (CGFloat)webrtc::WindowBorder::kBorderWidth / 2, (CGFloat)webrtc::WindowBorder::kBorderWidth / 2);
//  RTC_LOG(LS_INFO) << "BorderView drawRect borderRect=(" << borderRect.origin.x << ", " << borderRect.origin.y << ") " << borderRect.size.width << "x" << borderRect.size.height;
  [NSBezierPath setDefaultLineWidth:(CGFloat)webrtc::WindowBorder::kBorderWidth];
  NSBezierPath *path = [NSBezierPath bezierPathWithRect:borderRect];
  [[NSColor colorWithSRGBRed:(CGFloat)webrtc::WindowBorder::kBorderColorR / 0xff
                       green:(CGFloat)webrtc::WindowBorder::kBorderColorG / 0xff
                        blue:(CGFloat)webrtc::WindowBorder::kBorderColorB / 0xff
                       alpha:1.0] setStroke];
  [path stroke];
}

@end


namespace webrtc {

namespace {

// Returns true if the window exists.
bool IsWindowValid(CGWindowID id) {
  CFArrayRef window_id_array =
      CFArrayCreate(nullptr, reinterpret_cast<const void**>(&id), 1, nullptr);
  CFArrayRef window_array =
      CGWindowListCreateDescriptionFromArray(window_id_array);
  bool valid = window_array && CFArrayGetCount(window_array);
  CFRelease(window_id_array);
  CFRelease(window_array);

  return valid;
}

class WindowBorderMac : public WindowBorder {
 public:
  explicit WindowBorderMac() = default;
  ~WindowBorderMac() override;

  bool CreateForWindow(DesktopCapturer::SourceId source_id) override;
  bool CreateForScreen(const DesktopRect &window_rect) override;
  bool IsCreated() override;
  void Destroy() override;
  WindowId GetBorderId() override;

 private:
  bool Create(const DesktopRect &window_rect, CGWindowID source_id);
  void SetTimer();
  void DestroyInternal(bool sync);

  CGWindowID source_id_ = kCGNullWindowID;
  __strong NSWindow *border_window_ = nil;
  __strong dispatch_source_t window_timer_ = nil;

  RTC_DISALLOW_COPY_AND_ASSIGN(WindowBorderMac);
};

WindowBorderMac::~WindowBorderMac() {
  DestroyInternal(false);
}

bool WindowBorderMac::CreateForWindow(DesktopCapturer::SourceId source_id) {
  RTC_LOG(LS_INFO) << "CreateForWindow(" << this << "): Thread=" << rtc::CurrentThreadId();
  // must create window in main thread
  dispatch_semaphore_t border_created = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_main_queue(), ^(void) {
    DesktopRect window_rect = GetWindowBounds(source_id);
    if (Create(window_rect, source_id)) {
      SetTimer();
    }
    dispatch_semaphore_signal(border_created);
  });
  dispatch_semaphore_wait(border_created, DISPATCH_TIME_FOREVER);
  border_created = nil; // ARC
  RTC_LOG(LS_INFO) << "CreateForWindow(" << this << ") completed";
  return true;
}

bool WindowBorderMac::CreateForScreen(const DesktopRect &window_rect) {
  RTC_LOG(LS_INFO) << "CreateForScreen(" << this << "): Thread=" << rtc::CurrentThreadId();
  // must create window in main thread
  dispatch_semaphore_t border_created = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_main_queue(), ^(void) {
    Create(window_rect, kCGNullWindowID);
    dispatch_semaphore_signal(border_created);
  });
  dispatch_semaphore_wait(border_created, DISPATCH_TIME_FOREVER);
  border_created = nil; // ARC
  RTC_LOG(LS_INFO) << "CreateForScreen(" << this << ") completed";
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
  DestroyInternal(true);
}

WindowId WindowBorderMac::GetBorderId() {
  if (border_window_ != nil) {
    return [border_window_ windowNumber];
  } else {
    return 0;
  }
}

bool WindowBorderMac::Create(const DesktopRect &window_rect, CGWindowID source_id) {
  RTC_LOG(LS_INFO) << "Create(" << this << "): Thread=" << rtc::CurrentThreadId();
  // check created
  if (IsCreated()) {
    RTC_LOG(LS_WARNING) << "Create failed: already created";
    return false;
  }

  // check parameter
  if (window_rect.is_empty()) {
    RTC_LOG(LS_WARNING) << "Create failed: empty rect";
    return false;
  }

  gfx::Rect screen_rect(window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height());
  NSRect content_rect = gfx::ScreenRectToNSRect(screen_rect);
  NSWindowLevel border_level = kCGMaximumWindowLevel;
  if (source_id != kCGNullWindowID) {
    NSWindowLevel source_level = GetWindowLevel(source_id);
    RTC_LOG(LS_INFO) << "source_id=" << source_id <<
                        ", source_level=" << source_level <<
                        ", screen_rect(" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height()<<
                        ", ns_rect(" << content_rect.origin.x << ", " << content_rect.origin.y << ") " << content_rect.size.width << "x" << content_rect.size.height;
    border_level = source_level;
  } else {
    RTC_LOG(LS_INFO) << "source_id=" << source_id <<
                        ", source_level=screen" <<
                        ", screen_rect(" << window_rect.left() << ", " << window_rect.top() << ") " << window_rect.width() << "x" << window_rect.height()<<
                        ", ns_rect(" << content_rect.origin.x << ", " << content_rect.origin.y << ") " << content_rect.size.width << "x" << content_rect.size.height;
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
  [border_window_ setLevel:border_level]; // set border level same as source
  [border_window_ orderWindow:NSWindowAbove relativeTo:source_id]; // set border above source
  [border_window_ setSharingType:NSWindowSharingNone];

  NSInteger border_id = [border_window_ windowNumber];
  NSInteger border_order = [border_window_ orderedIndex];
  NSRect frame_rect =  [border_window_ frame];
  RTC_LOG(LS_INFO) << "Create border window OK: window=" << border_window_ <<
                      ", id=" << border_id <<
                      ", order=" << border_order <<
                      ", frame=(" << frame_rect.origin.x << ", " << frame_rect.origin.y << ") " << frame_rect.size.width << "x" << frame_rect.size.height;

  BorderView *borderView =
      [[BorderView alloc] initWithFrame:content_rect];
  if (nil == borderView) {
    RTC_LOG(LS_WARNING) << "Create border view failed";
    Destroy();
    return false;
  }

  [border_window_ setContentView:borderView];
  return true;
}

void WindowBorderMac::SetTimer() {
  window_timer_ = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());

  // set interval 15ms
  uint64_t intervalInNanoSecs = (uint64_t)(15 * USEC_PER_SEC);
  uint64_t leewayInNanoSecs = (uint64_t)(0 * NSEC_PER_SEC);
  dispatch_source_set_timer(window_timer_, DISPATCH_TIME_NOW, intervalInNanoSecs, leewayInNanoSecs);

  // set event handler
  dispatch_source_set_event_handler(window_timer_, ^{
    WindowBorderMac *pThis = (WindowBorderMac *)dispatch_get_context(window_timer_);
    if (!IsWindowValid(source_id_)) {
      RTC_LOG(LS_WARNING) << "Timer Event Handler(" << pThis << "): source window is invalid";
      return;
    }
    
    if (nil == border_window_) {
      RTC_LOG(LS_WARNING) << "Timer Event Handler(" << pThis << "): border window is invalid";
      return;
    }

    if (!IsWindowOnScreen(source_id_)) {
      if (IsWindowOnScreen(GetBorderId())) {
        RTC_LOG(LS_INFO) << "Timer Event Handler(" << pThis << "): border show => hide";
        [border_window_ orderOut:nil];
      }
      return;
    }

    if (!IsWindowOnScreen(GetBorderId())) {
      RTC_LOG(LS_INFO) << "Timer Event Handler(" << pThis << "): border hide => show";
    }

    NSWindowLevel border_level = [border_window_ level];
    NSWindowLevel source_level = GetWindowLevel(source_id_);
    if (border_level != source_level) {
      RTC_LOG(LS_WARNING) << "Timer Event Handler(" << pThis << "): border level " << border_level << " => " << source_level;
      [border_window_ setLevel:source_level];
    }

    NSInteger border_order_old = [border_window_ orderedIndex];
    [border_window_ orderWindow:NSWindowAbove relativeTo:source_id_];
    NSInteger border_order_new = [border_window_ orderedIndex];
    if (border_order_old != border_order_new) {
//      RTC_LOG(LS_INFO) << "Timer Event Handler(" << pThis << "): border order " << border_order_old << " => " << border_order_new;
    }
    
    NSRect border_rect =  [border_window_ frame];
    DesktopRect window_rect = GetWindowBounds(source_id_);
    gfx::Rect screen_rect(window_rect.left(), window_rect.top(), window_rect.width(), window_rect.height());
    NSRect source_rect = gfx::ScreenRectToNSRect(screen_rect);
    if (!NSEqualRects(source_rect, border_rect)) {
//      RTC_LOG(LS_INFO) << "border_rect=(" << border_rect.origin.x << ", " << border_rect.origin.y << ") " << border_rect.size.width << "x" << border_rect.size.height <<
//                          ", source_rect=(" << source_rect.origin.x << ", " << source_rect.origin.y << ") " << source_rect.size.width << "x" << source_rect.size.height;
      [border_window_ setFrame:source_rect display:YES];
    }
  });

  dispatch_resume(window_timer_);
}

void WindowBorderMac::DestroyInternal(bool sync) {
  if (window_timer_ != nil) {
    if (sync) {
      RTC_LOG(LS_INFO) << "Sync Cancel Timer(" << this << "): Thread=" << rtc::CurrentThreadId();
      dispatch_semaphore_t timer_cancelled = dispatch_semaphore_create(0);
      dispatch_source_set_cancel_handler(window_timer_, ^{
        RTC_LOG(LS_INFO) << "Sync Timer Cancel Handler: Thread=" << rtc::CurrentThreadId() << ", window_timer=" << window_timer_;
        if (window_timer_) {
          dispatch_semaphore_signal(timer_cancelled);
        }
      });
      dispatch_source_cancel(window_timer_);
      dispatch_semaphore_wait(timer_cancelled, DISPATCH_TIME_FOREVER);
      timer_cancelled = nil; // ARC
    } else {
      RTC_LOG(LS_INFO) << "Async Cancel Timer(" << this << "): Thread=" << rtc::CurrentThreadId();
      dispatch_source_set_cancel_handler(window_timer_, ^{
        RTC_LOG(LS_INFO) << "Async Timer Cancel Handler: Thread=" << rtc::CurrentThreadId() << ", window_timer=" << window_timer_;
      });
      dispatch_source_cancel(window_timer_);
    }

    window_timer_ = nil; // ARC
    RTC_LOG(LS_INFO) << "Timer cancelled";
  }

  if (border_window_ != nil) {
    RTC_LOG(LS_INFO) << "Close border: border_window=" << border_window_;
    [border_window_ close];
    border_window_ = nil; // ARC
    RTC_LOG(LS_INFO) << "Border closed";
  }

  source_id_ = kCGNullWindowID;
}

}  // namespace


// static
std::unique_ptr<WindowBorder> DesktopCapturer::CreateWindowBorder() {
  return std::unique_ptr<WindowBorder>(new WindowBorderMac());
}

}  // namespace webrtc
