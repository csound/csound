/*

 CsoundBindingDispatch.h:

 Copyright (C) 2026 Dan Moore

 This file is part of Csound for iOS.

 The Csound for iOS Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

 */

#import <Foundation/Foundation.h>

/// Bindings' setup/cleanup must touch UIKit on the main thread, but they can be
/// invoked either from a background thread (CsoundObj's audio setup path) or,
/// via CsoundObj addBinding:, directly from the main thread. dispatch_sync-ing
/// to the main queue from the main thread deadlocks, so run inline when we're
/// already there.
///
/// Rule: Code in `block` must never wait on the Csound thread, else deadlock.
/// Calls to `getInputChannelPtr:channelType:`, `getOutputChannelPtr:channelType:`, etc. are okay
/// because they do not block within CsoundObj.
NS_INLINE void CsoundRunSyncOnMainQueue(NS_NOESCAPE dispatch_block_t block) {
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}
