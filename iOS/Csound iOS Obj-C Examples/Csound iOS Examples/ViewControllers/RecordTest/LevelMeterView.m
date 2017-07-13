/* 
 
 LevelMeterView.m:
 
 Copyright (C) 2014 Thomas Hass, Aurelius Prochazka
 
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

#import "LevelMeterView.h"

@interface LevelMeterView () {
	float channelValue;
    float *channelPtr;
	CGFloat lastY;
	NSInteger ksmps, sr;
}
@property (nonatomic, strong) NSString *channelName;

@end

@implementation LevelMeterView

- (id)initWithFrame:(CGRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		channelValue = 0.0f;
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
	self = [super initWithCoder:aDecoder];
	if (self) {
		channelValue = 0.0f;
		lastY = -100;
	}
	return self;
}

- (void)addToCsoundObj:(CsoundObj *)csoundObj forChannelName:(NSString *)channelName_
{
	[csoundObj addBinding:self];
	self.channelName = channelName_;
}

- (void)drawRect:(CGRect)rect
{	
	// Round the corners
	UIBezierPath *clipPath = [UIBezierPath bezierPathWithRoundedRect:self.bounds
                                                   byRoundingCorners:UIRectCornerAllCorners 
                                                         cornerRadii:CGSizeMake(10.0, 10.0)];
    [clipPath addClip];
	
	// Get the context
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	// Flip coordinate system
	CGContextTranslateCTM(context, 0, rect.size.height);
	CGContextScaleCTM(context, 1.0, -1.0);
	
	// Draw background
	CGFloat grayComponents[] = {0.7f, 0.7f, 0.7f, 1.0f};
	CGColorRef grayColor = CGColorCreate(colorSpace, grayComponents);
	CGContextSetFillColorWithColor(context, grayColor);
	CGContextFillRect(context, self.bounds);
	CGColorRelease(grayColor);
	
	// Draw rects
	CGFloat width = CGRectGetMaxX(rect);
	CGFloat height = CGRectGetMaxY(rect);
	CGFloat squareWidth = width - 10;
	CGFloat squareHeight = height/30.0f;;
	
	CGFloat greenComponents[] = {0.0f, 1.0f, 0.0f, 1.0f};
	CGFloat yellowComponents[] = {221.0f/250.0f, 223.0f/250.0f, 14.0f/250.0f, 1.0f};
	CGFloat redComponents[] = {1.0f, 0.0f, 0.0f, 1.0f};
	CGColorRef greenColor = CGColorCreate(colorSpace, greenComponents);
	CGColorRef yellowColor = CGColorCreate(colorSpace, yellowComponents);
	CGColorRef redColor = CGColorCreate(colorSpace, redComponents);
	
	CGFloat x = 12.0f;
	
	// Draw the peak rect.
	if (lastY < (height * 0.7f)) {
		CGContextSetFillColorWithColor(context, greenColor);
	} else if (lastY < (height * 0.9f)) {
		CGContextSetFillColorWithColor(context, yellowColor);
	} else {
		CGContextSetFillColorWithColor(context, redColor);
	}
	CGContextMoveToPoint(context, x, lastY);
	CGContextAddLineToPoint(context, x, lastY + squareHeight);
	CGContextAddLineToPoint(context, x + squareWidth - 12.0f, lastY + squareHeight);
	CGContextAddLineToPoint(context, x + squareWidth - 12.0f, lastY);
	CGContextFillPath(context);
	
	// Draw the rest of the rects.
	for (int y = 12.0f; y < height * channelValue - 12.0f; y += squareHeight + 5.0f) {
		if (y < (height * 0.7f)) {
			CGContextSetFillColorWithColor(context, greenColor);
		} else if ((y < height * 0.9f)) {
			CGContextSetFillColorWithColor(context, yellowColor);
		} else {
			CGContextSetFillColorWithColor(context, redColor);
		}
		CGContextMoveToPoint(context, x, y);
		CGContextAddLineToPoint(context, x, y + squareHeight);
		CGContextAddLineToPoint(context, x + squareWidth - 12.0f, y + squareHeight);
		CGContextAddLineToPoint(context, x + squareWidth - 12.0f, y);
		CGContextFillPath(context);
		if (y > lastY) {
			lastY = y;
		}
	}
	CGColorRelease(greenColor);
	CGColorRelease(yellowColor);
	CGColorRelease(redColor);
	CGColorSpaceRelease(colorSpace);
	
	// Reset lastY periodically.
	static NSInteger count = 0;
	if (count % 100 == 0) {
		lastY = -squareHeight;
	}
	count++;
}

#pragma mark - Csound Binding

-(void)setup:(CsoundObj *)csoundObj
{
	channelPtr = [csoundObj getOutputChannelPtr:self.channelName channelType:CSOUND_AUDIO_CHANNEL];
	CSOUND *cs = [csoundObj getCsound];
	sr = csoundGetSr(cs);
	ksmps = csoundGetKsmps(cs);
}

-(void)updateValuesFromCsound
{
	channelValue = fabs(*channelPtr);
	
	static NSInteger count = 0;
	if (count % ((sr/ksmps)/20) == 0) {
		[self performSelectorOnMainThread:@selector(setNeedsDisplay) withObject:nil waitUntilDone:NO];
	}
	count++;
	if (count > INT_MAX) {
		count -= INT_MAX;
	}
}

- (void)cleanup
{
	channelValue = 0;
	lastY = -100;
	[self setNeedsDisplay];
}

@end
