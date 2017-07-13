/*
 
 CsoundLabelBinding.h:
 
 Copyright (C) 2014 Aurelius Prochazka
 
 This file is part of Csound for iOS.
 
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
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "CsoundObj.h"

@interface CsoundLabelBinding : NSObject<CsoundBinding>

-(instancetype)initLabel:(UILabel *)label channelName:(NSString *)channelName;

@property int precision;

@end
