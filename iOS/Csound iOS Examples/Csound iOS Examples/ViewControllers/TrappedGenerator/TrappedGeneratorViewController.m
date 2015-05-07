/*
 
 TrappedGeneratorViewController.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 
 This file is part of Csound iOS Examples.
 
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

#import "TrappedGeneratorViewController.h"

@interface TrappedGeneratorViewController ()

@end

@implementation TrappedGeneratorViewController

- (void)viewDidLoad {
    self.title = @"Trapped Generator";
    [super viewDidLoad];
}

- (IBAction)generateTrappedToDocumentsFolder:(id)sender {
    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"trapped" ofType:@"csd"];
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *localFilePath = [documentsDirectory stringByAppendingPathComponent:@"trapped.wav"];
    NSLog(@"OUTPUT: %@", localFilePath);
    
    [self.csound stop];
    
    self.csound = [[CsoundObj alloc] init];
    
    [self.csound addListener:self];
    [self.csound record:csdPath toFile:localFilePath];    
}

#pragma mark CsoundObjListener


- (void)csoundObjStarted:(CsoundObj *)csoundObj {
}

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    
    NSString *title = @"Render Complete";
    NSString *message = @"File generated as trapped.wav in application Documents Folder";
    
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:title
                                                    message:message
                                                   delegate:nil
                                          cancelButtonTitle:@"Dismiss"
                                          otherButtonTitles:nil];
                      
    [alert performSelectorOnMainThread:@selector(show) withObject:nil waitUntilDone:NO];

}


@end
