//
//  CsoundUI.h
//  Csound iOS Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import <Foundation/Foundation.h>
#import "CsoundObj.h"

@interface CsoundUI : NSObject

- (instancetype)initWithCsoundObj:(CsoundObj *)csound;

-(id<CsoundValueCacheable>)addSwitch:(UISwitch*)uiSwitch forChannelName:(NSString*)channelName;
-(id<CsoundValueCacheable>)addSlider:(UISlider*)uiSlider forChannelName:(NSString*)channelName;
-(id<CsoundValueCacheable>)addButton:(UIButton*)uiButton forChannelName:(NSString*)channelName;

@end
