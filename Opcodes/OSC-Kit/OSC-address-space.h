/*
Copyright (c) 1998.  The Regents of the University of California (Regents).
All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for educational, research, and not-for-profit purposes, without
fee and without a signed licensing agreement, is hereby granted, provided that
the above copyright notice, this paragraph and the following two paragraphs
appear in all copies, modifications, and distributions.  Contact The Office of
Technology Licensing, UC Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley,
CA 94720-1620, (510) 643-7201, for commercial licensing opportunities.

Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.

     IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
     ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
     REGENTS HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

     REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
     DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS".
     REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
     ENHANCEMENTS, OR MODIFICATIONS.

The OpenSound Control WWW page is 
    http://www.cnmat.berkeley.edu/OpenSoundControl
*/

/*
  OSC-address-space.h
  Matt Wright, 11/20/97
  Version 2.0 5/28/98

  C interface for registering the nodes in the OSC address space for an
  application.

  include OSC-timetag.h before this file

****************************** Introduction ******************************


  The model is based on our original C++ design and consists of two kinds of
  objects:

    methods are the leaf nodes of the address space hierarchy.  A complete OSC
    address corresponds to a particular method, which has a corresponding
    callback procedure that will be invoked to implement commands sent by an
    OSC client.

    containers are branch nodes of the address space hierarchy, and contain
    methods and other containers.  Each container has a single namespace;
    it cannot contain a method and a subcontainer with the same name.

  For example, let's examine the OSC message "/resonators/foo/decay 2.5".  The
  address of this message is "/resonators/foo/decay" and the message has a
  single argument, 2.5.  We'd say that the object corresponding to the prefix
  "/resonators" is a container, and that it contains another container whose
  address is "/resonators/foo".  The "/resonators/foo" container has a method
  "decay".

  The memory model used by this module is pre-allocation of fixed-size objects
  for containers, methods, and other internal objects.  This preallocated
  memory is dynamically managed internally by a custom high-performance memory
  allocator.  When the preallocated memory runs out, this module calls an
  optional realtime memory allocator that you provide.  If your memory allocator
  gives this module more memory, it will add it to the pool of objects and
  never free the memory.  If your system does not have a realtime memory 
  allocator, provide a procedure that always returns 0.
*/

/*************************** Type Definitions  ******************************/

/* Users of this module don't get to see what's inside these objects */
typedef struct OSCContainerStruct *OSCcontainer;
typedef struct OSCMethodStruct *OSCMethod;

/* This makes it easy to change the way we represent symbolic names: */
typedef const char *Name;


/************************ Initialization and Memory  ************************/

/* You will fill an OSCAddressSpaceMemoryTuner struct with the parameters that
   determine how memory will be allocated.

   initNumContainers is the number of containers that will be allocated at
   initialization time.  This should be the maximum number of containers you
   ever expect to have in your address space.

   initNumMethods is the number of methods that will be allocated at
   initialization time.  If you register the same method callback procedure
   multiple times at different places in the address space, each of these
   locations counts as a separate method as far as memory allocation is
   concerned.

   The MemoryAllocator fields are procedures you will provide that allocate
   memory.  Like malloc(), they take the number of bytes as arguments and return
   either a pointer to the new memory or 0 for failure.  This memory will never
   be freed.

   The InitTimeMemoryAllocator will be called only at initialization time,
   i.e., before OSCInitAddressSpace() returns.  If it ever returns 0, that's
   a fatal error.

   The RealTimeMemoryAllocator will be called if, while the application is
   running, the address space grows larger than can fit in what was allocated
   at initialization time.  If the RealTimeMemoryAllocator() returns 0, the
   operation attempting to grow the address space will fail.  If your system
   does not have real-time memory allocation, RealTimeMemoryAllocator should
   be a procedure that always returns 0.
*/

struct OSCAddressSpaceMemoryTuner {
    int initNumContainers;
    int initNumMethods;
    void *(*InitTimeMemoryAllocator)(int numBytes);
    void *(*RealTimeMemoryAllocator)(int numBytes);
};

/* Given an OSCAddressSpaceMemoryTuner, return the number of bytes of
   memory that would be allocated if OSCInitAddressSpace() were called
   on it. */
