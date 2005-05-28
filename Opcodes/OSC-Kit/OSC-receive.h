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
  OSC-receive.h
  Matt Wright, 11/18/97

  include OSC-timetag.h and NetworkReturnAddress.h before this file.
*/

/**************************************************
   Initialization and memory pre-allocation
 **************************************************/

/* The memory model used by this module is pre-allocation of fixed-size
  objects for network buffers and other internal objects.  This preallocated
  memory is dynamically managed internally by a custom high-performance memory
  allocator.  When the preallocated memory runs out, this module calls an
  optional realtime memory allocator that you provide.  If your memory
  allocator gives this module more memory, it will add it to the pool of
  objects and never free the memory.  If your system does not have a realtime
  memory allocator, provide a procedure that always returns 0.

  You will fill an OSCReceiveMemoryTuner struct with the parameters that
  determine how memory will be allocated.

  The MemoryAllocator fields are procedures you will provide that allocate
  memory.  Like malloc(), they take the number of bytes as arguments and return
  either a pointer to the new memory or 0 for failure.  This memory will never
  be freed.

- The InitTimeMemoryAllocator will be called only at initialization time,
  i.e., before OSCInitAddressSpace() returns.  If it ever returns 0, that's
  a fatal error.

- The RealTimeMemoryAllocator will be called if, while the application is
  running, the address space grows larger than can fit in what was allocated
  at initialization time.  If the RealTimeMemoryAllocator() returns 0, the
  operation attempting to grow the address space will fail.  If your system
  does not have real-time memory allocation, RealTimeMemoryAllocator should
  be a procedure that always returns 0.

  The remaining fields say how much memory to allocate at initialization time:

- receiveBufferSize is the maximum packet size that can be received.  Is the
  maximum UDP packet size 4096?  OSC clients can send a query to this system
  asking for this maximum packet size.

- numReceiveBuffers determines how many packets at a time can be sitting
  on the scheduler with messages waiting to take effect.  If all the
  receive buffers are tied up like this, you won't be able to receive
  new packets.

- numQueuedObjects is the number of messages and packets that can be sitting
  on the scheduler waiting to take effect.

- Because a message pattern may be dispatched before the message takes effect,
  we need memory to store the callback pointers corresponding to a message.
  numCallbackListNodes is the number of callbacks that may be stored in this
  fashion.  It must be at least as large as the maximum number of methods that
  any one message pattern may match, but if you want to take advantage of
  pre-dispatching, this should be large enough to hold all the callbacks for
  all the messages waiting in the scheduler.

*/

struct OSCReceiveMemoryTuner {
    void *(*InitTimeMemoryAllocator)(int numBytes);
    void *(*RealTimeMemoryAllocator)(int numBytes);
    int receiveBufferSize;
    int numReceiveBuffers;
    int numQueuedObjects;
    int numCallbackListNodes;
};

/* Given an OSCReceiveMemoryTuner, return the number of bytes of
   memory that would be allocated if OSCInitReceive() were called
   on it. */
int OSCReceiveMemoryThatWouldBeAllocated(struct OSCReceiveMemoryTuner *t);

/* Returns FALSE if it fails to initialize */
OSCBoolean OSCInitReceive(struct OSCReceiveMemoryTuner *t);

/**************************************************
   Managing packet data structures
 **************************************************/

/* You don't get to know what's in an OSCPacketBuffer. */
typedef struct OSCPacketBuffer_struct *OSCPacketBuffer;

/* Get an unused packet.  Returns 0 if none are free.  If you get a packet
   with this procedure, it is your responsibility either to call
   OSCAcceptPacket() on it (in which case the internals of the OSC Kit free
   the OSCPacketBuffer after the last message in it takes effect) or to call
   OSCFreePacket() on it. */
OSCPacketBuffer OSCAllocPacketBuffer(void);

