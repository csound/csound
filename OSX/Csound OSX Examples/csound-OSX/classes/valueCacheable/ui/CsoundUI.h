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

-(id<CsoundValueCacheable>)addButton:(NSButton *)button
                      forChannelName:(NSString *)channelName;

-(id<CsoundValueCacheable>)addSlider:(NSSlider*)slider
                      forChannelName:(NSString*)channelName;

@end
