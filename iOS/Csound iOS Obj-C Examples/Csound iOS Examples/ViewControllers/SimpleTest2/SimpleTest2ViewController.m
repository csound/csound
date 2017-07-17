/* 
 
 SimpleTest2ViewController.h:
 
 Copyright (C) 2014 Steven Yi, Victor Lazzarini, Aurelius Prochazka
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

#import "SimpleTest2ViewController.h"

@interface SimpleTest2ViewController() {
    IBOutlet UISwitch *onOffSwitch;
    
    IBOutlet UISlider *rateSlider;
    IBOutlet UISlider *durationSlider;
    IBOutlet UILabel *rateLabel;
    IBOutlet UILabel *durationLabel;
    
    IBOutlet UISlider *attackSlider;
    IBOutlet UISlider *decaySlider;
    IBOutlet UISlider *sustainSlider;
    IBOutlet UISlider *releaseSlider;
    
    IBOutlet UILabel *attackLabel;
    IBOutlet UILabel *decayLabel;
    IBOutlet UILabel *sustainLabel;
    IBOutlet UILabel *releaseLabel;
    
}

@end

@implementation SimpleTest2ViewController

-(void)viewDidLoad {
    self.title = @"02. Simple Test 2";
    self.csound = NULL;
    [super viewDidLoad];
}

-(IBAction) toggleOnOff:(id)component {
	UISwitch *uiswitch = (UISwitch *)component;
	NSLog(@"Status: %d", [uiswitch isOn]);
    
	if(uiswitch.on) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"test2" ofType:@"csd"];  
        NSLog(@"FILE PATH: %@", tempFile);
        
		//[self.csound stop];
        if(self.csound == NULL){
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        [csoundUI addSlider:rateSlider     forChannelName:@"noteRate"];
        [csoundUI addSlider:durationSlider forChannelName:@"duration"];
        [csoundUI addSlider:attackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:decaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:sustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:releaseSlider  forChannelName:@"release"];
        
        [csoundUI addLabel:rateLabel     forChannelName:@"noteRate"];
        [csoundUI addLabel:durationLabel forChannelName:@"duration"];
        [csoundUI addLabel:attackLabel   forChannelName:@"attack"];
        [csoundUI addLabel:decayLabel    forChannelName:@"decay"];
        [csoundUI addLabel:sustainLabel  forChannelName:@"sustain"];
        [csoundUI addLabel:releaseLabel  forChannelName:@"release"];
            
        [csoundUI setLabelPrecision:2];
        
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
    infoVC.definesPresentationContext = YES;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 140)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"A generative music example that contains a number of sliders that affect the rate, duration, and envelope of each note.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[onOffSwitch setOn:NO animated:YES];
}
@end
