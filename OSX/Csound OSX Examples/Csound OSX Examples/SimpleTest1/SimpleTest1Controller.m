//
//  SimpleTest1Controller.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import "SimpleTest1Controller.h"
#import "CsoundUI.h"
@interface SimpleTest1Controller() {
    CsoundObj* csound;
}
@end

@implementation SimpleTest1Controller

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.toggleOnOffButton.title isEqualToString:@"Start"]) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"test" ofType:@"csd"];
        NSLog(@"FILE PATH: %@", tempFile);

        csound = [[CsoundObj alloc] init];
        [csound addCompletionListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_mSlider forChannelName:@"slider"];
        
        [csound startCsound:tempFile];
        
	} else {
        [csound stopCsound];
    }
}

#pragma mark CsoundObjCompletionListener

-(void)csoundObjDidStart:(CsoundObj *)csoundObj {
    NSLog(@"Got here1");
    self.toggleOnOffButton.title = @"Stop";
}

-(void)csoundObjComplete:(CsoundObj *)csoundObj {
    NSLog(@"Got here");
	self.toggleOnOffButton.title = @"Start";
}
- (IBAction)sliderMoved:(id)sender {
//    *channelPtr = cachedValue;
//    self.cacheDirty = NO;
}


@end
