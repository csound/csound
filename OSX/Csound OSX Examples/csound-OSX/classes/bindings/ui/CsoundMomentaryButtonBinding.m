/*
 
 CsoundMomentaryButtonBinding.m:
 
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
#import "CsoundMomentaryButtonBinding.h"

@interface CsoundMomentaryButtonBinding () {
    MYFLT channelValue;
    MYFLT *channelPtr;
}

@property (nonatomic, strong) NSString *channelName;
@property (nonatomic, strong) NSButton *button;
@end

@implementation CsoundMomentaryButtonBinding

-(instancetype)initButton:(NSButton *)button channelName:(NSString *)channelName
{
    if (self = [super init]) {
        self.channelName = channelName;
        self.button = button;
    }
    return self;
}

-(void)updateChannelValue:(id)sender {
    channelValue = 1;
}

-(void)setup:(CsoundObj *)csoundObj
{
    channelValue = self.button.state;
    channelPtr = [csoundObj getInputChannelPtr:self.channelName
                                   channelType:CSOUND_CONTROL_CHANNEL];
    [self.button setTarget:self];
    [self.button setAction:@selector(updateChannelValue:)];
}

-(void)updateValuesToCsound
{
    *channelPtr = channelValue;
    channelValue = 0;
}

-(void)cleanup {
    [self.button setTarget:nil];
    [self.button setAction:nil];
}


@end
