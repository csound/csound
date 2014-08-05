//
//  BaseCsoundWindowController.h
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 8/5/14.
//
//

#import <Cocoa/Cocoa.h>
#import "CsoundObj.h"
#import "CsoundUI.h"
@interface BaseCsoundWindowController : NSWindowController

@property (nonatomic, strong) CsoundObj* csound;
@end