int OSCAddressSpaceMemoryThatWouldBeAllocated(struct OSCAddressSpaceMemoryTuner *t);


/* Call this before you call anything else.  It returns the container that
   corresponds to the address "/" and is the root of the address space tree. 
*/
OSCcontainer OSCInitAddressSpace(struct OSCAddressSpaceMemoryTuner *t);


/**************************** Containers  ****************************/

/* Here's how this system deals with the standard OSC queries addressed to
   containers.  This module handles the details of listening for these queries
   and returning a correctly-formatted answer; all it needs from you is the
   actual data that constitute the answers to these queries.

   You pass this data in an OSCContainerQueryResponseInfo structure.  Future versions
   of this module may have new kinds of queries that they can deal with, so
   the list of fields in this structure may grow.  That's why your code should
   call OSCInitContainerQueryResponseInfo() on your struct before putting new values
   into it; this procedure will initialize all of the fields to 0, meaning
   "don't have that information", which will cause the associated queries to
   fail.

   The "null" message, i.e., a message with a trailing slash, requesting the
   list of addresses under a given container, is handled automatically.
*/

struct OSCContainerQueryResponseInfoStruct {
    char *comment;
    /* Other fields may go here */
};

void OSCInitContainerQueryResponseInfo(struct OSCContainerQueryResponseInfoStruct *i);


/* Allocate a new container and initialize it.  Returns 0 if it cannot
   allocate a new container, e.g., if you've exceeded the initNumContainers
   limit of the OSCAddressSpaceMemoryTuner() and the RealTimeMemoryAllocator()
   didn't return any new memory.

   This procedure doesn't make a copy of the name string or any of the
   contents of the OSCContainerQueryResponseInfoStruct.  It does copy the fields
   of the OSCContainerQueryResponseInfoStruct.
*/
OSCcontainer OSCNewContainer(Name name, OSCcontainer parent,
			     struct OSCContainerQueryResponseInfoStruct *queryInfo);


/* Remove a container from the address space.  This also removes all the
   container's methods and recursively removes all sub-containers.  Memory
   freed by removing a container is kept in this module's internal pool. 
*/
void OSCRemoveContainer(OSCcontainer container);


/* Given a pointer to a container, and another name for that container, add or
   remove that name as an alias for the container.  Return FALSE for failure. */
OSCBoolean OSCAddContainerAlias(OSCcontainer container, Name otherName);
OSCBoolean OSCRemoveContainerAlias(OSCcontainer container, Name otherName);


/* Write the OSC address of the given container into the given string.
   Return FALSE if the address won't fit in the string. */
OSCBoolean OSCGetAddressString(char *target, int maxLength, OSCcontainer c);


/* Given an address (not a pattern!), return the single OSCcontainer it names,
   or 0 if there is no container at that address */
OSCcontainer OSCLookUpContainer(Name address);


/**************************** Methods  ****************************/

/* A methodCallback is a procedure that you write that will be called at the
   time that an OSC message is to take effect.  It will be called with 5
   arguments:
     - A context pointer that was registered with the methodNode
       this is a method of.  (Something like the C++ "this" pointer.)
     - The number of bytes of argument data
     - A pointer to the argument portion of the OSC message
     - The time tag at which this message is supposed to take effect.
     - A "return address" object you can use to send a message back to the
       client that sent this message.  This return channel is guaranteed
       to be usable only during the invocation of your method, so your method
       must use the return address immediately or ignore it, not store it away
       for later use.
*/
typedef const struct NetworkReturnAddressStruct *const NetworkReturnAddressPtr;
typedef void (*methodCallback)(void *context, int arglen, const void *args, 
			       OSCTimeTag when, NetworkReturnAddressPtr returnAddr);


/* A ParamValQuerier is a procedure that the OSC system will call when the
   user wants to know the current value of a parameter.  It will be passed the
   same context pointer as the associated method.  It should write its return
   value in the given buffer in the same format as the associated
   methodCallback would expect its "args" argument, and should return the
   length of data just like the method would expect in its "arglen" argument.
   It doesn't have to worry about the address portion of the OSC message.
*/
typedef char OSCData;	/* For pointers to OSC-formatted data */
typedef int (*ParamValQuerier)(OSCData *result, void *context);


