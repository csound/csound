/*
 
 SimpleTest1WindowController.m:
 
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

#import "SimpleTest1WindowController.h"

#import "CsoundObj.h"
#import "CsoundUI.h"

@interface SimpleTest1WindowController() <CsoundObjListener> {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *toggleOnOffButton;
@property (strong) IBOutlet NSSlider *slider;
@end

@implementation SimpleTest1WindowController

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.toggleOnOffButton.title isEqualToString:@"Start"]) {
        
        NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"csd"];
        csound = [[CsoundObj alloc] init];
        [csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_slider forChannelName:@"slider"];
        
        [csound play:csdFile];
        
	} else {
        [csound stop];
    }
}

#pragma mark CsoundObjListener

-(void)csoundObjStarted:(CsoundObj *)csoundObj {
    self.toggleOnOffButton.title = @"Stop";
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
    NSLog(@"Csound finished");
	self.toggleOnOffButton.title = @"Start";
}

@end
