###################################################################
#  Copyright (C) 2005 Victor Lazzarini
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
#  graph.tcl: Graph example using table commands
#
###################################################################

proc GraphTable {window height width ftable} {
set tbsize [csGetTableSize 1]
labelframe $window.frame -text "function table $ftable: $tbsize points"
pack $window.frame
canvas $window.frame.mytab -height $height -width $width
pack $window.frame.mytab
set inc [expr $tbsize / $width]
set mult [expr $height / -2.01]
for {set cnt 0 } {$cnt < [expr $tbsize - 1] } {incr cnt $inc} {
set nxt [expr $cnt + 1]
set x1  [expr $cnt / $inc]
set x2  [expr $nxt / $inc]
set y1  [expr [csGetTable 1 $cnt] * $mult - $mult]
set y2  [expr [csGetTable 1 $nxt] * $mult - $mult]
.table.frame.mytab create line $x1 $y1 $x2 $y2
}
}
