//
//  CsoundMotion.m
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

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
    }
    return self;
}

-(id<CsoundValueCacheable>)enableAccelerometer {
    
    if (!mMotionManager.accelerometerAvailable) {
        NSLog(@"Accelerometer not available");
        return nil;
    }
    
    CachedAccelerometer* accelerometer = [[CachedAccelerometer alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:accelerometer];
    
    
    mMotionManager.accelerometerUpdateInterval = 1 / 100.0; // 100 hz
    
    [mMotionManager startAccelerometerUpdates];
	
	return accelerometer;
}

-(id<CsoundValueCacheable>)enableGyroscope {
    
    if (!mMotionManager.isGyroAvailable) {
        NSLog(@"Gyroscope not available");
        return nil;
    }
    
    CachedGyroscope* gyro = [[CachedGyroscope alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:gyro];
    
    mMotionManager.gyroUpdateInterval = 1 / 100.0; // 100 hz
    
    [mMotionManager startGyroUpdates];
	
	return gyro;
}

-(id<CsoundValueCacheable>)enableAttitude {
    if (!mMotionManager.isDeviceMotionAvailable) {
        NSLog(@"Attitude not available");
        return nil;
    }
    
    CachedAttitude* attitude = [[CachedAttitude alloc] init:mMotionManager];
    [csoundObj.valuesCache addObject:attitude];
    
    mMotionManager.deviceMotionUpdateInterval = 1 / 100.0; // 100hz
    
    [mMotionManager startDeviceMotionUpdates];
	
	return attitude;
}

-(void)csoundObjDidStart:(CsoundObj *)csoundObj {
}


- (void)csoundObjComplete:(CsoundObj *)csoundObj {
    [mMotionManager stopAccelerometerUpdates];
    [mMotionManager stopGyroUpdates];
    [mMotionManager stopDeviceMotionUpdates];
}

@end
