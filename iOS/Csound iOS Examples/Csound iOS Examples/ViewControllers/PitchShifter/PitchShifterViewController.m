/* 
 
 PitchShifterViewController.m:
 
 Copyright (C) 2014 Thomas Hass, Aurelius Prochazka
 
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

#import "PitchShifterViewController.h"

@implementation PitchShifterViewController

- (IBAction)toggleOnOff:(UISwitch *)sender
{
	if (sender.on) {
		NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"pitchshifter" ofType:@"csd"];
		
		[self.csound stop];
		self.csound = [[CsoundObj alloc] init];
        self.csound.useAudioInput = YES;
		
		[self.csound addValueCacheable:mXYControl];
		
		[self.csound play:tempFile];
	} else {
		[self.csound stop];
	}
}

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[mSwitch setOn:NO animated:YES];
}

#pragma mark - Lifecycle

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self) {
		
	}
	return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.title = @"Pitch Shifter";
	
	[mXYControl setXValue:1.0f];
	[mXYControl setYValue:0.5f];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    if (interfaceOrientation == UIInterfaceOrientationLandscapeLeft ||
        interfaceOrientation == UIInterfaceOrientationLandscapeRight) {
        return YES;
    } else {
        return NO;
    }
}


@end
