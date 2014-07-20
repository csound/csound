/* 
 
 HarmonizerTest.m:
 
 Copyright (C) 2011 Steven Yi
 
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


#import "HarmonizerTest.h"

@implementation HarmonizerTest

-(void)viewDidLoad {
    self.title = @"Harmonizer";
    [super viewDidLoad];
}

-(IBAction) toggleOnOff:(id)component {
	UISwitch *uiswitch = (UISwitch *)component;
	NSLog(@"Status: %d", [uiswitch isOn]);
    
	if(uiswitch.on) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"harmonizer" ofType:@"csd"];  
        NSLog(@"FILE PATH: %@", tempFile);
        
		[self.csound stop];
        
        self.csound = [[CsoundObj alloc] init];
        self.csound.useAudioInput = YES;
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        
        [csoundUI addSlider:mHarmPitchSlider forChannelName:@"slider"];
        [csoundUI addSlider:mGainSlider forChannelName:@"gain"];
        
        [self.csound play:tempFile];
        
	} else {
        [self.csound stop];
    }
}



#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[mSwitch setOn:NO animated:YES];
}

@end
