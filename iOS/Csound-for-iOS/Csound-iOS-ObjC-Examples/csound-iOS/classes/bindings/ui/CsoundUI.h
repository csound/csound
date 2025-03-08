/*
 
 CsoundUI.h:
 
 Copyright (C) 2014 Steven Yi, Victor Lazzarini, Aurelius Prochazka
 
 This file is part of Csound iOS Examples.
 
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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CsoundObj.h"

@interface CsoundUI : NSObject

- (instancetype)initWithCsoundObj:(CsoundObj *)csound;

- (void)addLabel:(UILabel *)uiLabel    forChannelName:(NSString *)channelName;
- (void)addButton:(UIButton *)uiButton forChannelName:(NSString *)channelName;
- (void)addSlider:(UISlider *)uiSlider forChannelName:(NSString *)channelName;
- (void)addSwitch:(UISwitch *)uiSwitch forChannelName:(NSString *)channelName;
- (void)addMomentaryButton:(UIButton *)uiButton forChannelName:(NSString *)channelName;

@property int labelPrecision;

@end
