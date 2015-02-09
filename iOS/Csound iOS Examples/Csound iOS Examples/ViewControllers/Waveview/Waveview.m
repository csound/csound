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
{
    BOOL tableLoaded;
    CGFloat lastY;
    CsoundObj *csObj;
    MYFLT *table;
    int tableLength;
    MYFLT *displayData;
}

- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
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

- (void)updataDisplayData
{
    float yScalingFactor = 0.8;

    int width = self.frame.size.width;
    int height = self.frame.size.height;
    int middle = height / 2;
    
    displayData = malloc(sizeof(MYFLT) * width);
    
    for(int i = 0; i < width; i++) {
        float percent = i / (float)(width);
        int index = (int)(percent * tableLength);
        displayData[i] = (table[index] * middle) + middle;
    }
    
    [self performSelectorOnMainThread:@selector(setNeedsDisplay)
                           withObject:nil
                        waitUntilDone:NO];
}

- (void)updateValuesFromCsound
{
    if (!tableLoaded) {
        CSOUND *cs = [csObj getCsound];
        
        if ((tableLength = csoundTableLength(cs, 1)) > 0) {
            table = malloc(tableLength * sizeof(MYFLT));
            csoundGetTable(cs, &table, 1);
            tableLoaded = YES;
            [self performSelectorInBackground:@selector(updataDisplayData) withObject:nil];
        }
    }
}

@end
