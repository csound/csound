/* 
 
 SimpleTest1ViewController.m:
 
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

#import "SimpleTest1ViewController.h"

@interface SimpleTest1ViewController() {
    IBOutlet UISlider *uiSlider;
    IBOutlet UISwitch *uiSwitch;
    IBOutlet UILabel *uiLabel;
}
@end

@implementation SimpleTest1ViewController

-(void)viewDidLoad {
    self.title = @"Simple Test 1";
    [super viewDidLoad];
}

-(IBAction) toggleOnOff:(id)sender {
	NSLog(@"Status: %d", [uiSwitch isOn]);
    
	if(uiSwitch.on) {
        
        NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"csd"];
        NSLog(@"FILE PATH: %@", csdFile);
        
		[self.csound stop];
        
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        [csoundUI addLabel:uiLabel forChannelName:@"slider"];
        [csoundUI addSlider:uiSlider forChannelName:@"slider"];
        
        [self.csound play:csdFile];
        
	} else {
        [self.csound stop];
    }
}

#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[uiSwitch setOn:NO animated:YES];
    uiLabel.text = @"";
}


@end
