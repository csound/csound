/* 
 
 MasterViewController.m:
 
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
 Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 
 */

#import "MasterViewController.h"
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

- (instancetype)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        self.title = @"Csound for iOS";
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            self.clearsSelectionOnViewWillAppear = NO;
        }
        testNames = [NSMutableArray arrayWithObjects:
                     @"01. Simple Test 1",
                     @"02. Simple Test 2",
                     @"03. Button Test",
                     @"04. Play: Haiku IV",
                     @"05. Render: Trapped in Convert",
                     @"06. Render: Console Output",
                     @"07. Instrument Tweaker",
                     @"08. F-table Viewer",
                     @"09. Soundfile: Pitch Shifter",
                     @"10. Mic: Stereo Delay",
                     @"11. Mic: Harmonizer",
                     @"12. Mic: Recording",
                     @"13. Hardware: MIDI Controller",
                     @"14. Hardware: Motion Control",
                     @"15. XY Pad: MultiTouch ",
                     @"16. XY Pad: Mic PitchShift+Mix",
                     nil];
    }
    return self;
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
            controller = [[CsoundHaiku4ViewController alloc] initWithNibName:@"CsoundHaiku4ViewController" bundle:nil];
            break;
        case 4:
            controller = [[TrappedGeneratorViewController alloc] initWithNibName:@"TrappedGeneratorViewController" bundle:nil];
            break;
        case 5:
            controller = [[ConsoleOutputViewController alloc] initWithNibName:@"ConsoleOutputViewController" bundle:nil];
            break;            
        case 6:
            controller = [[InstrumentEditorViewController  alloc] initWithNibName:@"InstrumentEditorViewController" bundle:nil];
            break;
		case 7:
            controller = [[WaveviewViewController alloc] initWithNibName:@"WaveviewViewController" bundle:nil];
			break;
        case 8:
            controller = [[AudioFileTestViewController alloc] initWithNibName:@"AudioFileTestViewController" bundle:nil];
			break;
		case 9:
            controller = [[AudioInTestViewController alloc] initWithNibName:@"AudioInTestViewController" bundle:nil];
			break;
		case 10:
            controller = [[HarmonizerTest alloc] initWithNibName:@"HarmonizerTest" bundle:nil];
			break;
		case 11:
            controller = [[RecordTestViewController alloc] initWithNibName:@"RecordTestViewController" bundle:nil];
			break;
		case 12:
            if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
                controller = [[MidiTestViewController alloc] initWithNibName:@"MidiTestViewController" bundle:nil];
            } else {
                controller = [[MidiTestViewController alloc] initWithNibName:@"MidiTestViewController_iPad" bundle:nil];
            }
			break;
		case 13:
            controller = [[HardwareTestViewController alloc] initWithNibName:@"HardwareTestViewController" bundle:nil];
            break;
        case 14:
			controller = [[MultiTouchXYViewController alloc] initWithNibName:@"MultiTouchXYViewController" bundle:nil];
			break;
        case 15:
			controller = [[PitchShifterViewController alloc] initWithNibName:@"PitchShifterViewController" bundle:nil];
			break;
		default:
            break;
    }

    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        [self.navigationController pushViewController:controller animated:YES];
        [(UITableView *)self.view deselectRowAtIndexPath:indexPath animated:YES];
        
    } else {
        
        AppDelegate *appDelegate = (AppDelegate *)([UIApplication sharedApplication].delegate);
        UISplitViewController *splitViewController = appDelegate.splitViewController;

        [splitViewController setViewController:controller forColumn:UISplitViewControllerColumnSecondary];

        if (splitViewController.collapsed) {
            // In a narrow window the columns collapse into one navigation
            // stack, so the example has to be pushed explicitly.
            [splitViewController showColumn:UISplitViewControllerColumnSecondary];
        } else if (splitViewController.displayMode == UISplitViewControllerDisplayModeOneOverSecondary) {
            // Dismiss the sidebar if it is overlaying the example.
            [splitViewController hideColumn:UISplitViewControllerColumnPrimary];
        }
    }
    
}

@end
