/* 
 
 WaveviewViewController.h:
 
 Copyright (C) 2011 Steven Yi, Ed Costello
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

#import "WaveviewViewController.h"

@implementation WaveviewViewController {
    int fTableIndex;
    NSArray *fTables;
}

-(void)viewDidLoad {
    self.title = @"09. F-table Viewer";
    [super viewDidLoad];
    fTableIndex = 0;
    fTables = @[@"Sine", @"Exponential Curves", @"Data Points", @"Normalizing Function", @"Triangle"];
}

-(void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	
	NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"Waveviewtest" ofType:@"csd"];  
	NSLog(@"FILE PATH: %@", tempFile);
	
	[self.csound stop];
	self.csound = [[CsoundObj alloc] init];
	[self.csound  addBinding:waveview];
    
    
    UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(incrementFTable:)];
    [waveview addGestureRecognizer:tap];
	
	[self.csound play:tempFile];
    
}

- (IBAction)incrementFTable:(UITapGestureRecognizer *)tap {
    fTableIndex++;
    if (fTableIndex >= fTables.count) {
        fTableIndex = 0;
    }
    titleLabel.text = fTables[fTableIndex];
    [waveview displayFTable:fTableIndex+1];
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 120)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Demonstrates using a Csound binding to draw F-table content to a view. Tap the screen to cycle through available F-tables.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

@end