/* This system deals with other standard per-method queries, such as
   documentation, valid parameter types and ranges, units, default values,
   etc., pretty much just like per-container queries.
*/

struct OSCMethodQueryResponseInfoStruct {
    char *description;
    ParamValQuerier pvq;
    /* For each argument of the method:
	min, max, default, units */
};

void OSCInitMethodQueryResponseInfo(struct OSCMethodQueryResponseInfoStruct *i);


/* Allocate a new method, initialize it, and add it to a container. Returns 0
   for failure, e.g., if you've exceeded the initNumMethods limit of the
   OSCAddressSpaceMemoryTuner() and the RealTimeMemoryAllocator() didn't return any
   new memory.

   This procedure doesn't make a copy of the name string or any of the
   contents of the OSCMethodQueryResponseInfoStruct.
*/

OSCMethod OSCNewMethod(Name name, OSCcontainer container, methodCallback meth, 
		       void *context, struct OSCMethodQueryResponseInfoStruct *queryInfo);



/******************************* Debug  ********************************/


#ifdef DEBUG
void OSCPrintWholeAddressSpace(void);
#endif


/**************************** Sample Code  *****************************/

/* Here's a gross approximation of how your application will invoke the
   procedures in this module.  It registers an address space with
   containers with addresses "/foo", "/foo/foochild", and "/bar",
   and gives each of them "play" and "shuddup" messages.


#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"


typedef struct {
    int playing;
    int param;
    float otherParam;
} Player;

void PlayCallback(void *context, int arglen, const void *vargs, 
		  OSCTimeTag when, NetworkReturnAddressPtr ra) {
    Player *p = context;
    const int *args = vargs;
    

    p->playing = 1;
    if (arglen >= 4) {
	p->param = args[0];
    }
}

void ShuddupCallback(void *context, int arglen, const void *vargs,
		     OSCTimeTag when, NetworkReturnAddressPtr ra) {
    Player *p = context;
    const float *args = vargs;
    

    p->playing = 0;
    if (arglen >= 4) {
	p->otherParam = args[0];
    }
}

void *InitTimeMalloc(int numBytes) {
    return malloc(numBytes);
}

void *NoRealTimeMalloc(int numBytes) {
    return 0;
}

main() {
    struct OSCAddressSpaceMemoryTuner oasmt;
    OSCcontainer topLevelContainer, foo, foochild, bar;
    struct OSCContainerQueryResponseInfoStruct ocqris;
    struct OSCMethodQueryResponseInfoStruct omqris;

    Player *players;

    players = (Player *) malloc(3 * sizeof(*players));
    if (!players) exit(1);

    oasmt.initNumContainers = 10;
    oasmt.initNumMethods = 10;
    oasmt.InitTimeMemoryAllocator = InitTimeMalloc;
    oasmt.RealTimeMemoryAllocator = NoRealTimeMalloc;

    topLevelContainer = OSCInitAddressSpace(&oasmt);

    OSCInitContainerQueryResponseInfo(&ocqris);
    ocqris.comment = "Foo for you";
    foo = OSCNewContainer("foo", topLevelContainer, &ocqris);

    OSCInitContainerQueryResponseInfo(&ocqris);
    ocqris.comment = "Beware the son of foo!";
    foochild = OSCNewContainer("foochild", foo, &ocqris);

    OSCInitContainerQueryResponseInfo(&ocqris);
    ocqris.comment = "Belly up to the bar";
    bar = OSCNewContainer("bar", topLevelContainer, &ocqris);

    if (foo == 0 || foochild == 0 || bar == 0) {
	fprintf(stderr, "Problem!\n");
	exit(1);
    }

    OSCInitMethodQueryResponseInfo(&omqris);
    OSCNewMethod("play", foo, PlayCallback, &(players[0]), &omqris);
    OSCNewMethod("shuddup", foo, ShuddupCallback, &(players[0]), &omqris);

    OSCNewMethod("play", foochild, PlayCallback, &(players[1]), &omqris);
    OSCNewMethod("shuddup", foochild, ShuddupCallback, &(players[1]), &omqris);

    OSCNewMethod("play", bar, PlayCallback, &(players[2]), &omqris);
    OSCNewMethod("shuddup", bar, ShuddupCallback, &(players[2]), &omqris);
}

*/
		 
