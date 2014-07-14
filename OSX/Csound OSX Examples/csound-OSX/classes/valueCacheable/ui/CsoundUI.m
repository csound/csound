//
//  CsoundUI.m
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import "CsoundUI.h"

#import "CachedSlider.h"

@interface CsoundUI () {
    CsoundObj *csoundObj;
}
@end
@implementation CsoundUI

- (instancetype)initWithCsoundObj:(CsoundObj *)csound {
    self = [super init];
    if (self) {
        csoundObj = csound;
    }
    return self;
}


-(id<CsoundValueCacheable>)addSlider:(NSSlider*)uiSlider forChannelName:(NSString*)channelName {
    
    CachedSlider* cachedSlider = [[CachedSlider alloc] init:uiSlider
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedSlider];
    
    return cachedSlider;
}


@end
