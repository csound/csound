//
//  CsoundUI.m
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import "CsoundUI.h"

#import "CachedButton.h"
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

-(id<CsoundValueCacheable>)addButton:(NSButton *)button
                      forChannelName:(NSString *)channelName
{
    
    CachedButton *cachedButton = [[CachedButton alloc] init:button
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedButton];
    
    return cachedButton;
}

-(id<CsoundValueCacheable>)addSlider:(NSSlider *)slider
                      forChannelName:(NSString *)channelName
{
    CachedSlider* cachedSlider = [[CachedSlider alloc] init:slider
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedSlider];
    
    return cachedSlider;
}


@end
