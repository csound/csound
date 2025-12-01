// ViewController.m: Generator app glue code
// Copyright (C) 2025 Victor Lazzarini.

// This file is part of Csound.

// The Csound Library is free software; you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// Csound is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with Csound; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA

#import "ViewController.h"
#import "Engine.h"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioServices.h>

@interface ViewController ()
- (void)setupSession;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    [self setupSession];
    
}

- (void)viewDidDisappear:(BOOL)animated {
    stop_csound();
}

- (IBAction) volumeSlider:(id)sender {
    UISlider * uiSlider = sender;
    NSLog(@"Slider1: %f", uiSlider.value);
    set_volume(uiSlider.value);
}

- (IBAction) pitchSlider:(id)sender {
    UISlider * uiSlider = sender;
    NSLog(@"Slider2: %f", uiSlider.value);
    set_octave(uiSlider.value);
}

- (IBAction) togglePlay:(id)sender {
    UISwitch * uiSwitch = sender;
    NSLog(@"AppOn: %d", [uiSwitch isOn]);
    if([uiSwitch isOn])
       start_csound();
     else stop_csound();
    
}

- (void)setupSession {
    @autoreleasepool {
        NSError *error;
        BOOL success;
        AVAudioSession *session = [AVAudioSession sharedInstance];
        success = [session
                   setCategory:AVAudioSessionCategoryPlayAndRecord
                   withOptions:(AVAudioSessionCategoryOptionMixWithOthers |
                                AVAudioSessionCategoryOptionDefaultToSpeaker)
                   error:&error];
        success = [session setActive:YES error:&error];
    }
}

@end
