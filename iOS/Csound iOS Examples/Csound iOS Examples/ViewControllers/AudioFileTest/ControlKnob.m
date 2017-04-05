/* 
 
 ControlKnob.m:
 
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

#import "ControlKnob.h"
#import "CsoundObj.h"

@interface ControlKnob ()
{
    float channelValue;
    float *channelPtr;

	CGFloat angle;
    CGPoint lastTouchPoint;
}

@end

@implementation ControlKnob

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setBackgroundColor:[UIColor clearColor]];
        _defaultValue = _value = 0.0f;
        _minimumValue = 0.0f;
        _maximumValue = 1.0f;
        angle = 0;
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self setBackgroundColor:[UIColor clearColor]];
        _defaultValue = _value = 0.0f;
        _minimumValue = 0.0f;
        _maximumValue = 1.0f;
        angle = 0;
    }
    return self;
}

/* This method is inaccurate */
- (void)setValue:(Float32)value_
{
    _value = value_;
    angle = (_value/_maximumValue) * 270.0f;
    [self setNeedsDisplay];
}

#pragma mark - UIControl Overrides

- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    CGPoint touchPoint = [touch locationInView:[self superview]];
    if (touchPoint.y < lastTouchPoint.y) {
        angle += angle < 270 ? 5 : 0;
    } else {
        angle -= angle > 0 ? 5 : 0;
    }
    _value = _minimumValue + angle/270.0f * (_maximumValue - _minimumValue);
    lastTouchPoint = touchPoint;
    [self setNeedsDisplay];
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    return YES;
}

- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    CGPoint touchPoint = [touch locationInView:[self superview]];
    if (touchPoint.y < lastTouchPoint.y) {
        angle += angle < 270 ? 5 : 0;
    } else {
        angle -= angle > 0 ? 5 : 0;
    }
    _value = _minimumValue + angle/270.0f * (_maximumValue - _minimumValue);
    lastTouchPoint = touchPoint;
    [self setNeedsDisplay];
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    return YES;
}

- (void)cancelTrackingWithEvent:(UIEvent *)event
{
    
}

#pragma mark - Drawing

- (void)drawRect:(CGRect)rect
{    
    self.transform = CGAffineTransformMakeRotation(angle*M_PI/180.0f);
    if (angle >= 360) {
        angle -= 360.0f;
    }
    
    // Get the context
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	// Flip coordinate system
	CGContextTranslateCTM(context, 0, rect.size.height);
	CGContextScaleCTM(context, 1.0, -1.0);
	
    // Draw circle
    CGFloat redComponents[] = {1.0f, 0.1f, 0.0f, 1.0f};
	CGColorRef redColor = CGColorCreate(colorSpace, redComponents);
	CGContextSetFillColorWithColor(context, redColor);
    CGContextAddEllipseInRect(context, rect);
    CGContextFillEllipseInRect(context, rect);
    CGColorRelease(redColor);
        
    // Draw line
    CGContextMoveToPoint(context, rect.size.width/4.0f, rect.size.height/4.0f);
    CGContextAddLineToPoint(context, rect.size.width/2.0f, rect.size.height/2.0f);
    CGFloat blackComponents[] = {0.0f, 0.0f, 0.0f, 1.0f};
	CGColorRef blackColor = CGColorCreate(colorSpace, blackComponents);
	CGContextSetStrokeColorWithColor(context, blackColor);
    CGContextStrokePath(context);
    CGColorRelease(blackColor);
    
    CGColorSpaceRelease(colorSpace);
}

#pragma mark - Csound Binding

- (void)setup:(CsoundObj *)csoundObj
{
	channelPtr = [csoundObj getInputChannelPtr:@"pitch" channelType:CSOUND_CONTROL_CHANNEL];
    channelValue = _value;
    [self addTarget:self
             action:@selector(updateChannelValue:)
   forControlEvents:UIControlEventValueChanged];
}

- (void)updateChannelValue:(id)sender
{
	channelValue = ((ControlKnob *)sender).value;
}

- (void)updateValuesToCsound
{
	*channelPtr = channelValue;
}

- (void)cleanup
{
	[self removeTarget:self
                action:@selector(updateChannelValue:)
      forControlEvents:UIControlEventValueChanged];
}

@end
