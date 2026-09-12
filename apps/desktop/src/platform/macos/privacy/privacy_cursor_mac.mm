/* Copyright (c) 2026 by DI JUNKUN, All Rights Reserved. */
#include "privacy_cursor_mac.h"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <atomic>
#include <dlfcn.h>

@interface CrossDeskPrivacyCursorPanel : NSPanel
@end
@implementation CrossDeskPrivacyCursorPanel
- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)canBecomeMainWindow { return NO; }
@end

namespace crossdesk {
namespace {
std::atomic<bool> hidden_by_privacy{false};
struct CursorApi {
  using Connection = int (*)();
  using Copy = CGError (*)(int, int, CFStringRef, CFTypeRef*);
  using Set = CGError (*)(int, int, CFStringRef, CFTypeRef);
  Connection connection = reinterpret_cast<Connection>(dlsym(RTLD_DEFAULT, "CGSMainConnectionID"));
  Copy copy = reinterpret_cast<Copy>(dlsym(RTLD_DEFAULT, "CGSCopyConnectionProperty"));
  Set set = reinterpret_cast<Set>(dlsym(RTLD_DEFAULT, "CGSSetConnectionProperty"));
};
const CursorApi& Api() { static const CursorApi api; return api; }
CFStringRef const kBackgroundCursor = CFSTR("SetsCursorInBackground");
}

struct MacPrivacyCursor::Impl {
  int connection = 0;
  bool previous_background = false;
  bool background_changed = false;
  unsigned hide_count = 0;
  unsigned visible_ticks = 0;
  bool healthy = true;
  NSTimer* __strong timer = nil;
  CrossDeskPrivacyCursorPanel* __strong panel = nil;
  NSImageView* __strong view = nil;

  void UpdateImage() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSCursor* cursor = NSCursor.currentSystemCursor ?: NSCursor.arrowCursor;
#pragma clang diagnostic pop
    NSImage* image = cursor.image;
    if (!image || image.size.width <= 0 || image.size.height <= 0) return;
    const NSPoint point = NSEvent.mouseLocation;
    const NSSize size = image.size;
    [panel setFrame:NSMakeRect(point.x - cursor.hotSpot.x,
        point.y - size.height + cursor.hotSpot.y, size.width, size.height) display:NO];
    view.image = image;
    [panel displayIfNeeded];
  }

  bool KeepHidden() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (hide_count && !CGCursorIsVisible()) { visible_ticks = 0; return true; }
#pragma clang diagnostic pop
    if (++visible_ticks > 6 ||
        CGDisplayHideCursor(kCGDirectMainDisplay) != kCGErrorSuccess) return false;
    ++hide_count;
    return true;
  }
};

MacPrivacyCursor::MacPrivacyCursor() : impl_(std::make_unique<Impl>()) {}
MacPrivacyCursor::~MacPrivacyCursor() { Restore(); }
bool MacPrivacyCursor::Supported() {
  return Api().connection && Api().copy && Api().set;
}
bool MacPrivacyCursor::Prepare() {
  if (impl_->panel) return true;
  if (!Supported()) return false;
  // A cursor sprite below the local cover is captured only for clients that
  // request an embedded cursor. Desktop clients exclude it and use metadata.
  impl_->panel = [[CrossDeskPrivacyCursorPanel alloc] initWithContentRect:NSMakeRect(0, 0, 32, 32)
      styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
      backing:NSBackingStoreBuffered defer:NO];
  auto* panel = impl_->panel;
  panel.releasedWhenClosed = NO;
  panel.hidesOnDeactivate = NO;
  panel.opaque = NO;
  panel.backgroundColor = NSColor.clearColor;
  panel.hasShadow = NO;
  panel.ignoresMouseEvents = YES;
  panel.level = CGWindowLevelForKey(kCGScreenSaverWindowLevelKey);
  panel.animationBehavior = NSWindowAnimationBehaviorNone;
  panel.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
      NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorStationary |
      NSWindowCollectionBehaviorIgnoresCycle;
  panel.sharingType = NSWindowSharingReadOnly;
  impl_->view = [[NSImageView alloc] initWithFrame:panel.contentView.bounds];
  impl_->view.imageScaling = NSImageScaleNone;
  impl_->view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  panel.contentView = impl_->view;
  panel.alphaValue = 0;
  impl_->UpdateImage();
  [panel orderFrontRegardless];
  return panel != nil && panel.windowNumber > 0;
}
bool MacPrivacyCursor::Hide() {
  if (impl_->timer) return true;
  if (!Prepare()) return false;
  impl_->connection = Api().connection();
  impl_->previous_background = false;
  CFTypeRef previous = nullptr;
  if (Api().copy(impl_->connection, impl_->connection, kBackgroundCursor, &previous) == kCGErrorSuccess && previous)
    impl_->previous_background = CFEqual(previous, kCFBooleanTrue);
  if (previous) CFRelease(previous);
  // This connection-scoped WindowServer property is also used by Deskflow.
  // Resolve it dynamically: an unavailable API disables optional privacy only.
  if (Api().set(impl_->connection, impl_->connection, kBackgroundCursor, kCFBooleanTrue) != kCGErrorSuccess)
    return false;
  impl_->background_changed = true;
  impl_->healthy = true;
  impl_->visible_ticks = 0;
  hidden_by_privacy = true;
  if (!impl_->KeepHidden()) { Restore(); return false; }
  impl_->UpdateImage();
  impl_->panel.alphaValue = 1;
  auto* state = impl_.get();
  impl_->timer = [NSTimer timerWithTimeInterval:1.0 / 60 repeats:YES block:^(NSTimer*) {
    if (state->healthy) state->healthy = state->KeepHidden();
    state->UpdateImage();
  }];
  [[NSRunLoop mainRunLoop] addTimer:impl_->timer forMode:NSRunLoopCommonModes];
  return true;
}
bool MacPrivacyCursor::Healthy() const { return impl_->timer && impl_->healthy; }
uint32_t MacPrivacyCursor::WindowId() const {
  return impl_->panel ? static_cast<uint32_t>(impl_->panel.windowNumber) : 0;
}
void MacPrivacyCursor::Restore() {
  [impl_->timer invalidate];
  impl_->timer = nil;
  [impl_->panel orderOut:nil];
  [impl_->panel close];
  impl_->panel = nil;
  impl_->view = nil;
  // Balance only our own successful hide calls, including re-hides after
  // WindowServer changes cursor ownership when a remote click activates an app.
  while (impl_->hide_count) {
    CGDisplayShowCursor(kCGDirectMainDisplay);
    --impl_->hide_count;
  }
  hidden_by_privacy = false;
  if (impl_->background_changed) {
    Api().set(impl_->connection, impl_->connection, kBackgroundCursor,
              impl_->previous_background ? kCFBooleanTrue : kCFBooleanFalse);
    impl_->background_changed = false;
  }
}
bool IsMacPrivacyCursorHidden() { return hidden_by_privacy.load(); }
}
