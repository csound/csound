/* 
 
 ConsoleOutputViewController.m:
 
 Copyright (C) 2014 Thomas Hass, Aurelius Prochazka
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

#import "ConsoleOutputViewController.h"

@interface ConsoleOutputViewController() {
    NSArray *csdArray;
    NSString *csdPath;
    NSString *folderPath;
    
    UIViewController *csdListVC;
}

@end

@implementation ConsoleOutputViewController

- (void)viewDidLoad {
    NSError *error;
    folderPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"csdStorage/"];
    csdArray = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:folderPath error:&error];
    
    if(error != nil) NSLog(@"%@", [error localizedDescription]);
    
    csdPath = [[NSBundle mainBundle] pathForResource:@"Yi - Countdown" ofType:@"csd" inDirectory:@"csdStorage"];
    NSLog(@"%@", csdArray);
}

#pragma mark UITableView Methods

-(void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    csdPath = [folderPath stringByAppendingPathComponent:[tableView cellForRowAtIndexPath:indexPath].textLabel.text];
    [csdListVC dismissViewControllerAnimated:YES completion:nil];
}

-(NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return csdArray.count;
}

-(UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [[UITableViewCell alloc] init];
    cell.textLabel.text = [csdArray objectAtIndex:indexPath.row];
    return cell;
}

#pragma mark IBActions

- (IBAction)run:(UIButton *)sender
{
    if(sender.isSelected == NO) {
        mTextView.text = @"";
        [sender setSelected:YES];
        
        [self.csound stop];
        self.csound = [[CsoundObj alloc] init];
        [self.csound setMessageCallback:@selector(messageCallback:) withListener:self];
        [self.csound addListener:self];
        [self.csound play:csdPath];
    } else {
        [self.csound stop];
        [sender setSelected:NO];
    }
}

- (IBAction)showCSDs:(UIButton *)sender {
    csdListVC = [[UIViewController alloc] init];
    csdListVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = csdListVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [csdListVC setPreferredContentSize:CGSizeMake(300, 600)];
    
    self.csdTable = [[UITableView alloc] initWithFrame:CGRectMake(0, 0, csdListVC.preferredContentSize.width, csdListVC.preferredContentSize.height)];
    self.csdTable.delegate = self;
    self.csdTable.dataSource = self;
    [csdListVC.view addSubview:self.csdTable];
    
    popover.delegate = self;
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:csdListVC animated:YES completion:nil];
}

- (IBAction)showInfo:(UIButton *)sender {
    UIViewController *infoVC = [[UIViewController alloc] init];
    infoVC.modalPresentationStyle = UIModalPresentationPopover;
    
    UIPopoverPresentationController *popover = infoVC.popoverPresentationController;
    popover.sourceView = sender;
    popover.sourceRect = sender.bounds;
    [infoVC setPreferredContentSize:CGSizeMake(300, 160)];
    
    UITextView *infoText = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, infoVC.preferredContentSize.width, infoVC.preferredContentSize.height)];
    infoText.editable = NO;
    infoText.selectable = NO;
    NSString *description = @"Renders, by default, a simple 5-second 'countdown' csd and publishes information to a virtual Csound console every second. Click the CSD button on the right to select a different csd file.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

-(void)csoundObjCompleted:(CsoundObj *)csoundObj
{
    dispatch_async(dispatch_get_main_queue(), ^(void) {
        [self.renderButton setSelected:NO];
    });
}

- (void)updateUIWithNewMessage:(NSString *)newMessage
{
	NSString *oldText = mTextView.text;
	NSString *fullText = [oldText stringByAppendingString:newMessage];
	mTextView.text = fullText;
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

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        self.title = @"07. Console Output";
    }
    return self;
}


@end
