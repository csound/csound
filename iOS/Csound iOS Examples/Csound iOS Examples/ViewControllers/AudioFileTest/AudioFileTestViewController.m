/* 
 
 AudioFileTestViewController.m:
 
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

#import "AudioFileTestViewController.h"

@implementation AudioFileTestViewController

- (IBAction)play:(UIButton *)sender
{
	NSString *audioFilePath = [[NSBundle mainBundle] pathForResource:@"testAudioFile"
															  ofType:@"aif"];
	NSString *score = [NSString stringWithFormat:@"i1 0 1 \"%@\"", audioFilePath];
    [self.csound sendScore:score];
}

- (IBAction)changePitch:(ControlKnob *)sender
{
	[_mPitchLabel setText:[NSString stringWithFormat:@"%.2f", [sender value]]];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.csound = [[CsoundObj alloc] init];
	NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"audiofiletest" ofType:@"csd"];
	[_mPitchKnob setMinimumValue:0.5f];
	[_mPitchKnob setMaximumValue:2.0f];
	[_mPitchKnob setValue:1.0f];
	[self.csound addValueCacheable:_mPitchKnob];
	[self.csound play:csdPath];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    [self.csound stop];
}


@end
