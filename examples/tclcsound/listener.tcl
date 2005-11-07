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
# listener.tcl: socket server example, will respond to
# string commands of clients connected to port 40001
#
###################################################################

set forever 0
set client 0
set clientname 0
set portno 0

proc Disconnect { } {
global forever client server clientname portno
set forever 1
close $client
puts "Csound server: disconnected from client $clientname on port $portno (channel $client)"
close $server
puts "Csound server: exiting..."
}

proc ChanEval { chan } {
global client
if { [catch { set rtn [eval [gets  $chan]]} err] } {
puts "Error: $err"
} else {
puts $client $rtn
flush $client
}
}

proc NewChan { chan host port } {
global client clientname portno
puts "Csound server: connected to client $host on port $port (channel $chan)"
fileevent $chan readable [list ChanEval $chan]
set client $chan
set clientname $host
set portno $port
}

set server [socket -server NewChan 40001]
set sinfo  [fconfigure $server -sockname]
puts "Csound server: ready for tcp connections on port [lindex $sinfo 2]"
vwait forever
