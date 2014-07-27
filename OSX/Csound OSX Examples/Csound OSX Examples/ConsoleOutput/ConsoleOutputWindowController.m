/*
 
 ConsoleOutputWindowController.m:
 
 Copyright (C) 2014 Aurelius Prochazka
 
 This file is part of Csound OSX Examples.
 
 The Csound for iOS Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

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
