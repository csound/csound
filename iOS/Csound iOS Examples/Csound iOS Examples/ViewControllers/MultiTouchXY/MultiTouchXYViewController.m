/* 
 
 MultiTouchXYViewController.m:
 
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

#import "MultiTouchXYViewController.h"

@implementation MultiTouchXYViewController

-(void)viewDidLoad {
    self.title = @"15. MultiTouch XY Pad";
	
	for (int i = 0; i < 10; i++) {
		touchIds[i] = 0;
		touchX[i] = 0.0f;
		touchY[i] = 0.0f;
		touchArray[i] = nil;
	}
    
    touchesCount = 0;
		
    [super viewDidLoad];
	

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
    NSString *description = @"Multitouch XY Pad demonstrates a multitouch performance surface. Each touch is dynamically mapped to a unique instance of a Csound instrument.";
    [infoText setAttributedText:[[NSAttributedString alloc] initWithString:description]];
    infoText.font = [UIFont fontWithName:@"Menlo" size:16];
    [infoVC.view addSubview:infoText];
    popover.delegate = self;
    
    [popover setPermittedArrowDirections:UIPopoverArrowDirectionUp];
    
    [self presentViewController:infoVC animated:YES completion:nil];
    
}

-(void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	
	NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"multiTouchXY" ofType:@"csd"];  
	NSLog(@"FILE PATH: %@", tempFile);
	
	[self.csound stop];
	
	self.csound = [[CsoundObj alloc] init];
	
	[self.csound addBinding:self];
	
	[self.csound play:tempFile];
}


#pragma mark Csound Binding

-(void)setup:(CsoundObj *)csoundObj {

	for (int i = 0; i < 10; i++) {
		touchXPtr[i] = [csoundObj getInputChannelPtr:[NSString stringWithFormat:@"touch.%d.x", i, nil]
                                  channelType:CSOUND_CONTROL_CHANNEL];
		touchYPtr[i] = [csoundObj getInputChannelPtr:[NSString stringWithFormat:@"touch.%d.y", i, nil]
                                  channelType:CSOUND_CONTROL_CHANNEL];
    }
}

-(void)updateValuesToCsound {
	for (int i = 0; i < 10; i++) {
		*touchXPtr[i] = touchX[i];
		*touchYPtr[i] = touchY[i];		
	}
}

-(void)updateValuesFromCsound {}

-(void)cleanup { 
	for (int i = 0; i < 10; i++) {
		touchXPtr[i] = 0;
		touchYPtr[i] = 0;		
	}
}

#pragma mark Touch Event Handling

- (int)getTouchIdAssignment {
	for (int i = 0; i < 10; i++) {
		if (touchIds[i] == 0) {
			return i;
		}
	}
	return -1;
}

- (int) getTouchId:(UITouch *)touch {
	for (int i = 0; i < 10; i++) {
		if (touchArray[i] == touch) {
			return i;
		}
	}
	return -1;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch* touch in touches) {
		int touchId = [self getTouchIdAssignment];
		if (touchId != -1) {
			touchArray[touchId] = touch;
			touchIds[touchId] = 1;
			
			CGPoint pt = [touch locationInView:self.view];
			touchX[touchId] = pt.x / self.view.frame.size.width;
			touchY[touchId] = 1 - (pt.y / self.view.frame.size.height); // flip y value so zero is on 
			
			*touchXPtr[touchId] = touchX[touchId];
			*touchYPtr[touchId] = touchY[touchId];
			
			[self.csound sendScore:[NSString stringWithFormat:@"i1.%d 0 -2 %d", touchId, touchId, nil]];
		}
	}
    touchesCount += touches.allObjects.count;
    touchesLabel.text = [NSString stringWithFormat:@"Touches: %lu", touchesCount];
    
    if(touchesCount > [[event allTouches] count]) {
        touchesCount = [[event allTouches] count];
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch* touch in touches) {
		int touchId = [self getTouchId:touch];
		if (touchId != -1) {
			CGPoint pt = [touch locationInView:self.view];
			touchX[touchId] = pt.x / self.view.frame.size.width;
			touchY[touchId] = 1 - (pt.y / self.view.frame.size.height); // flip y value so zero is on bottom
		}
	}
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch* touch in touches) {
		int touchId = [self getTouchId:touch];
		if (touchId != -1) {
			touchIds[touchId] = 0;
			touchArray[touchId] = nil;
			[self.csound sendScore:[NSString stringWithFormat:@"i-1.%d 0 0 %d", touchId, touchId]];

		}
	}
    touchesCount -= touches.allObjects.count;
    touchesLabel.text = [NSString stringWithFormat:@"Touches: %lu", touchesCount];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
	for (UITouch* touch in touches) {
		int touchId = [self getTouchId:touch];
		if (touchId != -1) {
			touchIds[touchId] = 0;
			touchArray[touchId] = nil;
			[self.csound sendScore:[NSString stringWithFormat:@"i-1.%d 0 0", touchId, nil]];
			
		}
	}
}

@end
