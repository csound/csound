/*
 
 SimpleTest2WindowController.m:
 
 Copyright (C) 2014 Aurelius Prochazka
 
 This file is part of Csound OSX Examples.
 
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

#import "SimpleTest2WindowController.h"
#import "CsoundObj.h"
#import "CsoundUI.h"

@interface SimpleTest2WindowController() <CsoundObjListener> {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *startStopButton;
@property (strong) IBOutlet NSSlider *rateSlider;
@property (strong) IBOutlet NSSlider *durationSlider;
@property (strong) IBOutlet NSSlider *attackSlider;
@property (strong) IBOutlet NSSlider *decaySlider;
@property (strong) IBOutlet NSSlider *sustainSlider;
@property (strong) IBOutlet NSSlider *releaseSlider;
@end

@implementation SimpleTest2WindowController

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.startStopButton.title isEqualToString:@"Start"]) {
        
        NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"test2" ofType:@"csd"];
        
        csound = [[CsoundObj alloc] init];
        [csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_rateSlider     forChannelName:@"noteRate"];
        [csoundUI addSlider:_durationSlider forChannelName:@"duration"];
        [csoundUI addSlider:_attackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:_decaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:_sustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:_releaseSlider  forChannelName:@"release"];
        
        [csound play:csdFile];
        
	} else {
        NSLog(@"try to stop csound");
        [csound stop];
    }
}

#pragma mark CsoundObjListener

-(void)csoundObjStarted:(CsoundObj *)csoundObj {
    self.startStopButton.title = @"Stop";
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	self.startStopButton.title = @"Start";
}

@end
