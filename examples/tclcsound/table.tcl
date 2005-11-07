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
# table.tcl: table commands example
#
###################################################################


proc CosTab { ftn } {
set size [csGetTableSize $ftn]
set cnst [expr 6.283 /  $size]
for {set pos 0}  { $pos < $size}  { incr pos }  {
set ang [expr $pos * $cnst]
csSetTable $ftn $pos [expr cos($ang)]
}
csSetTable $ftn $size [csGetTable $ftn 0]
}