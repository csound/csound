TclCsound examples

All examples should be run from this directory.

test.tcl: an example of a GUI controller for a csound instrument, uses cswish (or wish with Tclcsound module)
csound.tcl: a command-line csound frontend; if csound.tcl has executable permissions (the default), then 
  
  csound.tcl <args>

otherwise

  sh csound.tcl <args>
  
listener.tcl: a cstclsh csound server, which accepts tclcsound commands through a TCP socket on port 40001  
remote-gui.tcl: a client-server version of test.tcl, can be run from plain wish
remote.tcl: a client-server example, can be run from plain tclsh
remote-gui.pd: a PD client version of test.tcl, requires listener.tcl to be running under cstclsh
table.tcl: table writing example
graph.tcl: Tk table graphing example

VL, 2005



