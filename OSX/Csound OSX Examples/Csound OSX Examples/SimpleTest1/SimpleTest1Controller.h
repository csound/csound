//
//  SimpleTest1Controller.h
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import <Foundation/Foundation.h>
#import "CsoundObj.h"

@interface SimpleTest1Controller : NSObject <CsoundObjCompletionListener>

@property (weak) IBOutlet NSButton *toggleOnOffButton;
@property (weak) IBOutlet NSSlider *mSlider;

- (IBAction)toggleOnOff:(id)sender;

@end
