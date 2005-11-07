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
# remote-gui.tcl: script to test remote execution of tclcsound
# through socket connections; requires listener.tcl
# Requires Tcl/Tk lib 8.4
#
###################################################################

exec ../../cstclsh listener.tcl &
exec sleep 1
set aval 1000
set fval 300
set filename "test_sliders.csd"
set grph 0
set chan [socket localhost 40001]

proc NetMess { chann mess } {
global chan
puts $chann $mess
flush $chann
return [gets $chan]
}

NetMess $chan [list csCompile -odac -iadc -d $filename -b512 -B512]
NetMess $chan [list csInChannel freq]
NetMess $chan [list csInChannel amp]
NetMess $chan [list csInChannel filfreq]
NetMess $chan [list csInChannel filq]
NetMess $chan [list csOutChannel freq]
NetMess $chan [list source graph.tcl]

labelframe .middle -text "csound control panel" -padx 20 -pady 20
frame .bottom -padx 20 -pady 20
frame .sliders -padx 20 -pady 20

proc newOrc { } {
global filename grph .table chan
set filename [tk_getOpenFile -filetypes "{{CSD} {.csd} TEXT}"]
if { $filename != "" } {
NetMess $chan [list csStop]
NetMess $chan [list csCompile -odac -iadc -d $filename -b512 -B512]
if { $grph == 1} {
destroy .table
} 
}
}

proc GraphTable {window height width ftable} {
global chan
set tbsize [NetMess $chan [list csGetTableSize 1]]
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
set y1  [expr [NetMess $chan [list csGetTable 1 $cnt]] * $mult - $mult]
set y2  [expr [NetMess $chan [list csGetTable 1 $nxt]] * $mult - $mult]    
.table.frame.mytab create line $x1 $y1 $x2 $y2
}
}

proc newGraph { } {
global grph .table chan
if { [NetMess $chan [list csGetTableSize 1]] != -1} {
if { $grph == 1 } {
destroy .table
}
toplevel .table -padx 4 -pady 2
GraphTable .table 200 400 1
set grph 1
}
}

proc Quit { } {
global chan
NetMess $chan [list Disconnect]
exit
}

proc SliderCmd { name val } {
global chan
NetMess $chan [list csInValue $name $val]
}

pack .middle .sliders .bottom
button .bottom.open -text "open new csd" -command newOrc
button .middle.run -text "play" -command [list NetMess $chan [list csPlay]]
button .middle.stop -text "stop" -command [list NetMess $chan [list csStop]]
button .middle.pause -text "pause" -command [list NetMess $chan [list csPause]]
button .bottom.graph -text "graph" -command newGraph
button .bottom.quit -text "quit" -command Quit
scale .sliders.freq -variable fval -orient horizontal -from 200 -to 400 -command [list SliderCmd freq] -label "frequency" -length 300
scale .sliders.amp -variable aval -orient horizontal -from 0 -to 10000 -command [list SliderCmd amp]   -label "amplitude" -length 300
scale .sliders.ffreq -variable ffval -orient horizontal -from 200 -to 4000 -command [list SliderCmd filfreq]  -label "filter cf" -length 300
scale .sliders.q -variable qval -orient horizontal -from 1 -to 100 -command [list SliderCmd filq]  -label "filter Q" -length 300
pack .middle.run .middle.pause .middle.stop -side left
pack .bottom.open .bottom.graph  .bottom.quit -side left
pack .sliders.amp .sliders.freq .sliders.ffreq .sliders.q

