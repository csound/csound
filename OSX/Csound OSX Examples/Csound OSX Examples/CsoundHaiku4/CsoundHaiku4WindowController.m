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
	NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"IV" ofType:@"csd"];
	
	csound = [[CsoundObj alloc] init];
	[csound play:csdFile];
    
}

- (IBAction)stopPlayCSDFile:(id)sender {
    [csound stop];
}


@end
