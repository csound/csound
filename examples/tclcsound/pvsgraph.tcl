###################################################################
#  Copyright (C) 2006 Victor Lazzarini
#
#  This file is part of Csound.
#
#  The Csound Library is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  Csound is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with Csound; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307 USA
#
#  pvsgraph.tcl: Graph example using pvs out bus command
#
###################################################################

csPvsOut 0
labelframe .frame -text "Spectrum"
pack .frame
canvas .frame.mytab -height 100 -width 100
pack .frame.mytab

proc spectrum { height width points } {
    destroy .frame.mytab 
    canvas .frame.mytab -height $height -width $width
    pack .frame.mytab
    set mult [expr $height / 10000.0]
    set ratio [expr $width / $points]
    set x2 0.0
    set y2 0.0
    set en 3.0
    set bin 0
    for { set cnt 0 } { $cnt < $width } { incr cnt $ratio } {
	set st $en
	set en [expr [csPvsOutGet 0 $bin] * $mult + 3]
	for { set cnt2 0 } { $cnt2 < $ratio } { incr cnt2 } {
            set x1   $x2
            set y1   $y2
	    set x2   [expr $cnt2 + $cnt]
            set y2   [expr $height - ($st + $cnt2*($en - $st)/$ratio)]
	    .frame.mytab create line $x1 $y1 $x2 $y2 -fill blue
	}
     incr bin
    }
}
