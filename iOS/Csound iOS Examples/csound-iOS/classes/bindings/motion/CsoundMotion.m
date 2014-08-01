/*
 
 CsoundMotion.m:
 
 Copyright (C) 2014 Steven Yi, Victor Lazzarini, Aurelius Prochazka
 
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

#import "CsoundMotion.h"

#import <CoreMotion/CoreMotion.h>

#import "CsoundAccelerometerBinding.h"
#import "CsoundAttitudeBinding.h"
#import "CsoundGyroscopeBinding.h"

@interface CsoundMotion () {
    CMMotionManager* motionManager;
    CsoundObj *csoundObj;
}
@end

@implementation CsoundMotion

- (instancetype)initWithCsoundObj:(CsoundObj *)csound {
    self = [super init];
    if (self) {
        csoundObj = csound;
        motionManager = [[CMMotionManager alloc] init];
        [csoundObj addListener:self];
    }
    return self;
}

- (void)enableAccelerometer {
    
    if (!motionManager.accelerometerAvailable) {
        NSLog(@"Accelerometer not available");
    }
    
    CsoundAccelerometerBinding  *accelerometer = [[CsoundAccelerometerBinding alloc] init:motionManager];
    [csoundObj addDataBinding:accelerometer];
    
    
    motionManager.accelerometerUpdateInterval = 1 / 100.0; // 100 hz
    
    [motionManager startAccelerometerUpdates];
}

- (void)enableGyroscope {
    
    if (!motionManager.isGyroAvailable) {
        NSLog(@"Gyroscope not available");
    }
    
    CsoundGyroscopeBinding *gyro = [[CsoundGyroscopeBinding alloc] init:motionManager];
    [csoundObj addDataBinding:gyro];
    
    motionManager.gyroUpdateInterval = 1 / 100.0; // 100 hz
    
    [motionManager startGyroUpdates];
}

- (void)enableAttitude {
    if (!motionManager.isDeviceMotionAvailable) {
        NSLog(@"Attitude not available");
    }
    
    CsoundAttitudeBinding *attitude = [[CsoundAttitudeBinding alloc] init:motionManager];
    [csoundObj addDataBinding:attitude];
    
    motionManager.deviceMotionUpdateInterval = 1 / 100.0; // 100hz
    
    [motionManager startDeviceMotionUpdates];
}

#pragma mark CsoundObjListener

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    [motionManager stopAccelerometerUpdates];
    [motionManager stopGyroUpdates];
    [motionManager stopDeviceMotionUpdates];
}

@end
