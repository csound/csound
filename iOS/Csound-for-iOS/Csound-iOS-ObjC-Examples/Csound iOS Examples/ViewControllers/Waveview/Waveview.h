/* 
 
 Waveview.h:
 
 Copyright (C) 2015 Steven Yi, Ed Costello, Aurelius Prochazka
 
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1335, USA
 
 */

#import <UIKit/UIKit.h>
#import "CsoundObj.h"

@interface Waveview : UIView <CsoundBinding>

@property (nonatomic, strong) NSString *channelName;

- (void)displayFTable:(int)fTableNum;

@end
