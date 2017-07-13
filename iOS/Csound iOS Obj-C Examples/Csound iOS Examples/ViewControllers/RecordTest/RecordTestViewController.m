/* 
 
 RecordTestViewController.m:
 
 Copyright (C) 2011 Thomas Hass
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

#import "RecordTestViewController.h"

@implementation RecordTestViewController

-(void)viewDidLoad {
    self.title = @"13. Mic: Recording";
    [super viewDidLoad];
	self.mPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[self recordingURL] error:nil];
	[self.mPlayer setDelegate:self];
}

-(IBAction) toggleOnOff:(id)component {
	UISwitch *uiswitch = (UISwitch *)component;
	NSLog(@"Status: %d", [uiswitch isOn]);
    
	if(uiswitch.on) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"recordTest" ofType:@"csd"];  
        
		[self.csound stop];
        self.csound = [[CsoundObj alloc] init];
        self.csound.useAudioInput = YES;
        [self.csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
		[csoundUI addSlider:_mGainSlider forChannelName:@"gain"];
        
		[_mLevelMeter addToCsoundObj:self.csound forChannelName:@"meter"];
		[self.csound play:tempFile];
	} else {
		[self.csound stopRecording];
        [self.csound stop];
    }
}

- (IBAction)changeGain:(UISlider *)sender
{
	[_mGainLabel setText:[NSString stringWithFormat:@"%.2f", [sender value]]];
}

- (IBAction)play:(UIButton *)sender
{
	[_mPlayer play];
	[sender removeTarget:self action:@selector(play:) forControlEvents:UIControlEventTouchUpInside];
	[sender addTarget:self action:@selector(stop:) forControlEvents:UIControlEventTouchUpInside];
	[sender setTitle:@"Stop" forState:UIControlStateNormal];
}

- (IBAction)stop:(UIButton *)sender
{
	[_mPlayer stop];
	[_mPlayer setCurrentTime:0];
	[sender removeTarget:self action:@selector(stop:) forControlEvents:UIControlEventTouchUpInside];
	[sender addTarget:self action:@selector(play:) forControlEvents:UIControlEventTouchUpInside];
	[sender setTitle:@"Play" forState:UIControlStateNormal];
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 120)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"This example uses a custom level-meter widget, and demonstrates efficient use of concurrency for an audio application.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
	[_mPlayer setCurrentTime:0];
	[_mPlayButton removeTarget:self action:@selector(stop:) forControlEvents:UIControlEventTouchUpInside];
	[_mPlayButton addTarget:self action:@selector(play:) forControlEvents:UIControlEventTouchUpInside];
	[_mPlayButton setTitle:@"Play" forState:UIControlStateNormal];
}


- (NSURL *)recordingURL
{
    NSURL *localDocDirURL = nil;
    if (localDocDirURL == nil) {
        NSString *docDirPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) 
                                objectAtIndex:0];
        localDocDirURL = [NSURL fileURLWithPath:docDirPath];
    }
    return [localDocDirURL URLByAppendingPathComponent:@"recording.wav"];
}

#pragma mark CsoundObjListener

-(void)csoundObjStarted:(CsoundObj *)csoundObj {
	[self.csound recordToURL:[self recordingURL]];
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
	[_mSwitch setOn:NO animated:YES];
    _mPlayer = nil;
	_mPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[self recordingURL] error:nil];
	[_mPlayer setDelegate:self];
}

@end
