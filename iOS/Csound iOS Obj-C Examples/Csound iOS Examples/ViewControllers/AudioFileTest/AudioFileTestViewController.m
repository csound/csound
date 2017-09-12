/* 
 
 AudioFileTestViewController.m:
 
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

#import "AudioFileTestViewController.h"

@implementation AudioFileTestViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self setTitle:@"10. Soundfile: Pitch Shifter"];
    self.csound = [[CsoundObj alloc] init];
    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"audiofiletest" ofType:@"csd"];
    [self.pitchKnob setMinimumValue:0.5f];
    [self.pitchKnob setMaximumValue:2.0f];
    [self.pitchKnob setValue:1.0f];
    [self.csound addBinding:self.pitchKnob];
    [self.csound play:csdPath];
}

- (IBAction)play:(UIButton *)sender
{
	NSString *audioFilePath = [[NSBundle mainBundle] pathForResource:@"testAudioFile"
															  ofType:@"aif"];
	NSString *score = [NSString stringWithFormat:@"i1 0 1 \"%@\"", audioFilePath];
    [self.csound sendScore:score];
}

- (IBAction)changePitch:(ControlKnob *)sender
{
	[self.pitchLabel setText:[NSString stringWithFormat:@"%.2f", [sender value]]];
}


- (IBAction)stop:(UIButton *)sender {
    [self.csound sendScore:@"i3 0 1 2"];
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 140)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Soundfile PitchShifter uses the URL of a bundled AIFF file and playing it with Csound. Also demonstrated is a custom UI control knob widget, used to change playback pitch.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    [self.csound stop];
}


@end
