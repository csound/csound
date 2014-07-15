//
//  SimpleTest2Controller.h
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/14/14.
//
//

#import <Foundation/Foundation.h>
#import "CsoundObj.h"

@interface SimpleTest2Controller : NSObject <CsoundObjCompletionListener>

@property (weak) IBOutlet NSButton *startStopButton;
@property (weak) IBOutlet NSSlider *rateSlider;
@property (weak) IBOutlet NSSlider *durationSlider;
@property (weak) IBOutlet NSSlider *attackSlider;
@property (weak) IBOutlet NSSlider *decaySlider;
@property (weak) IBOutlet NSSlider *sustainSlider;
@property (weak) IBOutlet NSSlider *releaseSlider;

@end
