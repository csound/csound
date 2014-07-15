//
//  SimpleTest2Controller.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/14/14.
//
//

#import "SimpleTest2Controller.h"
#import "CsoundUI.h"

@interface SimpleTest2Controller() {
    CsoundObj* csound;
}
@end

@implementation SimpleTest2Controller

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.startStopButton.title isEqualToString:@"Start"]) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"test2" ofType:@"csd"];
        NSLog(@"FILE PATH: %@", tempFile);
        
        csound = [[CsoundObj alloc] init];
        [csound addCompletionListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_rateSlider     forChannelName:@"noteRate"];
        [csoundUI addSlider:_durationSlider forChannelName:@"duration"];
        [csoundUI addSlider:_attackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:_decaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:_sustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:_releaseSlider  forChannelName:@"release"];
        
        [csound startCsound:tempFile];
        
	} else {
        [csound stopCsound];
    }
}

#pragma mark CsoundObjCompletionListener

-(void)csoundObjDidStart:(CsoundObj *)csoundObj {
    self.startStopButton.title = @"Stop";
}

-(void)csoundObjComplete:(CsoundObj *)csoundObj {
	self.startStopButton.title = @"Start";
}

@end
