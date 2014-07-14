//
//  CsoundMotion.h
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import <Foundation/Foundation.h>
#import "CsoundObj.h"

@interface CsoundMotion : NSObject<CsoundObjCompletionListener>

- (instancetype)initWithCsoundObj:(CsoundObj *)csound;

-(id<CsoundValueCacheable>)enableAccelerometer;
-(id<CsoundValueCacheable>)enableGyroscope;
-(id<CsoundValueCacheable>)enableAttitude;

@end
