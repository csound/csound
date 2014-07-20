/* 
 
 ButtonTestViewController.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 
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
#import "ButtonTestViewController.h"

@implementation ButtonTestViewController

-(void)viewDidLoad {
    self.title = @"Button Test";
    [super viewDidLoad];
}

-(IBAction) eventButtonHit:(id)sender {
    NSString *score = [NSString stringWithFormat:@"i2 0 %f", [mDurationSlider value]];

    [self.csound sendScore:score];
}

-(IBAction) toggleOnOff:(id)component {
	UISwitch *uiswitch = (UISwitch *)component;
	NSLog(@"Status: %d", [uiswitch isOn]);
    
	if(uiswitch.on) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"buttonTest" ofType:@"csd"];  
        NSLog(@"FILE PATH: %@", tempFile);
        
		[self.csound stopCsound];
        
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        
        [csoundUI addButton:mValueButton forChannelName:@"button1"];
        
        [csoundUI addSlider:mDurationSlider forChannelName:@"duration"];
        [csoundUI addSlider:mAttackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:mDecaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:mSustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:mReleaseSlider  forChannelName:@"release"];
        
        [self.csound startCsound:tempFile];
        
	} else {
        [self.csound stopCsound];
    }
}

#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[mSwitch setOn:NO animated:YES];
}

@end
