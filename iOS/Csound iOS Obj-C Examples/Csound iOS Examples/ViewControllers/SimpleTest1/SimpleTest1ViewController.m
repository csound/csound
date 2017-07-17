/* 
 
 SimpleTest1ViewController.m:
 
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

#import "SimpleTest1ViewController.h"

@interface SimpleTest1ViewController() {
    IBOutlet UISlider *uiSlider;
    IBOutlet UISwitch *uiSwitch;
    IBOutlet UILabel *uiLabel;
}
@end

@implementation SimpleTest1ViewController

-(void)viewDidLoad {
    self.title = @"01. Simple Test 1";
    self.csound = NULL;
    [super viewDidLoad];
}

-(IBAction) toggleOnOff:(id)sender {
	NSLog(@"Status: %d", [uiSwitch isOn]);
    
	if(uiSwitch.on) {
        
        NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"csd"];
        NSLog(@"FILE PATH: %@", csdFile);
        
		//[self.csound stop];
        if(self.csound == NULL){
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
        [csoundUI setLabelPrecision:2];
        [csoundUI addLabel:uiLabel forChannelName:@"slider"];
        [csoundUI addSlider:uiSlider forChannelName:@"slider"];
        
        [self.csound play:csdFile];
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
    [infoVC setPreferredContentSize:CGSizeMake(200, 100)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Flip the switch to begin rendering Csound. Use the slider to control pitch.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

#pragma mark CsoundObjListener

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[uiSwitch setOn:NO animated:YES];
    [uiLabel performSelectorOnMainThread:@selector(setText:) withObject:@"" waitUntilDone:NO];
}


@end
