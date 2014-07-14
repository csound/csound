//
//  CsoundHaiku4Controller.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/13/14.
//
//

#import "CsoundHaiku4Controller.h"
#import "CsoundObj.h"

@interface CsoundHaiku4Controller() {
    CsoundObj* csound;
}
@end


@implementation CsoundHaiku4Controller

- (IBAction)startPlayCSDFile:(id)sender {
	NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"IV" ofType:@"csd"];
	
	csound = [[CsoundObj alloc] init];
	[csound startCsound:tempFile];

}

- (IBAction)stopPlayCSDFile:(id)sender {
    [csound stopCsound];
}

@end
