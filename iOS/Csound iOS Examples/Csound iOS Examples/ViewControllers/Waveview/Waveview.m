/* 
 
 Waveview.m:
 
 Copyright (C) 2011 Steven Yi, Ed Costello
 
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

#import "Waveview.h"
#import "CsoundObj.h"

@implementation Waveview

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
    
    }
    return self;
}


// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.


- (void)drawRect:(CGRect)rect
{

	CGContextRef context = UIGraphicsGetCurrentContext();
//	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB()
	CGContextSetRGBFillColor(context, 0, 0, 0, 1);
    CGContextFillRect(context, rect);
	
	if (tableLoaded) {
		
		int width = self.frame.size.width;
		
		CGContextSetRGBStrokeColor(context, 255, 255, 255, 1);
		CGContextSetRGBFillColor(context, 255, 255, 255, 1);
		CGMutablePathRef fill_path = CGPathCreateMutable();
		CGFloat x = 0;
		CGFloat y = displayData[0];
		
		CGPathMoveToPoint(fill_path, &CGAffineTransformIdentity, x, y);
		
		for(int i = 1; i < width; i++) {
			CGPathAddLineToPoint(fill_path, &CGAffineTransformIdentity, i, displayData[i]);
			//NSLog(@"%d: %f", index, val);
										  
		}
	
		CGContextAddPath(context, fill_path);	
		CGContextSetAllowsAntialiasing(context, YES);
		CGContextDrawPath(context, kCGPathStroke);
		CGPathRelease(fill_path);
	}
}

- (void)setup:(CsoundObj *)csoundObj
{
	tableLoaded = NO;
	csObj = csoundObj;

	
	
}

- (void)updateValuesToCsound;
{
	
}

- (void)updataDisplayData
{
	int width = self.frame.size.width;
	int height = self.frame.size.height;
	int middle = height / 2;

	@autoreleasepool {


		displayData = malloc(sizeof(MYFLT) * width);
		
			
		for(int i = 0; i < width; i++) {
			float percent = i / (float)(width);
			int index = (int)(percent * tableLength);
			displayData[i] = (table[index] * middle) + middle;
			
			//NSLog(@"%d: %f", index, val);
			
		}
		
		
		[self performSelectorOnMainThread:@selector(setNeedsDisplay) withObject:nil waitUntilDone:NO];
	
	}

}

- (void)updateValuesFromCsound
{
	if (!tableLoaded) {
	
		@autoreleasepool {

			CSOUND *cs = [csObj getCsound];
			sr = csoundGetSr(cs);
			ksmps = csoundGetKsmps(cs);
			
					
				if ((tableLength = csoundTableLength(cs, 1)) > 0) {
					
					table = malloc(tableLength * sizeof(MYFLT));
					csoundGetTable(cs, &table, 1);
					tableLoaded = YES;
					[self performSelectorInBackground:@selector(updataDisplayData) withObject:nil];
				}

		}
	}
}

- (void)cleanup
{
}


 
@end
