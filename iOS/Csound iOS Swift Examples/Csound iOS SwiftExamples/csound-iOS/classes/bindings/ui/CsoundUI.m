/*
 
 CsoundUI.m:
 
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

#import "CsoundUI.h"

#import "CsoundButtonBinding.h"
#import "CsoundLabelBinding.h"
#import "CsoundMomentaryButtonBinding.h"
#import "CsoundSliderBinding.h"
#import "CsoundSwitchBinding.h"

@interface CsoundUI () {
    CsoundObj *csoundObj;
}
@end

@implementation CsoundUI

- (instancetype)initWithCsoundObj:(CsoundObj *)csound {
    self = [super init];
    if (self) {
        csoundObj = csound;
    }
    return self;
}

- (void)addLabel:(UILabel *)uiLabel
  forChannelName:(NSString *)channelName
{
    CsoundLabelBinding *labelBinding;
    labelBinding = [[CsoundLabelBinding alloc] initLabel:uiLabel
                                             channelName:channelName];
    if(!_labelPrecision) {
        _labelPrecision = 8;
    }
    
    labelBinding.precision = _labelPrecision;
    
    [csoundObj addBinding:labelBinding];
}


- (void)addButton:(UIButton *)uiButton forChannelName:(NSString *)channelName
{
    CsoundButtonBinding *buttonBinding;
    buttonBinding = [[CsoundButtonBinding alloc] initButton:uiButton
                                                channelName:channelName];
    [csoundObj addBinding:buttonBinding];
}

- (void)addSlider:(UISlider *)uiSlider forChannelName:(NSString *)channelName
{
    CsoundSliderBinding *sliderBinding;
    sliderBinding = [[CsoundSliderBinding alloc] initSlider:uiSlider
                                                channelName:channelName];
    [csoundObj addBinding:sliderBinding];
}

- (void)addSwitch:(UISwitch *)uiSwitch forChannelName:(NSString *)channelName
{
    CsoundSwitchBinding *switchBinding;
    switchBinding = [[CsoundSwitchBinding alloc] initSwitch:uiSwitch
                                                channelName:channelName];
    [csoundObj addBinding:switchBinding];
}

- (void)addMomentaryButton:(UIButton *)uiButton forChannelName:(NSString *)channelName
{
    CsoundMomentaryButtonBinding *momentaryButtonBinding;
    momentaryButtonBinding = [[CsoundMomentaryButtonBinding alloc] initButton:uiButton
                                                                  channelName:channelName];
    [csoundObj addBinding:momentaryButtonBinding];
}


@end
