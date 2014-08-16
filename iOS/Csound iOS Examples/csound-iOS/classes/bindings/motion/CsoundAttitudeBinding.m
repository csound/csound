/* 
 
 CsoundAttitudeBinding.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 
 This file is part of Csound for iOS.
 
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

#import "CsoundAttitudeBinding.h"

@interface CsoundAttitudeBinding () {
    float *channelPtrRoll;
    float *channelPtrPitch;
    float *channelPtrYaw;
    
    CMMotionManager* manager;
}
@end

@implementation CsoundAttitudeBinding

static NSString *CS_ATTITUDE_ROLL  = @"attitudeRoll";
static NSString *CS_ATTITUDE_PITCH = @"attitudePitch";
static NSString *CS_ATTITUDE_YAW   = @"attitudeYaw";

-(id)init:(CMMotionManager *)cmManager {
    if (self = [super init]) {
        manager = cmManager;
    }
    return self;
}

-(void)setup:(CsoundObj *)csoundObj {
    channelPtrRoll = [csoundObj getInputChannelPtr:CS_ATTITUDE_ROLL
                                       channelType:CSOUND_CONTROL_CHANNEL];
    channelPtrPitch = [csoundObj getInputChannelPtr:CS_ATTITUDE_PITCH
                                        channelType:CSOUND_CONTROL_CHANNEL];
    channelPtrYaw = [csoundObj getInputChannelPtr:CS_ATTITUDE_YAW
                                      channelType:CSOUND_CONTROL_CHANNEL];
    
    *channelPtrRoll  = manager.deviceMotion.attitude.roll;
    *channelPtrPitch = manager.deviceMotion.attitude.pitch;
    *channelPtrYaw   = manager.deviceMotion.attitude.yaw;
    
}

-(void)updateValuesToCsound {
    @autoreleasepool {
        *channelPtrRoll  = manager.deviceMotion.attitude.roll;
        *channelPtrPitch = manager.deviceMotion.attitude.pitch;
        *channelPtrYaw   = manager.deviceMotion.attitude.yaw;
    }
}


@end