/* Free.  This is called automatically after the last message that was
   in the packet is invoked.  You shouldn't need to call this unless
   you get a packet with OSCAllocPacketBuffer() and then for some reason
   decide not to call OSCAcceptPacket() on it. */
void OSCFreePacket(OSCPacketBuffer p);

/* Whatever code actually gets packets from the network should use these
   three selectors to access the fields in the packet structure that need
   to be filled in with the data from the network. */

/* Selector to get the buffer from a packet.  This buffer's size will be
   equal to the receiveBufferSize you passed to OSCInitReceive(). */
char *OSCPacketBufferGetBuffer(OSCPacketBuffer p);

/* Selector to get a pointer to the int that's the size count for the
   data currently in a packet.  (Not the capacity of the packet's buffer,
   but the size of the packet that's actually stored in the buffer.) */
int *OSCPacketBufferGetSize(OSCPacketBuffer);

/* Selector to get the client's network address from a packet.  This buffer's
   size will be equal to the clientAddrSize you passed to OSCInitReceive().
   Note that the NetworkReturnAddressPtr type is full of "const"s, so your
   code that fills in the return address will probably have to cast the return
   value of this procedure to some non-const type to be able to write into it. */
NetworkReturnAddressPtr OSCPacketBufferGetClientAddr(OSCPacketBuffer p);

/* Returns the capacity of packet buffers (the receiveBufferSize you passed
   to OSCInitReceive()). */
int OSCGetReceiveBufferSize(void);

/**************************************************
   Dealing with OpenSoundControl packets and
   making the messages take effect.
 **************************************************/

/* Call this as soon as a packet comes in from the network.
   It will take care of anything that has to happen immediately,
   but put off as much as possible of the work of parsing the
   packet.  (This tries to be as fast as possible in case a
   lot of packets come in.) */
void OSCAcceptPacket(OSCPacketBuffer packet);

/* Call this during an otherwise idle time.  It goes through
   everything that's sitting in the OSC scheduler waiting to
   happen and does some of the work of parsing, pattern
   matching, dispatching, etc., that will have to be done
   at some point before the scheduled messages can take
   effect.

   The return value indicates whether there is more work of
   this sort that could be done.  (Each time you call this,
   it does only a small unit of this kind of work.  If it
   returns TRUE and you still have time before the next thing
   you have to do, call it again.) */
OSCBoolean OSCBeProductiveWhileWaiting(void);

/* Call this whenever enough time has passed that you want to see which
   messages are now ready and have them take effect.  (For example, in a
   synthesizer, you might call this once per synthesis frame, just before
   synthesizing the audio for that frame.)

   This procedure finds the earliest time tag of all the queued messages
   and invokes *all* of the queued messages with that time tag.  (OSC
   guarantees that messages with the same tag take effect atomically.)
   If there are more messages that are ready, but with a different time
   tag, this procedure does not invoke them, but returns TRUE to indicate
   that more messages are ready.
*/
OSCBoolean OSCInvokeMessagesThatAreReady(OSCTimeTag now);

/* Same thing, but invokes all of the messages whose time has come. */
void OSCInvokeAllMessagesThatAreReady(OSCTimeTag now);

/**************************************************
   How to use this stuff
 **************************************************/

/* Here's a gross approximation of how your application will invoke the
   procedures in this module:

while (1) {
    OSCTimeTag now = CurrentTime();
    do {
        if (WeAreSoLateThatWeNeedToDelayOSCMessagesToAvoidACrisis()) break;
    } while (OSCInvokeMessagesThatAreReady(now) == TRUE);

    SynthesizeSomeSound();
    if (NetworkPacketWaiting()) {
        OSCPacketBuffer p = OSCAllocPacketBuffer();
        if (!p) {
            Bummer();
        } else {
            NetworkReceivePacket(p);
            OSCAcceptPacket(p);
        }
    }
    while (TimeLeftBeforeWeHaveDoSomething()) {
        if (!OSCBeProductiveWhileWaiting()) break;
    }
}

*/

