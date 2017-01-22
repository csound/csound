/* 
 
 HardwareTestViewController.m:
 
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

#import "HardwareTestViewController.h"

@implementation HardwareTestViewController

-(void)viewDidLoad {
    self.title = @"14. Hardware: Motion Control";
    [super viewDidLoad];
    
    motionManager = [[CMMotionManager alloc] init];
    
    [motionManager startAccelerometerUpdates];
    [motionManager startGyroUpdates];
    [motionManager startDeviceMotionUpdates];
    
    [motionManager setAccelerometerUpdateInterval:.1];
    [motionManager setGyroUpdateInterval:.1];
    [motionManager setDeviceMotionUpdateInterval:.1];
    
    [motionManager startAccelerometerUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:^(CMAccelerometerData *accData, NSError *error){
        if(error!= nil) NSLog(@"%@", [error localizedDescription]);
        
        accX.text = [NSString stringWithFormat:@"%.3f", accData.acceleration.x];
        accY.text = [NSString stringWithFormat:@"%.3f", accData.acceleration.y];
        accZ.text = [NSString stringWithFormat:@"%.3f", accData.acceleration.z];
    }];
    
    [motionManager startGyroUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:^(CMGyroData *gyroData, NSError *error){
        if(error!= nil) NSLog(@"%@", [error localizedDescription]);
        
        gyroX.text = [NSString stringWithFormat:@"%.3f", gyroData.rotationRate.x];
        gyroY.text = [NSString stringWithFormat:@"%.3f", gyroData.rotationRate.y];
        gyroZ.text = [NSString stringWithFormat:@"%.3f", gyroData.rotationRate.z];
    }];
    
    [motionManager startDeviceMotionUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:^(CMDeviceMotion *motionData, NSError *error) {
        if(error!= nil) NSLog(@"%@", [error localizedDescription]);
        
        roll.text = [NSString stringWithFormat:@"%.3f", motionData.attitude.roll];
        pitch.text = [NSString stringWithFormat:@"%.3f", motionData.attitude.pitch];
        yaw.text = [NSString stringWithFormat:@"%.3f", motionData.attitude.yaw];
    }];
}

-(IBAction) toggleOnOff:(id)component {
	UISwitch *uiswitch = (UISwitch *)component;
	NSLog(@"Status: %d", [uiswitch isOn]);
    
	if(uiswitch.on) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"hardwareTest"
                                                             ofType:@"csd"];
        NSLog(@"FILE PATH: %@", tempFile);
        
		[self.csound stop];
        
        self.csound = [[CsoundObj alloc] init];
        [self.csound addListener:self];
        
        CsoundMotion *csoundMotion = [[CsoundMotion alloc] initWithCsoundObj:self.csound];
        
        [csoundMotion enableAccelerometer];
        [csoundMotion enableAttitude];
        [csoundMotion enableGyroscope];
        
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
    [infoVC setPreferredContentSize:CGSizeMake(300, 220)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Hardware: Motion Control shows how to use the device's motion sensor data as a set of controllers for Csound, and also displays this data in a set of UILabels. Accelerometer X controls oscillator frequency, Attitude: Yaw controls filter cutoff, Attitude: Pitch controls amplitude, and Attitude: Roll controls filter resonance.";
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
