# script to test cswish
# use csOpcodedir to set OPCODEDIR in your system
# if needed
# Tcl/Tk 8.4
# VL, 2005

global filename fval aval ffval qaval
set aval 1000
set fval 300
set filename "test_sliders.csd"
csCompile -odac -iadc -d $filename
csInChannel freq
csInChannel amp
csInChannel filfreq
csInChannel filq
csOutChannel freq

labelframe .middle -text "csound control panel" -padx 20 -pady 20
frame .bottom -padx 20 -pady 20
frame .sliders -padx 20 -pady 20

proc newOrc { } {
global filename
set filename [tk_getOpenFile -filetypes "{{CSD} {.csd} TEXT}"]
if { $filename != "" } {
csStop
csCompile -odac -iadc -d $filename
}
}

pack .middle .sliders .bottom
button .bottom.open -text "open new csd" -command newOrc
button .middle.run -text "play" -command csPlay
button .middle.stop -text "stop" -command csStop
button .middle.pause -text "pause" -command csPause
button .bottom.quit -text "quit" -command exit
scale .sliders.freq -variable fval -orient horizontal -from 200 -to 400 -command [list csInValue freq] -label "frequency" -length 300
scale .sliders.amp -variable aval -orient horizontal -from 0 -to 10000 -command [list csInValue amp]   -label "amplitude" -length 300
scale .sliders.ffreq -variable ffval -orient horizontal -from 200 -to 4000 -command [list csInValue filfreq]  -label "filter cf" -length 300
scale .sliders.q -variable qval -orient horizontal -from 1 -to 100 -command [list csInValue filq]   -label "filter Q" -length 300
pack .middle.run .middle.pause .middle.stop -side left
pack .bottom.open   .bottom.quit -side left
pack .sliders.amp .sliders.freq .sliders.ffreq .sliders.q

