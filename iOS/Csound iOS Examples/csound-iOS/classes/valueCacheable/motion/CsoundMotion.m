/*
 
 CsoundMotion.m:
 
 Copyright (C) 2011 Steven Yi, Victor Lazzarini, Aurelius Prochazka
 
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

#import "CachedAccelerometer.h"
#import "CachedGyroscope.h"
#import "CachedAttitude.h"

@interface CsoundMotion () {
    CMMotionManager* mMotionManager;
    CsoundObj *csoundObj;
}
@end

@implementation CsoundMotion

- (instancetype)initWithCsoundObj:(CsoundObj *)csound {
    self = [super init];
    if (self) {
        csoundObj = csound;
        mMotionManager = [[CMMotionManager alloc] init];
        [csoundObj addCompletionListener:self];
    }
    return self;
}

- (void)enableAccelerometer {
    
    if (!mMotionManager.accelerometerAvailable) {
        NSLog(@"Accelerometer not available");
    }
    
    CachedAccelerometer* accelerometer = [[CachedAccelerometer alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:accelerometer];
    
    
    mMotionManager.accelerometerUpdateInterval = 1 / 100.0; // 100 hz
    
    [mMotionManager startAccelerometerUpdates];
}

- (void)enableGyroscope {
    
    if (!mMotionManager.isGyroAvailable) {
        NSLog(@"Gyroscope not available");
    }
    
    CachedGyroscope* gyro = [[CachedGyroscope alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:gyro];
    
    mMotionManager.gyroUpdateInterval = 1 / 100.0; // 100 hz
    
    [mMotionManager startGyroUpdates];
}

- (void)enableAttitude {
    if (!mMotionManager.isDeviceMotionAvailable) {
        NSLog(@"Attitude not available");
    }
    
    CachedAttitude* attitude = [[CachedAttitude alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:attitude];
    
    mMotionManager.deviceMotionUpdateInterval = 1 / 100.0; // 100hz
    
    [mMotionManager startDeviceMotionUpdates];
}

#pragma mark CsoundObjListener

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    [mMotionManager stopAccelerometerUpdates];
    [mMotionManager stopGyroUpdates];
    [mMotionManager stopDeviceMotionUpdates];
}

@end
