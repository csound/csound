//
//  RecordTestWindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/27/14.
//
//

#import "RecordTestWindowController.h"
#import <AVFoundation/AVFoundation.h>

@interface RecordTestWindowController () <CsoundObjListener, AVAudioPlayerDelegate>
@property (strong) IBOutlet NSButton *recordButton;
@property (strong) IBOutlet NSButton *playButton;
@property (strong) IBOutlet NSSlider *gainSlider;
@property (strong) IBOutlet NSLevelIndicator *audioLevelIndicator;
@property (strong) IBOutlet NSTextField *gainLabel;

@property (nonatomic, strong) AVAudioPlayer *audioPlayer;
@end

@implementation RecordTestWindowController

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:self.csound];
    [csoundUI addSlider:self.gainSlider forChannelName:@"gain"];
    
    self.audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:[self recordingURL] error:nil];
	[self.audioPlayer setDelegate:self];
}


-(IBAction) recordOrStop:(id)sender {
    NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"recordTest"
                                                        ofType:@"csd"];

    if ([self.recordButton.title isEqualToString:@"Record"]) {
//        [self.csound record:csdFile toURL:[self recordingURL]];
        [self.csound play:csdFile];
        self.recordButton.title = @"Stop";
    } else {
        [self.csound stopRecording];
        [self.csound stop];
        self.recordButton.title = @"Record";
    }
}

- (IBAction)changeGain:(id)sender
{
	[self.gainLabel setStringValue:[NSString stringWithFormat:@"%.2f", self.gainSlider.floatValue]];
}

- (IBAction)playOrStop:(id)sender
{
    if ([self.playButton.title isEqualToString:@"Play"]) {
        [self.audioPlayer setCurrentTime:0];
        [self.audioPlayer play];
        self.playButton.title = @"Stop";
    } else {
        [self.audioPlayer stop];
        self.playButton.title = @"Play";
    }
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
    [self.audioPlayer setCurrentTime:0];
    self.playButton.title = @"Play";
}


- (NSURL *)recordingURL
{
    NSURL *localDocDirURL = nil;
    if (localDocDirURL == nil) {
        NSString *docDirPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)
                                objectAtIndex:0];
        localDocDirURL = [NSURL fileURLWithPath:docDirPath];
    }
    return [localDocDirURL URLByAppendingPathComponent:@"recording.wav"];
}

#pragma mark CsoundObjListener

-(void)csoundObjStarted:(CsoundObj *)csoundObj {
	[self.csound recordToURL:[self recordingURL]];
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj {
    [self close];
}


@end
