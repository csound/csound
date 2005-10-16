# script to test cswish
# use csOpcodedir to set OPCODEDIR in your system
# if needed
# VL, 2005

global filename fval aval
set aval 1000
set fval 300
set filename "test_sliders.csd"
csCompile -odac -iadc -d $filename
csInChannel freq
csInChannel amp
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

proc slidval {val} {
csInValue freq $val
}

pack .middle .sliders .bottom
button .bottom.open -text "open new csd" -command newOrc
button .middle.run -text "play" -command csPlay
button .middle.stop -text "stop" -command csStop
button .middle.pause -text "pause" -command csPause
button .bottom.quit -text "quit" -command exit
scale .sliders.freq -variable fval -orient horizontal -from 200 -to 400 -command [list csInValue freq] -label "frequency" -length 300
scale .sliders.amp -variable aval -orient horizontal -from 0 -to 10000 -command [list csInValue amp]   -label "amplitude" -length 300
pack .middle.run .middle.pause .middle.stop -side left
pack .bottom.open   .bottom.quit -side left
pack .sliders.amp .sliders.freq

