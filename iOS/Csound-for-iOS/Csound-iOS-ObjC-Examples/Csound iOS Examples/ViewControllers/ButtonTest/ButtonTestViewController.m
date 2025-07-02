/* 
 
 ButtonTestViewController.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
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
#import "ButtonTestViewController.h"

@implementation ButtonTestViewController

-(void)viewDidLoad {
    self.title = @"03. Button Test";
    self.csound = NULL;
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
        
		//[self.csound stop];
        if(self.csound == NULL){
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        
        [csoundUI addMomentaryButton:mValueButton forChannelName:@"button1"];
        
        [csoundUI addSlider:mDurationSlider forChannelName:@"duration"];
        [csoundUI addSlider:mAttackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:mDecaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:mSustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:mReleaseSlider  forChannelName:@"release"];
        
        [self.csound play:tempFile];
        }
        
	} else {
        [self.csound stop];
        self.csound = NULL;
    }
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 160)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Uses a .csd based on SimpleTest 2, but depends on the user to press a button to trigger each note. One button uses a binding and the other sends a score message to CsoundObj.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[mSwitch setOn:NO animated:YES];
}

@end
