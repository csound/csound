/*
 
 AudioInTestWindowController.m:
 
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

#import "AudioInTestWindowController.h"
#import "CsoundObj.h"
#import "CsoundUI.h"

@interface AudioInTestWindowController () <CsoundObjListener> {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *startStopButton;
@property (strong) IBOutlet NSSlider *leftDelaySlider;
@property (strong) IBOutlet NSSlider *leftFeedbackSlider;
@property (strong) IBOutlet NSSlider *rightDelaySlider;
@property (strong) IBOutlet NSSlider *rightFeedbackSlider;

@end

@implementation AudioInTestWindowController


- (IBAction)toggleStartStop:(id)sender {
    if([self.startStopButton.title isEqualToString:@"Start"]) {
        
        NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"audioInTest" ofType:@"csd"];
        
        csound = [[CsoundObj alloc] init];
        csound.useAudioInput = YES;
        [csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        
        [csoundUI addSlider:self.leftDelaySlider     forChannelName:@"leftDelayTime"];
        [csoundUI addSlider:self.leftFeedbackSlider  forChannelName:@"leftFeedback"];
        [csoundUI addSlider:self.rightDelaySlider    forChannelName:@"rightDelayTime"];
        [csoundUI addSlider:self.rightFeedbackSlider forChannelName:@"rightFeedback"];
        
        [csound play:csdFile];
        
	} else {
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
