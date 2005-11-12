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
# gen.tcl: a simple script example integrating instrument
# construction and performance. If using with standard tcl
# interpreters, uncomment the relevant 'load' command line. If OPCODEDIR
# is not set, edit and uncomment the 'csOpcodedir line'
#
###################################################################

# OSX:
#load ../../tclcsound.dylib Tclcsound
# Windows:
#load ../../tclcsound.dll Tclcsound
# Linux:
#load ../../tclcsound.so Tclcsound

#csOpcodedir /Users/victor/csound5

set orcfile "tcl.orc"
set scofile "tcl.sco"
set orc [open $orcfile w]
set sco [open $scofile w]

proc MakeIns { no code } {
global orc sco
puts $orc "instr $no"
puts $orc $code
puts $orc "endin"
}

append ins "asum init 0 \n"
append ins "ifreq = p5 \n"
append ins "iamp = p4 \n"

for { set i 0 } { $i < 10 } { incr i } {
    append ins "a$i  oscili iamp, ifreq+ifreq*[expr $i * 0.002], 1\n"
}

for { set i 0 } {$i < 10 } { incr i } {
    if { $i } {
         append ins " + a$i"
    } else {
         append ins "asum = a$i "
    }
}
append ins "\nk1 linen 1, 0.01, p3, 0.1 \n"
append ins "out asum*k1"

MakeIns 1 $ins

puts $sco  "f0 10"

close $orc
close $sco

csCompile $orcfile $scofile -odac -d -m0
csTable 1 0 16384 10 1 .5 .25 .2 .17 .15 .12 .1 .05
for {set i 0} { $i < 60 } { incr i } {
    csNote 1 [expr $i * 0.1] .5 [expr ($i * 10) + 500] [expr 100 + $i * 10]
}
csPerform

