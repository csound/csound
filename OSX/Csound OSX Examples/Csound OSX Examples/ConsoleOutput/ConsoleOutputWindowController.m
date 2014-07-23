//
//  ConsoleOutputWindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/15/14.
//
//

#import "ConsoleOutputWindowController.h"
#import "CsoundObj.h"

@interface ConsoleOutputWindowController () {
    CsoundObj *csound;
}
@property (strong) IBOutlet NSTextView *consoleOutputTextView;
@property (nonatomic, strong) NSString *currentMessage;

@end

@implementation ConsoleOutputWindowController

- (IBAction)render:(NSButton *)sender
{
	_consoleOutputTextView.string = @"";
	

	csound = [[CsoundObj alloc] init];

	[csound setMessageCallback:@selector(messageCallback:) withListener:self];
	
	NSString *csdPath = nil;
	csdPath = [[NSBundle mainBundle] pathForResource:@"consoleoutput" ofType:@"csd"];
	[csound play:csdPath];
}

- (void)updateUIWithNewMessage:(NSString *)newMessage
{
	NSString *oldText = _consoleOutputTextView.string;
	NSString *fullText = [oldText stringByAppendingString:newMessage];
	_consoleOutputTextView.string = fullText;
}

- (void)messageCallback:(NSValue *)infoObj
{
    @autoreleasepool {
        
        Message info;
        [infoObj getValue:&info];
        char message[1024];
        vsnprintf(message, 1024, info.format, info.valist);
        NSString *messageStr = [NSString stringWithFormat:@"%s", message];
        [self performSelectorOnMainThread:@selector(updateUIWithNewMessage:)
                               withObject:messageStr
                            waitUntilDone:NO];
    }
}


@end
