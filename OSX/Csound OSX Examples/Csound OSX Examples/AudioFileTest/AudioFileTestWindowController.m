//
//  AudioFileTestWindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/16/14.
//
//

#import "AudioFileTestWindowController.h"
#import "CsoundObj.h"
#import "CsoundUI.h"

@interface AudioFileTestWindowController () <CsoundObjListener> {
    CsoundObj *csound;
}
@property (strong) IBOutlet NSSlider *pitchSlider;

@end

@implementation AudioFileTestWindowController

- (void)windowDidLoad {
    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"audiofiletest" ofType:@"csd"];
    csound = [[CsoundObj alloc] init];
    [csound addListener:self];
    
    CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
    [csoundUI addSlider:_pitchSlider forChannelName:@"pitch"];
    
    [csound play:csdPath];
}

- (IBAction)play:(NSButton *)sender
{
	NSString *audioFilePath = [[NSBundle mainBundle] pathForResource:@"testAudioFile"
															  ofType:@"aif"];
	NSString *score = [NSString stringWithFormat:@"i1 0 1 \"%@\"", audioFilePath];
    NSLog(@"Sending Score %@", score);
    [csound sendScore:score];
}

- (void)csoundObjStarted:(CsoundObj *)csoundObj {
    NSLog(@"Csound Started");
}
- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    NSLog(@"Csound Completed");
}


@end
