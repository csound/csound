/*
 
 TrappedGeneratorViewController.m:
 
 Copyright (C) 2014 Steven Yi, Aurelius Prochazka
 Updated in 2017 by Dr. Richard Boulanger, Nikhil Singh
 
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
 Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 
 */

#import "TrappedGeneratorViewController.h"

// Shared across instances: disk renders outlive the controller that started them,
// and they all write to the same file. Only touched on the main thread.
static BOOL sIsRendering = NO;

@interface TrappedGeneratorViewController () {
    NSString *localFilePath;
    AVPlayer *player;
}

@end

@implementation TrappedGeneratorViewController

- (void)viewDidLoad {
    self.title = @"06. Render: Trapped in Convert";
    [super viewDidLoad];

    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    localFilePath = [paths[0] stringByAppendingPathComponent:@"trapped.wav"];
}

- (IBAction)generateTrappedToDocumentsFolder:(id)sender {
    // Disk renders can't be stopped mid-way, so don't start a second one
    if (sIsRendering) {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Already Rendering" message:@"Please wait for the current render to finish before starting another." preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:defaultAction];
        [self presentViewController:alert animated:YES completion:nil];
        return;
    }

    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"trapped" ofType:@"csd"];
    NSLog(@"OUTPUT: %@", localFilePath);

    [player pause];
    player = nil;
    sIsRendering = YES;

    // Remove any previous output so completion can tell whether this render wrote a file
    [[NSFileManager defaultManager] removeItemAtPath:localFilePath error:nil];

    self.csound = [[CsoundObj alloc] init];

    [self.csound addListener:self];
    [self.csound record:csdPath toFile:localFilePath];
}

- (IBAction)play:(id)sender {
    // Checked on each tap rather than cached: the render may have been started, and
    // finished, by a previous instance of this screen that receives the completion.
    BOOL hasRendered = !sIsRendering && [[NSFileManager defaultManager] fileExistsAtPath:localFilePath];

    if(hasRendered) {
        // AVPlayer loads the file asynchronously, so this is safe on the main thread
        player = [AVPlayer playerWithURL:[NSURL fileURLWithPath:localFilePath]];
        [player play];
    } else {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Not Rendered" message:@"Please render the file before playing it." preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        
        [alert addAction:defaultAction];
        [self presentViewController:alert animated:YES completion:nil];
    }
}

- (IBAction)stop:(id)sender {
    [player pause];
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 140)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"This example demonstrates creating a virtual Csound console, and implementing a method to update this virtual console with information accessed from CsoundObj.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

#pragma mark CsoundObjListener

- (void)csoundObjCompleted:(CsoundObj *)csoundObj {
    
    // Hop over to the main thread safely
    dispatch_async(dispatch_get_main_queue(), ^{
        sIsRendering = NO;
        
        // Completion fires even if Csound failed to compile, so check the file was written
        BOOL succeeded = [[NSFileManager defaultManager] fileExistsAtPath:self->localFilePath];

        // If this controller is no longer on screen (e.g. the user navigated away while
        // the render was in flight), there's nowhere to present the alert.
        if (self.view.window == nil) {
            return;
        }

        NSString *title = succeeded ? @"Render Complete" : @"Render Failed";
        NSString *message = succeeded ? @"File generated as trapped.wav in application Documents Folder." : @"Csound could not render trapped.csd. Check the console for details.";

        UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        [alert addAction:defaultAction];

        [self presentViewController:alert animated:YES completion:nil];
    });
}


@end
