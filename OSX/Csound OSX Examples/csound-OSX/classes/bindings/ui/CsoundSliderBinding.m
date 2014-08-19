/*
 
 CsoundSliderBinding.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 
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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

#import "CsoundSliderBinding.h"

@interface CsoundSliderBinding() {
    MYFLT channelValue;
    MYFLT *channelPtr;
}

@property (nonatomic, strong) NSString *channelName;
@property (nonatomic, strong) NSSlider *slider;

@end

@implementation CsoundSliderBinding

-(instancetype)initSlider:(NSSlider *)slider channelName:(NSString *)channelName
{
    if (self = [super init]) {
        self.slider = slider;
        self.channelName = channelName;
    }
    return self;
}

-(void)updateChannelValue:(id)sender {
    channelValue = ((NSSlider *)sender).doubleValue;
}

-(void)setup:(CsoundObj *)csoundObj
{
    channelPtr = [csoundObj getInputChannelPtr:self.channelName
                                   channelType:CSOUND_CONTROL_CHANNEL];
    channelValue = self.slider.doubleValue;
    [self.slider setTarget:self];
    [self.slider setAction:@selector(updateChannelValue:)];
}

-(void)updateValuesToCsound {
    *channelPtr = channelValue;
}

-(void)cleanup {
    [self.slider setTarget:nil];
    [self.slider setAction:nil];
}


@end
