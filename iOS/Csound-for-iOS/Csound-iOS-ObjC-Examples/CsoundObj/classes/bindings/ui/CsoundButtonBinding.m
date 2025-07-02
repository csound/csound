/* 
 
 CsoundButtonBinding.m:
 
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

#import "CsoundButtonBinding.h"

@interface CsoundButtonBinding () {
    float channelValue;
    float *channelPtr;
}

@property (nonatomic, strong) NSString *channelName;
@property (nonatomic, strong) UIButton *button;
@end

@implementation CsoundButtonBinding

-(instancetype)initButton:(UIButton *)button channelName:(NSString *)channelName
{
    if (self = [super init]) {
        self.channelName = channelName;
        self.button = button;
    }
    return self;
}

-(void)updateChannelValueButtonIsDown:(id)sender {
    channelValue = 1;
}

-(void)updateChannelValueButtonIsUp:(id)sender {
    channelValue = 0;
}

-(void)setup:(CsoundObj *)csoundObj
{
    channelValue = self.button.selected ? 1 : 0;
    channelPtr = [csoundObj getInputChannelPtr:self.channelName
                                   channelType:CSOUND_CONTROL_CHANNEL];
    [self.button addTarget:self
                    action:@selector(updateChannelValueButtonIsDown:)
          forControlEvents:UIControlEventTouchDown];
    [self.button addTarget:self
                    action:@selector(updateChannelValueButtonIsUp:)
          forControlEvents:UIControlEventTouchUpInside];
    [self.button addTarget:self
                    action:@selector(updateChannelValueButtonIsUp:)
          forControlEvents:UIControlEventTouchUpOutside];
}


-(void)updateValuesToCsound
{
    *channelPtr = channelValue;
}

-(void)cleanup
{
    [self.button removeTarget:self
                       action:@selector(updateChannelValueButtonIsDown:)
             forControlEvents:UIControlEventTouchDown];
    [self.button removeTarget:self
                       action:@selector(updateChannelValueButtonIsUp:)
             forControlEvents:UIControlEventTouchUpInside];
    [self.button removeTarget:self
                       action:@selector(updateChannelValueButtonIsUp:)
             forControlEvents:UIControlEventTouchUpOutside];
}


@end
