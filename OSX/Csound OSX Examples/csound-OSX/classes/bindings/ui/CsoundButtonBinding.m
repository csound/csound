/*
 
 CsoundButtonBinding.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 
 
 This file is part of Csound for OSX.
 
 The Csound for OSX Library is free software; you can redistribute it
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

#import "CsoundButtonBinding.h"

@interface CsoundButtonBinding () {
    MYFLT channelValue;
    MYFLT *channelPtr;
}

@property (nonatomic, strong) NSString *channelName;
@property (nonatomic, strong) NSButton *button;
@end

@implementation CsoundButtonBinding

-(id)initButton:(NSButton *)button channelName:(NSString *)channelName
{
    if (self = [super init]) {
        self.channelName = channelName;
        self.button = button;
    }
    return self;
}

-(void)updateChannelValueButtonStateChanged:(id)sender {
    channelValue = self.button.state;
}

-(void)setup:(CsoundObj *)csoundObj
{
    channelValue = self.button.state;
    channelPtr = [csoundObj getInputChannelPtr:self.channelName
                                   channelType:CSOUND_CONTROL_CHANNEL];
    [self.button setTarget:self];
    [self.button setAction:@selector(updateChannelValueButtonStateChanged:)];
}

-(void)updateValuesToCsound
{
    *channelPtr = channelValue;
}

-(void)cleanup {
    [self.button setTarget:nil];
    [self.button setAction:nil];
}


@end
