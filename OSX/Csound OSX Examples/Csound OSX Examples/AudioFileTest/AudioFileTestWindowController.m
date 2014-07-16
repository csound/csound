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

@interface AudioFileTestWindowController () {
    CsoundObj *csound;
}

@property (strong) IBOutlet NSSlider *pitchSlider;
@end

@implementation AudioFileTestWindowController

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {

        csound = [[CsoundObj alloc] init];
        NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"audiofiletest" ofType:@"csd"];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] init];
        [csoundUI addSlider:self.pitchSlider forChannelName:@"pitch" ];
        [csound startCsound:csdPath];
    }
    return self;
}

- (IBAction)play:(NSButton *)sender
{
	NSString *audioFilePath = [[NSBundle mainBundle] pathForResource:@"testAudioFile"
															  ofType:@"aif"];
	NSString *score = [NSString stringWithFormat:@"i1 0 1 \"%@\"", audioFilePath];
    [csound sendScore:score];
}


@end
