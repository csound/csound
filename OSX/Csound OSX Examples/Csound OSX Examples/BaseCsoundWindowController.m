//
//  BaseCsoundWindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 8/5/14.
//
//

#import "BaseCsoundWindowController.h"


@implementation BaseCsoundWindowController

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
        self.csound = [[CsoundObj alloc] init];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowWillClose:)
                                                     name:NSWindowWillCloseNotification
                                                   object:nil];
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}
- (void)windowWillClose:(NSNotification *)note {
    [self.csound stop];
}

@end
