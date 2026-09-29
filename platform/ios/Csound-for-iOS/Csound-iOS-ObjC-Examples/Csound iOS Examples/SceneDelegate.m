/*
 
 SceneDelegate.m: UI scene lifecycle implementation
 
 Copyright (C) 2026 Victor Lazzarini
 
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
#import "SceneDelegate.h"
#import "AppDelegate.h"
#import "MasterViewController.h"
#import "SimpleTest1ViewController.h"
@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions {
    UIWindowScene *windowScene = (UIWindowScene *)scene;
    if (!windowScene) return;

    // 1. Establish the window utilizing the modern scene context
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];

    // 2. Grab a reference to the running AppDelegate
    AppDelegate *appDelegate = (AppDelegate *)[UIApplication sharedApplication].delegate;

    // 3. Mirror your original device layout setup
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        MasterViewController *masterViewController = [[MasterViewController alloc] initWithNibName:@"MasterViewController_iPhone" bundle:nil];
        UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:masterViewController];
        
        // Save references back to appDelegate if your app logic reads them elsewhere
        appDelegate.navigationController = navController;
        
        self.window.rootViewController = navController;
    } else {
        MasterViewController *masterViewController = [[MasterViewController alloc] initWithNibName:@"MasterViewController_iPad" bundle:nil];
        UINavigationController *masterNavigationController = [[UINavigationController alloc] initWithRootViewController:masterViewController];
        
        SimpleTest1ViewController *detailViewController = [[SimpleTest1ViewController alloc] initWithNibName:@"SimpleTest1ViewController" bundle:nil];
        
        UINavigationController *detailNavigationController = [[UINavigationController alloc] initWithRootViewController:detailViewController];
        
        UISplitViewController *splitViewController = [[UISplitViewController alloc] init];
        splitViewController.delegate = detailViewController;
        splitViewController.viewControllers = @[masterNavigationController, detailNavigationController];
        detailViewController.navigationItem.leftBarButtonItem = splitViewController.displayModeButtonItem;
        
        // Save reference back to appDelegate if needed
        appDelegate.splitViewController = splitViewController;
        
        self.window.rootViewController = splitViewController;
    }

    // 4. Synced link back to the global delegate instance structure
    appDelegate.window = self.window;

    // 5. Present the fully populated view stack on screen
    [self.window makeKeyAndVisible];
}
@end

