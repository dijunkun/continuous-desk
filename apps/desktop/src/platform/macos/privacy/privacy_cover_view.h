/*
 * @Author: DI JUNKUN
 * @Date: 2026-09-13
 * Copyright (c) 2026 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _PRIVACY_COVER_VIEW_H_
#define _PRIVACY_COVER_VIEW_H_

#import <AppKit/AppKit.h>

namespace crossdesk {
// Returns nil if the CrossDesk icon resource cannot be loaded.
NSView* CreateMacPrivacyCoverView(NSRect frame, NSString* unlock_hint);
}

#endif