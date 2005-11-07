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
# test.tcl: script to test cswish, and tclcsound commands example
# use csOpcodedir to set OPCODEDIR in your system if needed
# Requires Tcl/Tk 8.4
#
###################################################################

source graph.tcl
set aval 1000
set fval 300
set filename "test_sliders.csd"
set grph 0
csCompile -odac -iadc -d $filename -b512 -B512
csInChannel freq
csInChannel amp
csInChannel filfreq
csInChannel filq
csOutChannel freq

labelframe .middle -text "csound control panel" -padx 20 -pady 20
frame .bottom -padx 20 -pady 20
frame .sliders -padx 20 -pady 20

proc newOrc { } {
global filename grph .table
set filename [tk_getOpenFile -filetypes "{{CSD} {.csd} TEXT}"]
if { $filename != "" } {
csStop
csCompile -odac -iadc -d $filename -b512 -B512
if { $grph == 1} {
destroy .table
}
}
}

proc newGraph { } {
global grph .table
if { [csGetTableSize 1] != -1} {
if { $grph == 1 } {
destroy .table
}
toplevel .table -padx 4 -pady 2
GraphTable .table 200 400 1
set grph 1
}
}

pack .middle .sliders .bottom
button .bottom.open -text "open new csd" -command newOrc
button .middle.run -text "play" -command csPlay
button .middle.stop -text "stop" -command csStop
button .middle.pause -text "pause" -command csPause
button .bottom.quit -text "quit" -command exit
button .bottom.graph -text "graph" -command newGraph
scale .sliders.freq -variable fval -orient horizontal -from 200 -to 400 -command [list csInValue freq] -label "frequency" -length 300
scale .sliders.amp -variable aval -orient horizontal -from 0 -to 10000 -command [list csInValue amp]   -label "amplitude" -length 300
scale .sliders.ffreq -variable ffval -orient horizontal -from 200 -to 4000 -command [list csInValue filfreq]  -label "filter cf" -length 300
scale .sliders.q -variable qval -orient horizontal -from 1 -to 100 -command [list csInValue filq]   -label "filter Q" -length 300
pack .middle.run .middle.pause .middle.stop -side left
pack .bottom.open .bottom.graph  .bottom.quit -side left
pack .sliders.amp .sliders.freq .sliders.ffreq .sliders.q

