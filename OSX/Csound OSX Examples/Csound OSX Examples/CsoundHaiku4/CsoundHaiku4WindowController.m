/*
 
 CsoundHaiku4WindowController.m:
 
 Copyright (C) 2014 Aurelius Prochazka
 
 This file is part of Csound OSX Examples.
 
 The Csound for OSX Library is free software; you can redistribute it
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


#import "CsoundHaiku4WindowController.h"

@implementation CsoundHaiku4WindowController

- (IBAction)startPlayCSDFile:(id)sender {
	NSString *csdFile = [[NSBundle mainBundle] pathForResource:@"IV" ofType:@"csd"];
	
	[self.csound play:csdFile];
    
}

- (IBAction)stopPlayCSDFile:(id)sender {
    [self.csound stop];
}


@end
