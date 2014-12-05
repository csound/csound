/* 
 
 AudioFileTestViewController.h:
 
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

#import <UIKit/UIKit.h>
#import "BaseCsoundViewController.h"
#import "ControlKnob.h"

@interface AudioFileTestViewController : BaseCsoundViewController

@property (weak, nonatomic)	IBOutlet UIButton *playButton;
@property (weak, nonatomic)	IBOutlet ControlKnob *pitchKnob;
@property (weak, nonatomic)	IBOutlet UILabel *pitchLabel;

- (IBAction)play:(UIButton *)sender;
- (IBAction)changePitch:(ControlKnob *)sender;

@end
