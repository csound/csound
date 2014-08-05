//
//  AudioFileTestWindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/16/14.
//
//

#import "AudioFileTestWindowController.h"

@interface AudioFileTestWindowController () <CsoundObjListener>
@property (strong) IBOutlet NSSlider *pitchSlider;

@end

@implementation AudioFileTestWindowController

- (void)windowDidLoad {
    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"audiofiletest" ofType:@"csd"];

    [self.csound addListener:self];
    
    CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
    [csoundUI addSlider:_pitchSlider forChannelName:@"pitch"];
    
    [self.csound play:csdPath];
}

- (IBAction)play:(NSButton *)sender
{
	NSString *audioFilePath = [[NSBundle mainBundle] pathForResource:@"testAudioFile"
															  ofType:@"aif"];
	NSString *score = [NSString stringWithFormat:@"i1 0 1 \"%@\"", audioFilePath];
    NSLog(@"Sending Score %@", score);
    [self.csound sendScore:score];
}

#pragma mark CsoundObjListener

- (void)csoundObjStarted:(CsoundObj *)csoundObj {
    NSLog(@"Csound Started");
}
- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    NSLog(@"Csound Completed");
}


@end
