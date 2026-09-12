/* Copyright (c) 2026 by DI JUNKUN, All Rights Reserved. */

#include "privacy_cover_view.h"

#include <algorithm>
#include <cmath>

namespace {
NSImage* LoadPrivacyLogo() {
  // The macOS installer currently packages the icon as "crossedesk.icns".
  for (NSString* name in @[@"crossdesk", @"crossedesk"]) {
    NSString* path = [NSBundle.mainBundle pathForResource:name ofType:@"icns"];
    if (path) {
      NSImage* image = [[NSImage alloc] initWithContentsOfFile:path];
      if (image.valid) return image;
    }
  }
  // Support unbundled development builds, including launches from outside
  // the repository. No source-tree path is embedded in the executable.
  NSString* resource = @"apps/desktop/resources/macos/crossdesk.icns";
  NSString* directory = NSBundle.mainBundle.executablePath.stringByDeletingLastPathComponent;
  NSMutableArray<NSString*>* paths = [NSMutableArray arrayWithObject:
      [NSFileManager.defaultManager.currentDirectoryPath stringByAppendingPathComponent:resource]];
  for (int depth = 0; directory.length && depth <= 4; ++depth) {
    [paths addObject:[directory stringByAppendingPathComponent:resource]];
    directory = directory.stringByDeletingLastPathComponent;
  }
  for (NSString* path in paths) {
    NSImage* image = [[NSImage alloc] initWithContentsOfFile:path.stringByStandardizingPath];
    if (image.valid) return image;
  }
  return nil;
}

NSAttributedString* Text(NSString* value, CGFloat size, NSFontWeight weight, NSColor* color) {
  NSMutableParagraphStyle* paragraph = [[NSMutableParagraphStyle alloc] init];
  paragraph.lineBreakMode = NSLineBreakByWordWrapping;
  return [[NSAttributedString alloc] initWithString:value attributes:@{
      NSFontAttributeName: [NSFont systemFontOfSize:size weight:weight],
      NSForegroundColorAttributeName: color,
      NSParagraphStyleAttributeName: paragraph}];
}
constexpr NSStringDrawingOptions kTextOptions =
    NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading;

CGFloat TextHeight(NSAttributedString* text, CGFloat width) {
  return std::ceil([text boundingRectWithSize:NSMakeSize(width, CGFLOAT_MAX)
                                    options:kTextOptions].size.height);
}
}

@interface CrossDeskPrivacyCoverView : NSView
@property(nonatomic, strong) NSImage* logo;
@property(nonatomic, copy) NSString* unlockHint;
@end

@implementation CrossDeskPrivacyCoverView
- (BOOL)isOpaque { return YES; }
- (BOOL)isFlipped { return YES; }
- (void)drawRect:(NSRect)dirtyRect {
  [NSColor.blackColor setFill];
  NSRectFill(self.bounds);
  const CGFloat width = NSWidth(self.bounds), height = NSHeight(self.bounds);
  if (width <= 0 || height <= 0) return;
  // Match the Windows cover's sizes, colors and bottom-left alignment.
  // AppKit coordinates are points; Retina backing scale is applied by AppKit.
  const CGFloat scale = std::min({CGFloat(1), width / 640, height / 200});
  const CGFloat margin = 32 * scale, text_width = 398 * scale;
  NSColor* foreground = [NSColor colorWithSRGBRed:245.0 / 255 green:247.0 / 255
                                           blue:250.0 / 255 alpha:1];
  NSColor* muted = [NSColor colorWithSRGBRed:181.0 / 255 green:190.0 / 255
                                      blue:204.0 / 255 alpha:1];
  NSAttributedString* brand = Text(@"CrossDesk", 14 * scale, NSFontWeightSemibold, foreground);
  NSAttributedString* hint = Text(self.unlockHint, 16 * scale, NSFontWeightRegular, muted);
  NSAttributedString* shortcut = Text(@"Control + Option + Shift + Esc", 24 * scale,
                                      NSFontWeightSemibold, foreground);
  const CGFloat logo_size = std::ceil(brand.size.width);
  const CGFloat brand_height = TextHeight(brand, logo_size);
  const CGFloat hint_height = TextHeight(hint, text_width);
  const CGFloat shortcut_height = TextHeight(shortcut, text_width);
  const CGFloat bottom = height - margin;
  const CGFloat logo_top = bottom - brand_height - 6 * scale - logo_size;
  [self.logo drawInRect:NSMakeRect(margin, logo_top, logo_size, logo_size)
              fromRect:NSZeroRect operation:NSCompositingOperationSourceOver
              fraction:1 respectFlipped:YES hints:@{NSImageHintInterpolation: @(NSImageInterpolationHigh)}];
  [brand drawWithRect:NSMakeRect(margin, bottom - brand_height, logo_size, brand_height)
             options:kTextOptions];
  const CGFloat text_left = margin + logo_size + 24 * scale;
  [hint drawWithRect:NSMakeRect(text_left, bottom - shortcut_height - 12 * scale - hint_height,
                               text_width, hint_height) options:kTextOptions];
  [shortcut drawWithRect:NSMakeRect(text_left, bottom - shortcut_height,
                                   text_width, shortcut_height) options:kTextOptions];
}
@end

namespace crossdesk {
NSView* CreateMacPrivacyCoverView(NSRect frame, NSString* unlock_hint) {
  NSImage* logo = LoadPrivacyLogo();
  if (!logo) return nil;
  auto* view = [[CrossDeskPrivacyCoverView alloc] initWithFrame:frame];
  view.logo = logo;
  view.unlockHint = unlock_hint ?: @"To turn off the privacy screen, press";
  view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
  return view;
}
}
