/* 
 
 CsoundHaiku4ViewController.m:
 
 Copyright (C) 2011 Steven Yi
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
#import "CsoundHaiku4ViewController.h"

@implementation CsoundHaiku4ViewController

-(void)viewDidLoad {
    self.title = @"05. Play: Haiku IV";
    [super viewDidLoad];
}

-(void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	
	NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"IV" ofType:@"csd"];  
	NSLog(@"FILE PATH: %@", tempFile);
	
	[self.csound stop];
	
	self.csound = [[CsoundObj alloc] init];
	
	[self.csound play:tempFile];
}

-(IBAction)showSite:(UIButton *)sender {
    NSURL *url = [NSURL URLWithString:@"http://iainmccurdy.org/csoundhaiku.html"];
    SFSafariViewController *safariVC = [[SFSafariViewController alloc] initWithURL:url];
    
    [self presentViewController:safariVC animated:YES completion:nil];
}

- (IBAction)showInfo:(UIButton *)sender {
   UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 180)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Haiku IV is the fourth in a suite of nine generative Csound pieces by Iain McCurdy. Csound begins rendering the work when the view appears and stops when the view unloads and CsoundObj is deallocated.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

@end
