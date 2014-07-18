//
//  SimpleTest1WindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/14/14.
//
//

#import "SimpleTest1WindowController.h"

#import "CsoundObj.h"
#import "CsoundUI.h"

@interface SimpleTest1WindowController() <CsoundObjListener> {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *toggleOnOffButton;
@property (strong) IBOutlet NSSlider *slider;
@end

@implementation SimpleTest1WindowController

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.toggleOnOffButton.title isEqualToString:@"Start"]) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"csd"];
        csound = [[CsoundObj alloc] init];
        [csound addListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_slider forChannelName:@"slider"];
        
        [csound startCsound:tempFile];
        
	} else {
        [csound stopCsound];
    }
}

#pragma mark CsoundObjListener

-(void)csoundObjStarted:(CsoundObj *)csoundObj {
    self.toggleOnOffButton.title = @"Stop";
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
    NSLog(@"Csound finished");
	self.toggleOnOffButton.title = @"Start";
}

@end
