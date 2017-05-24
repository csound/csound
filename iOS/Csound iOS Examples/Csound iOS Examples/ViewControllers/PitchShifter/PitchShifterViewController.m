/* 
 
 PitchShifterViewController.m:
 
 Copyright (C) 2014 Thomas Hass, Aurelius Prochazka
 Updated in 2017 by Dr. Richard Boulanger, Nikhil Singh
 
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
		
		[self.csound addBinding:mXYControl];
		
		[self.csound play:tempFile];
	} else {
		[self.csound stop];
	}
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 150)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"This example uses Csound's 'pvs' real-time spectral-processing opcodes to perform pitch-shifting on a live microphone input signal, controlled with a custom XY control pad.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[mSwitch setOn:NO animated:YES];
}

#pragma mark - Lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.title = @"16. Mic: XY PitchShift+Mix";
	
	[mXYControl setXValue:1.0f];
	[mXYControl setYValue:0.5f];
}


@end
