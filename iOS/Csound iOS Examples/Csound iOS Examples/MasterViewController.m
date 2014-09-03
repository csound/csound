/* 
 
 MasterViewController.m:
 
 Copyright (C) 2011 Steven Yi
 
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

#import "MasterViewController.h"
#import "DetailViewController.h"
#import "SimpleTest1ViewController.h"
#import "SimpleTest2ViewController.h"
#import "ButtonTestViewController.h"
#import "MidiTestViewController.h"
#import "AppDelegate.h"
#import "BaseCsoundViewController.h"
#import "AudioInTestViewController.h"
#import "HarmonizerTest.h"
#import "HardwareTestViewController.h"
#import "InstrumentEditorViewController.h"
#import "CsoundHaiku4ViewController.h"
#import "RecordTestViewController.h"
#import "MultiTouchXYViewController.h"
#import "WaveviewViewController.h"
#import "AudioFileTestViewController.h"
#import "ConsoleOutputViewController.h"
#import "PitchShifterViewController.h"
#import "TrappedGeneratorViewController.h"

@implementation MasterViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        self.title = @"Csound for iOS";
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.clearsSelectionOnViewWillAppear = NO;
            self.preferredContentSize = CGSizeMake(320.0, 600.0);
        }
        testNames = [NSMutableArray arrayWithObjects:@"Simple Test 1", @"Simple Test 2", 
                      @"Button Test", @"MIDI Test", @"Ping Pong Delay", @"Harmonizer", @"Hardware Test", @"Csound Haiku 4", @"Record Test", @"Multitouch XY", @"Waveview", @"Audio File Test", @"Console Output", @"Pitch Shifter", @"Trapped Generator",@"Instrument Editor", nil];
    }
    return self;
}
							

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        [self.tableView selectRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0] animated:NO scrollPosition:UITableViewScrollPositionMiddle];
    }
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
	[super viewDidDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
    } else {
        return YES;
    }
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return testNames.count;
}

// Customize the appearance of table view cells.
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        }
    }

    // Configure the cell.
    cell.textLabel.text = [testNames objectAtIndex:indexPath.row];
    return cell;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source.
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view.
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    
    BaseCsoundViewController* controller;
	
	//should probably enum this someday
    
    switch (indexPath.row) {
        case 0:
            controller = [[SimpleTest1ViewController alloc] initWithNibName:@"SimpleTest1ViewController" bundle:nil];
            break;
        case 1:
            controller = [[SimpleTest2ViewController alloc] initWithNibName:@"SimpleTest2ViewController" bundle:nil];
            break;
        case 2:
            controller = [[ButtonTestViewController alloc] initWithNibName:@"ButtonTestViewController" bundle:nil];
            break;
        case 3:
		{
			if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
				controller = [[MidiTestViewController alloc] initWithNibName:@"MidiTestViewController" bundle:nil];
			} else {
				controller = [[MidiTestViewController alloc] initWithNibName:@"MidiTestViewController_iPad" bundle:nil];
			}
			
		}
            
            break;
        case 4:
            controller = [[AudioInTestViewController alloc] initWithNibName:@"AudioInTestViewController" bundle:nil];
            break;
        case 5:
            controller = [[HarmonizerTest alloc] initWithNibName:@"HarmonizerTest" bundle:nil];
            break;            
        case 6:
            controller = [[HardwareTestViewController alloc] initWithNibName:@"HardwareTestViewController" bundle:nil];
            break;
		case 7:
			controller = [[CsoundHaiku4ViewController alloc] initWithNibName:@"CsoundHaiku4ViewController" bundle:nil];
			break;
        case 8:
			controller = [[RecordTestViewController alloc] initWithNibName:@"RecordTestViewController" bundle:nil];
			break;
		case 9:
			controller = [[MultiTouchXYViewController alloc] initWithNibName:@"MultiTouchXYViewController" bundle:nil];
			break;
		case 10:
			controller = [[WaveviewViewController alloc] initWithNibName:@"WaveviewViewController" bundle:nil];
			break;	
		case 11:
			controller = [[AudioFileTestViewController alloc] initWithNibName:@"AudioFileTestViewController" bundle:nil];
			break;
		case 12:
			controller = [[ConsoleOutputViewController alloc] initWithNibName:@"ConsoleOutputViewController" bundle:nil];
			break;
		case 13:
			controller = [[PitchShifterViewController alloc] initWithNibName:@"PitchShifterViewController" bundle:nil];
            break;
        case 14:
			controller = [[TrappedGeneratorViewController alloc] initWithNibName:@"TrappedGeneratorViewController" bundle:nil];
			break;
        case 15:
			controller = [[InstrumentEditorViewController  alloc] initWithNibName:@"InstrumentEditorViewController" bundle:nil];
			break;
		default:
            break;
    }

    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
//	    if (!self.detailViewController) {
//	        self.detailViewController = [[[DetailViewController alloc] initWithNibName:@"DetailViewController_iPhone" bundle:nil] autorelease];
//	    }
//        [self.navigationController pushViewController:self.detailViewController animated:YES];
        [self.navigationController pushViewController:controller animated:YES];
        [(UITableView *)self.view deselectRowAtIndexPath:indexPath animated:YES];
        
    } else {
        
        AppDelegate *appDelegate = [UIApplication sharedApplication].delegate;
        UISplitViewController *splitViewController = appDelegate.splitViewController;
        
        BaseCsoundViewController *currentDetail = (BaseCsoundViewController *)splitViewController.delegate;
        
        if(currentDetail.navigationItem.leftBarButtonItem != nil) {
            controller.navigationItem.leftBarButtonItem = currentDetail.navigationItem.leftBarButtonItem;
            controller.masterPopoverController = currentDetail.masterPopoverController;
        }
        
        UINavigationController *detailNavigationController = [[UINavigationController alloc] initWithRootViewController:controller];
        
        NSArray *viewControllers = [[NSArray alloc] initWithObjects:[splitViewController.viewControllers objectAtIndex:0], detailNavigationController, nil];
        splitViewController.viewControllers = viewControllers;
        splitViewController.delegate = controller;
        
    }

    
    

}

@end
