There are a lot of files in this Kit, but only a few that you need to look at.

In general, the model is that the .h files you #include are full of
documentation and should explain everything you need to know, and the
associated .c files implement the functionality described in the
.h files.  You shouldn't need to read any of the .c files except for
the example test programs.

Here's a list of the files you need to know about and maybe do something with:


********** Files you may need to modify for your particular system **********

* NetworkReturnAddress.c

You will need to write this file to implement the
interface in NetworkReturnAddress.h based on whatever kind of networking your
system uses.


* OSC-drop.c

The Kit calls these procedures whenever it has to drop a packet, bundle, or
message because of some bad condition it encounters.  The default
implementation uses OSCWarning() to print a message, but you may want to do
something else.


* OSC-system-dependent.c

You need to implement the procedures in this file.  Currently it's just
different ways to print errors and warnings to the user.  The default
implementation prints these messages on stderr.  


* OSC-priority-queue.c

If you have a faster implementation of priority queues than heaps (which are
O(log(n)) insertion and removal), you can rewrite this.  (But you don't have
to.)


* OSC-timetag.[ch]

OSC timetags are unsigned 8-byte fixed point numbers.  Since some systems have
8 byte ints and some do not, there are two implementations of all the time tag
utilities.  Edit the .h file based on whether your system has 8 byte ints, and
to make typedefs for all the necessary integer types.  The .c file has a
procedure that returns the current time as an OSC timetag; you need to write
that for your system.



********** Interfaces to code in the Kit that your program will call **********

* OSC-address-space.h

Read this to learn how to set up the OSC address space for an application.
There's a lot of text there, but the basic idea is that there are "Containers"
and "Methods" that your application creates in a tree structure.


* OSC-receive.h

This is the interface between the OSC Kit and your scheduler.  It handles
receipt of OSC messages and declares some procedures that you need to call
when you're ready to invoke messages whose time has come or if you have some
spare CPU time and want to get a leg up on future processing.


* OSC-internal-messages.h

This allows the parts of your application to be able to communicate to each
other via internally-sent OSC messages.


* OSC-string-help.h

Utilities to make it easy to deal with OSC strings.  (OSC strings are regular
null-terminated ASCII strings, except that they have 0-3 extra null bytes so
that the total length is a multiple of 4 bytes.)



********** Test programs you may want to look at **********

There are a bunch of test wrapper programs that I used to debug the kit, and
that may be useful as examples.  Any .c file whose name begins "test" is one
of these.

