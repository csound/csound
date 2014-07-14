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

-(id<CsoundValueCacheable>)addSlider:(NSSlider*)uiSlider forChannelName:(NSString*)channelName;

@end
