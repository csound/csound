//
//  CsoundUI.m
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import "CsoundUI.h"

#import "CachedSlider.h"
#import "CachedButton.h"
#import "CachedSwitch.h"

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

-(id<CsoundValueCacheable>)addSwitch:(UISwitch*)uiSwitch forChannelName:(NSString*)channelName {
    CachedSwitch* cachedSwitch = [[CachedSwitch alloc] init:uiSwitch
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedSwitch];
	
    return cachedSwitch;
}

-(id<CsoundValueCacheable>)addSlider:(UISlider*)uiSlider forChannelName:(NSString*)channelName {
    
    CachedSlider* cachedSlider = [[CachedSlider alloc] init:uiSlider
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedSlider];
    
    return cachedSlider;
}

-(id<CsoundValueCacheable>)addButton:(UIButton*)uiButton forChannelName:(NSString*)channelName {
    CachedButton* cachedButton = [[CachedButton alloc] init:uiButton
                                                channelName:channelName];
    [csoundObj.valuesCache addObject:cachedButton];
    return cachedButton;
}

@end
