/*
 
 AppDelegate.m:
 
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

#import "AppDelegate.h"

#import "AudioFileTestWindowController.h"
#import "ButtonTestWindowController.h"
#import "ConsoleOutputWindowController.h"
#import "CsoundHaiku4WindowController.h"
#import "SimpleTest1WindowController.h"
#import "SimpleTest2WindowController.h"

@interface AppDelegate() {
    AudioFileTestWindowController *audioFileTestWC;
    ButtonTestWindowController *buttonTestWC;
    ConsoleOutputWindowController *consoleOutputWC;
    CsoundHaiku4WindowController *csoundHaiku4WC;
    SimpleTest1WindowController *simpleTest1WC;
    SimpleTest2WindowController *simpleTest2WC;
}

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
}
- (IBAction)loadAudioFileTest:(id)sender {
    audioFileTestWC = [[AudioFileTestWindowController alloc] initWithWindowNibName:@"AudioFileTestWindowController"];
    [audioFileTestWC window];
}
- (IBAction)loadButtonTest:(id)sender {
    buttonTestWC = [[ButtonTestWindowController alloc] initWithWindowNibName:@"ButtonTestWindowController"];
    [buttonTestWC window];
}
- (IBAction)loadConsoleOutput:(id)sender {
    consoleOutputWC = [[ConsoleOutputWindowController alloc] initWithWindowNibName:@"ConsoleOutputWindowController"];
    [consoleOutputWC window];
}
- (IBAction)loadCsoundHaiku4:(id)sender {
    csoundHaiku4WC = [[CsoundHaiku4WindowController alloc] initWithWindowNibName:@"CsoundHaiku4WindowController"];
    [csoundHaiku4WC window];
}
- (IBAction)loadSimpleTest1:(id)sender {
    simpleTest1WC = [[SimpleTest1WindowController alloc] initWithWindowNibName:@"SimpleTest1WindowController"];
    [simpleTest1WC window];
}
- (IBAction)loadSimpleTest2:(id)sender {
    simpleTest2WC = [[SimpleTest2WindowController alloc] initWithWindowNibName:@"SimpleTest2WindowController"];
    [simpleTest2WC window];
}


@end
