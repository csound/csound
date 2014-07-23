//
//  CsoundHaiku4WindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/14/14.
//
//

#import "CsoundHaiku4WindowController.h"
#import "CsoundObj.h"

@interface CsoundHaiku4WindowController() {
    CsoundObj* csound;
}
@end


@implementation CsoundHaiku4WindowController

- (IBAction)startPlayCSDFile:(id)sender {
	NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"IV" ofType:@"csd"];
	
	csound = [[CsoundObj alloc] init];
	[csound play:tempFile];
    
}

- (IBAction)stopPlayCSDFile:(id)sender {
    [csound stop];
}


@end
