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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

#import "TrappedGeneratorViewController.h"

@interface TrappedGeneratorViewController () {
    BOOL hasRendered;
    NSString *localFilePath;
    AVAudioPlayer *player;
}

@end

@implementation TrappedGeneratorViewController

- (void)viewDidLoad {
    self.title = @"06. Render: Trapped in Convert";
    [super viewDidLoad];
    hasRendered = NO;
}

- (IBAction)generateTrappedToDocumentsFolder:(id)sender {
    NSString *csdPath = [[NSBundle mainBundle] pathForResource:@"trapped" ofType:@"csd"];
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    localFilePath = [documentsDirectory stringByAppendingPathComponent:@"trapped.wav"];
    NSLog(@"OUTPUT: %@", localFilePath);
    
    [self.csound stop];
    
    self.csound = [[CsoundObj alloc] init];
    
    [self.csound addListener:self];
    [self.csound record:csdPath toFile:localFilePath];    
}

- (IBAction)play:(id)sender {
    if(hasRendered) {
        NSError *error;
        player = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:localFilePath] error:&error];
        
        if(error != nil) NSLog(@"%@", [error localizedDescription]);
        
        [player prepareToPlay];
        [player play];
    } else {
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Not Rendered" message:@"Please render the file before playing it." preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
        
        [alert addAction:defaultAction];
        [self presentViewController:alert animated:YES completion:nil];
    }
}

- (IBAction)stop:(id)sender {
    [player stop];
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
    
    NSString *title = @"Render Complete";
    NSString *message = @"File generated as trapped.wav in application Documents Folder.";
    
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil];
    [alert addAction:defaultAction];
    [self presentViewController:alert animated:YES completion:nil];
    
    hasRendered = YES;
}


@end
