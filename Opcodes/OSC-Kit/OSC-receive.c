/* Copyright (c) 1998.  The Regents of the University of California (Regents).
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
  OSC-receive.c
  Matt Wright, 3/13/98, 6/3/98

  Adapted from OSC-addressability.c (and seriously cleaned up!)

*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include "NetworkReturnAddress.h"
#include "OSC-receive.h"
#include "OSC-priority-queue.h"
#include "OSC-string-help.h"
#include "OSC-drop.h"
#include "OSC-dispatch.h"

#if defined(DEBUG_INTERNAL) || defined(DEBUG) || defined(DEBUG_PACKET_MEM) || defined(DEBUG_QD_MEM) || defined(DEBUG_8BYTE_ALIGN) || defined(SUSPECT_QD_PROB)
#include <stdio.h>
#endif



struct {
    OSCQueue TheQueue;		/* The Priority Queue */
    OSCTimeTag lastTimeTag;	/* Best approximation to current time */
    OSCBoolean timePassed;		/* TRUE if OSCInvokeMessagesThatAreReady() has been
				   called since the last time OSCBeProductiveWhileWaiting() was. */
    int recvBufSize;		/* Size of all receive buffers */
    void *(*InitTimeMalloc)(int numBytes);
    void *(*RealTimeMemoryAllocator)(int numBytes);
} globals;


/* Data structures */

struct OSCPacketBuffer_struct {
    char *buf;			/* Contents of network packet go here */
    int n;			/* Overall size of packet */
    int refcount;		/* # queued things using memory from this buffer */
    struct OSCPacketBuffer_struct *nextFree;	/* For linked list of free packets */

    OSCBoolean returnAddrOK;       /* Because returnAddr points to memory we need to
				   store future return addresses, we set this
				   field to FALSE in situations where a packet
				   buffer "has no return address" instead of
				   setting returnAddr to 0 */

    void *returnAddr;	/* Addr of client this packet is from */
	/* This was of type NetworkReturnAddressPtr, but the constness
           was making it impossible for me to initialize it.  There's
	   probably a better way that I don't understand. */

};

/* These are the data objects that are inserted and removed from the
   scheduler.  The idea is that we can insert a single message or
   an entire bundle on the scheduler, and we can leave it in various
   states of being parsed and pattern matched.  */

#define NOT_DISPATCHED_YET ((callbackList) -1)

typedef struct queuedDataStruct {
    OSCTimeTag timetag;	  /* When this bundle or message is supposed to happen */
    OSCPacketBuffer myPacket;   /* Ptr. to buffer this is contained in */

    enum {MESSAGE, BUNDLE} type;

    union {
	struct {
	    char *bytes;
	    int length;
	} bundle;

	struct {
	    char *messageName;			/* Ptr. into receive buffer */
	    int length;				/* Includes name and arugments */
	    void *args;				/* 0 if not yet parsed */
	    int argLength;
	    callbackList callbacks;		/* May be NOT_DISPATCHED_YET */
	} message;
    } data;

    struct queuedDataStruct *nextFree;	    /* For linked list of free structures */
} queuedData;



/* Static procedure declatations */
static OSCBoolean InitPackets(int receiveBufferSize, int clientAddrSize, int numReceiveBuffers);
static OSCBoolean InitQueuedData(int numQueuedObjects);
static queuedData *AllocQD(void);
static void FreeQD(queuedData *qd);
static void CallWholeCallbackList(callbackList l, int argLength, void *args, OSCTimeTag when, NetworkReturnAddressPtr returnAddr);
static void InsertBundleOrMessage(char *buf, int n, OSCPacketBuffer packet, OSCTimeTag enclosingTimeTag);
static void ParseBundle(queuedData *qd);
static OSCBoolean ParseMessage(queuedData *qd);
/* static void CheckPacketRefcount(OSCPacketBuffer packet); */
static void PacketAddRef(OSCPacketBuffer packet);
static void PacketRemoveRef(OSCPacketBuffer packet);


/**************************************************
   Initialization and memory pre-allocation
 **************************************************/


OSCBoolean OSCInitReceive(struct OSCReceiveMemoryTuner *t) {
    globals.recvBufSize = t->receiveBufferSize;
    globals.InitTimeMalloc = t->InitTimeMemoryAllocator;
    globals.RealTimeMemoryAllocator = t->RealTimeMemoryAllocator;

    globals.TheQueue = OSCNewQueue(t->numQueuedObjects, t->InitTimeMemoryAllocator);
    if (globals.TheQueue == 0) return FALSE;

    globals.lastTimeTag = OSCTT_Immediately();
    globals.timePassed = TRUE;

    if (InitPackets(t->receiveBufferSize, SizeOfNetworkReturnAddress(),
		    t->numReceiveBuffers) == FALSE) return FALSE;
    if (InitQueuedData(t->numQueuedObjects) == FALSE) return FALSE;
    if (InitCallbackListNodes(t->numCallbackListNodes, t->InitTimeMemoryAllocator)
	 == FALSE) return FALSE;
    
    return TRUE;
}


/**************************************************
   Managing packet data structures
 **************************************************/

static struct OSCPacketBuffer_struct *freePackets;

#ifdef DEBUG_PACKET_MEM
static void PrintPacketFreeList(void) {
    struct OSCPacketBuffer_struct *p;
    printf("- freePackets:");
    if (freePackets == 0) {
	printf(" [none]");
    }
    for (p = freePackets; p != 0; p = p->nextFree) {
	printf(" %p", p);
    }
    printf("\n");
}
#endif

#define MIN_REASONABLE_RCV_BUFSIZE 128

static OSCBoolean InitPackets(int receiveBufferSize, int clientAddrSize, int numReceiveBuffers) {
    int i;
    struct OSCPacketBuffer_struct *allPackets;

    if (receiveBufferSize < MIN_REASONABLE_RCV_BUFSIZE) {
	fatal_error("OSCInitReceive: receiveBufferSize of %d is unreasonably small.",
		    receiveBufferSize);
    }

    allPackets = (*(globals.InitTimeMalloc))(numReceiveBuffers * sizeof(*allPackets));
    if (allPackets == 0) return FALSE;

    for (i = 0; i < numReceiveBuffers; ++i) {
	allPackets[i].returnAddr = (*(globals.InitTimeMalloc))(clientAddrSize);
	if (allPackets[i].returnAddr == 0) return FALSE;

	allPackets[i].buf = (*(globals.InitTimeMalloc))(receiveBufferSize);
	if (allPackets[i].buf == 0) return FALSE;

	allPackets[i].nextFree = &(allPackets[i+1]);
    }
    allPackets[numReceiveBuffers-1].nextFree = ((struct OSCPacketBuffer_struct *) 0);
    freePackets = allPackets;

    return TRUE;
}

char *OSCPacketBufferGetBuffer(OSCPacketBuffer p) {
    return p->buf;
}

int *OSCPacketBufferGetSize(OSCPacketBuffer p) {
    return &(p->n);
}

int OSCGetReceiveBufferSize(void) {
    return globals.recvBufSize;
}

NetworkReturnAddressPtr OSCPacketBufferGetClientAddr(OSCPacketBuffer p) {
    return p->returnAddr;
}

#ifdef DEBUG
void PrintPacket(OSCPacketBuffer p) {
    printf("Packet %p.  buf %p, n %d, refcount %d, nextFree %p\n",
	   p, p->buf, p->n, p->refcount, p->nextFree);
}
#endif

    

OSCPacketBuffer OSCAllocPacketBuffer(void) {
    OSCPacketBuffer result;
    if (freePackets == 0) {
	/* Could try to call the real-time memory allocator here */
	OSCWarning("OSCAllocPacketBuffer: no free packets!");
	return 0;
    }

    result = freePackets;
    freePackets = result->nextFree;
    result->refcount = 0;

#ifdef DEBUG_PACKET_MEM
    printf("OSCAllocPacketBuffer: allocating %p ", result);
    PrintPacketFreeList();
#endif

    return result;
}

void OSCFreePacket(OSCPacketBuffer p) {
#ifdef PARANOID
    if (p->refcount != 0) {
	OSCWarning("OSCFreePacket: %p's refcount is %d!\n", p, p->refcount);
    }
#endif

    p->nextFree = freePackets;
    freePackets = p;

#ifdef DEBUG_PACKET_MEM
    printf("OSCFreePacket: freed %p ", p);
    PrintPacketFreeList();
#endif

}


/**************************************************
   Dealing with OpenSoundControl packets and
   making the messages take effect.
 **************************************************/

static queuedData *freeQDList;

#ifdef DEBUG_QD_MEM
static void PrintQDFreeList(void) {
    static queuedData *p;
    printf("- freeQDList:");
    if (freeQDList == 0) {
        printf(" [none]");
    }
    for (p = freeQDList;  p != 0; p = p->nextFree) {
        printf(" %p", p);
    }
    printf("\n");
}
#endif

static OSCBoolean InitQueuedData(int numQueuedObjects) {
    int i;
    queuedData *allQD;

    allQD = (*(globals.InitTimeMalloc))(numQueuedObjects * (sizeof(*allQD)));
    if (allQD == 0) return FALSE;

    for (i = 0; i < numQueuedObjects; ++i) {
	allQD[i].nextFree = &(allQD[i+1]);
    }
    allQD[numQueuedObjects-1].nextFree = 0;
    freeQDList = &(allQD[0]);

    return TRUE;
}

static queuedData *AllocQD(void) {
    queuedData *result;

    if (freeQDList == 0) {
	/* Could try to call realtime malloc() */
	OSCWarning("AllocQD: no QD objects free now; returning 0.");
	return 0;
    }

    result = freeQDList;
    freeQDList = freeQDList->nextFree;
    return result;
}
    
static void FreeQD(queuedData *qd) {
    qd->nextFree = freeQDList;
    freeQDList = qd;
}


void OSCAcceptPacket(OSCPacketBuffer packet) {
    if ((packet->n % 4) != 0) {
	OSCProblem("OSC packet size (%d bytes) not a multiple of 4.", packet->n);
	DropPacket(packet);
	return;
    }

#ifdef DEBUG
    printf("OSCAcceptPacket(OSCPacketBuffer %p, buf %p, size %d)\n", 
	   packet, packet->buf, packet->n);
#endif

    /* If the packet came from the user, it's return address is OK. */
    packet->returnAddrOK = TRUE;

    InsertBundleOrMessage(packet->buf, packet->n, packet, OSCTT_Immediately());

#ifdef PARANOID
    if (packet->refcount == 0) {
	if (freePackets != packet) {
	    fatal_error("OSCAcceptPacket: packet refcount 0, but it's not the head of the free list!");
	}
    }
#endif

    OSCInvokeAllMessagesThatAreReady(globals.lastTimeTag);
}

OSCBoolean OSCBeProductiveWhileWaiting(void) {
    /* Here's where we could be clever if an allocation fails. 
       (I.e., if we're out of QD objects, we should avoid
       parsing bundles.) The code isn't that smart yet. */

    queuedData *qd;

    if (globals.timePassed) {
	OSCQueueScanStart(globals.TheQueue);
    }

    while (1) {
	qd = (queuedData *) OSCQueueScanNext(globals.TheQueue);
	if (qd == 0) return FALSE;

	if (qd->type == BUNDLE) {
            ParseBundle(qd);
	    OSCQueueRemoveCurrentScanItem(globals.TheQueue);
	    return TRUE;
	} else {
	    if (qd->data.message.callbacks == NOT_DISPATCHED_YET) {
                if (ParseMessage(qd) == FALSE) {
                    /* Problem with this message - flush it. */
		    DropMessage(qd->data.message.messageName,
				qd->data.message.length,
				qd->myPacket);
		    OSCQueueRemoveCurrentScanItem(globals.TheQueue);
		    PacketRemoveRef(qd->myPacket);
                    FreeQD(qd);
		}
		return TRUE;
	    }
	    /* The item we found was an already-dispatched message,
	       so continue the while loop. */
	}
    }
}
    
OSCBoolean OSCInvokeMessagesThatAreReady(OSCTimeTag now) {
    queuedData *x;
    OSCTimeTag thisTimeTag;

    globals.lastTimeTag = now;
    globals.timePassed = TRUE;

    thisTimeTag = OSCQueueEarliestTimeTag(globals.TheQueue);

    if (OSCTT_Compare(thisTimeTag, now) > 0) {
	/* No messages ready yet. */
	return FALSE;
    }

#ifdef DEBUG
    printf("OSCInvokeMessagesThatAreReady(%llx) - yes, some are ready; earliest %llx\n", now, thisTimeTag);
#endif

    while (OSCTT_Compare(thisTimeTag, OSCQueueEarliestTimeTag(globals.TheQueue)) == 0) {
	x = (queuedData *) OSCQueueRemoveEarliest(globals.TheQueue);

#ifdef DEBUG
	printf("...Just removed earliest entry from queue: %p, TT %llx, %s\n",
	       x, x->timetag, x->type == MESSAGE ? "message" : "bundle");
	if (x->type == MESSAGE) {
	    printf("...message %s, len %d, args %p, arglen %d, callbacks %p\n",
		   x->data.message.messageName, x->data.message.length, x->data.message.args,
		   x->data.message.argLength, x->data.message.callbacks);
	} else {
	    if (x->data.bundle.length == 0) {
		printf("...bundle is empty.\n");
	    } else {
		printf("...bundle len %d, first count %d, first msg %s\n",
		       x->data.bundle.length, *((int *) x->data.bundle.bytes), x->data.bundle.bytes+4);
	    }
	}
	PrintPacket(x->myPacket);
#endif

	if (x->type == BUNDLE) {
	    ParseBundle(x);
	} else {
	    if (x->data.message.callbacks == NOT_DISPATCHED_YET) {
		if (ParseMessage(x) == FALSE) {
		    /* Problem with this message - flush it. */
		    PacketRemoveRef(x->myPacket);
		    FreeQD(x);
		    continue;
		}
	    }

	    CallWholeCallbackList(x->data.message.callbacks,
				  x->data.message.argLength,
				  x->data.message.args, 
				  thisTimeTag,
				  x->myPacket->returnAddrOK ? x->myPacket->returnAddr : 0);

	    PacketRemoveRef(x->myPacket);
	    FreeQD(x);
	}
    }


#ifdef PARANOID
    if (OSCTT_Compare(thisTimeTag, OSCQueueEarliestTimeTag(globals.TheQueue)) > 0) {
	fatal_error("OSCInvokeMessagesThatAreReady: corrupt queue!\n"
		    "  just did %llx; earliest in queue is now %llx",
		    thisTimeTag, OSCQueueEarliestTimeTag(globals.TheQueue));
    }
#endif

    return OSCTT_Compare(OSCQueueEarliestTimeTag(globals.TheQueue), now) <= 0;
}

void OSCInvokeAllMessagesThatAreReady(OSCTimeTag now) {
    while (OSCInvokeMessagesThatAreReady(now)) {
	/* Do nothing */
    }
}

static void CallWholeCallbackList(callbackList l, int argLength, void *args, OSCTimeTag when,
				  NetworkReturnAddressPtr returnAddr) {
    /* In a multithreaded application, this might run in a different thread
       than the thread that deals with the priority queue. */

    callbackList next;

    while (l != 0) {
	(*(l->callback))(l->context, argLength, args, when, returnAddr);
	next = l->next;
	FreeCallbackListNode(l);
	l = next;
    }
}

static void InsertBundleOrMessage(char *buf, int n, OSCPacketBuffer packet, OSCTimeTag enclosingTimeTag) {
    OSCBoolean IsBundle;
    queuedData *qd;

    /* We add the reference first thing so in case any of the upcoming
       potential failure situations come we can call PacketRemoveRef, thereby
       freeing the packet if necessary. */
    PacketAddRef(packet);

#ifdef PARANOID
    if ((n % 4) != 0) {
	OSCProblem("OSC message or bundle size (%d bytes) not a multiple of 4.", n);
	DropMessage(buf, n, packet)
	PacketRemoveRef(packet);
	return;
    }
#endif

    if ((n >= 8) && (strncmp(buf, "#bundle", 8) == 0)) {
	IsBundle = TRUE;

	if (n < 16) {
	    OSCProblem("Bundle message too small (%d bytes) for time tag.", n);
	    DropBundle(buf, n, packet);
	    PacketRemoveRef(packet);
	    return;
	}
    } else {
	IsBundle = FALSE;
    }


    qd = AllocQD();

    if (qd == 0) {
	OSCProblem("Not enough memory for queued data!");
	DropBundle(buf, n, packet);
	PacketRemoveRef(packet);
	return;
    }

    qd->myPacket = packet;
    qd->type = IsBundle ? BUNDLE : MESSAGE;

    if (IsBundle) {
	/* Be careful of 8-byte alignment when copying the time tag.  Here's a good
	   way to get a bus error when buf happens not to be 8-byte aligned:
	   qd->timetag = *((OSCTimeTag *)(buf+8));
	*/
	memcpy(&(qd->timetag), buf+8, sizeof(OSCTimeTag));

	if (OSCTT_Compare(qd->timetag, enclosingTimeTag) < 0) {
	    OSCProblem("Time tag of sub-bundle is before time tag of enclosing bundle.");
	    DropBundle(buf, n, packet);
	    PacketRemoveRef(packet);
	    FreeQD(qd);
	    return;
	}
	qd->data.bundle.bytes = buf + 16;
	qd->data.bundle.length = n - 16;
    } else {
        qd->timetag = enclosingTimeTag;
	qd->data.message.messageName = buf;
	qd->data.message.length = n;
	qd->data.message.callbacks = NOT_DISPATCHED_YET;
    }

    OSCQueueInsert(globals.TheQueue, (OSCSchedulableObject) qd);
}


static void ParseBundle(queuedData *qd) {
    /* A queued bundle has been removed from the scheduler queue, and now it's
       time to parse all the stuff inside it and schedule the enclosed
       messages and bundles.  Once all the contents of the bundle have been
       parsed and scheduled, we trash the bundle, decrementing the packet
       count and freeing the QD. */

    int size;
    int i = 0;

    if (qd->type != BUNDLE) {
        fatal_error("This can't happen: bundle isn't a bundle!");
    }

    while (i < qd->data.bundle.length) {
	size = *((int *) (qd->data.bundle.bytes + i));
	if ((size % 4) != 0) {
	    OSCProblem("Bad size count %d in bundle (not a multiple of 4).", size);
	    DropBundle(qd->data.bundle.bytes, qd->data.bundle.length, qd->myPacket);
	    goto bag;
	}
	if ((size + i + 4) > qd->data.bundle.length) {
	    OSCProblem("Bad size count %d in bundle (only %d bytes left in entire bundle).",
		     size, qd->data.bundle.length-i-4);
	    DropBundle(qd->data.bundle.bytes, qd->data.bundle.length, qd->myPacket);
	    goto bag;	
	}
	
	/* Recursively handle element of bundle */
	InsertBundleOrMessage(qd->data.bundle.bytes+i+4, size, qd->myPacket, qd->timetag);
	i += 4 + size;
    }

    if (i != qd->data.bundle.length) {
	fatal_error("This can't happen: internal logic error parsing bundle");
    }

bag:
    /* If we got here successfully, we've added to the packet's reference count for
       each message or subbundle by calling InsertBundleOrMessage(), so we remove the one
       reference for bundle that we just parsed.  If we got here by "goto bag", there's a
       problem with the bundle so we also want to lose the reference count. */

    PacketRemoveRef(qd->myPacket);
    FreeQD(qd);
}


static OSCBoolean ParseMessage(queuedData *qd) {
    /* Fill in all the information we'll need to execute the message as
       quickly as possible when the time comes.  This means figuring out where
       the address ends and the arguments begin, and also pattern matching the
       address to find the callbacks associated with it.

       The message may be something we have to invoke now, or it may be some
       message scheduled for the future that's just waiting on the queue; this
       procedure doesn't care.  */


    char *args; 	/* char * so we can do pointer subtraction */
    int messageLen;
    char *DAAS_errormsg;


    if (qd->type != MESSAGE) {
        fatal_error("This can't happen: message isn't a message!");
    }

    args = OSCDataAfterAlignedString(qd->data.message.messageName, 
				     qd->data.message.messageName+qd->data.message.length,
				     &DAAS_errormsg);

    if (args == 0) {
	OSCProblem("Bad message name string: %s\n", DAAS_errormsg);
	DropMessage(qd->data.message.messageName, qd->data.message.length, qd->myPacket);
	return FALSE;
    }

    qd->data.message.args = args;
    messageLen = args - qd->data.message.messageName;
    qd->data.message.argLength = qd->data.message.length - messageLen;

    qd->data.message.callbacks = OSCDispatchMessage(qd->data.message.messageName);

    if (qd->data.message.callbacks == 0) {
	OSCWarning("Message pattern \"%s\" did not correspond to any address in the synth.",
		   qd->data.message.messageName);
	return FALSE;
    }

    return TRUE;
}

static void PacketAddRef(OSCPacketBuffer packet) {
    ++(packet->refcount);
}

static void PacketRemoveRef(OSCPacketBuffer packet) {
    --(packet->refcount);
    if (packet->refcount == 0) {
        OSCFreePacket(packet);
    }
}



/**************************************************
 Implementation of procedures declared in 
 OSC-internal-messages.h
 **************************************************/

#include "OSC-internal-messages.h"

OSCBoolean OSCSendInternalMessage(char *address, int arglen, void *args) {
    return OSCSendInternalMessageWithRSVP(address, arglen, args, 0);
}

OSCBoolean OSCSendInternalMessageWithRSVP(char *address, int arglen, void *args,
				       NetworkReturnAddressPtr returnAddr) {
    callbackList l = OSCDispatchMessage(address);

    if (l == 0) return FALSE;

    CallWholeCallbackList(l, arglen, args, OSCTT_Immediately(), returnAddr);
    return TRUE;
}



OSCBoolean OSCScheduleInternalMessages(OSCTimeTag when, int numMessages, 
				    char **addresses, int *arglens, void **args) {
    int i, bufSizeNeeded, paddedStrLen;
    OSCPacketBuffer p;
    queuedData *qd, *scan;
    char *bufPtr;
    char *oldBufPtr;
    

    /* Figure out how big of a buffer we'll need to hold this huge bundle.
       We don't store the "#bundle" string or the time tag, just the 4-byte
       size counts, the addresses, possible extra null padding for the
       addresses, and the arguments. */

    bufSizeNeeded = 0;
    for (i = 0; i < numMessages; ++i) {
	bufSizeNeeded += 4 + OSCPaddedStrlen(addresses[i]) + arglens[i];
    }

    if (bufSizeNeeded > OSCGetReceiveBufferSize()) {
	return FALSE;
    }
	

    /* Now try to allocate the data objects to hold these messages */
    qd = AllocQD();
    if (qd == 0) return FALSE;

    p = OSCAllocPacketBuffer();
    if (p == 0) {
	FreeQD(qd);
	return FALSE;
    }

    /* Now fill in the buffer with a fake #bundle message.   This is almost like
       putting a real #bundle message in the buffer and then calling OSCAcceptPacket,
       except that we save a little time and memory by not writing "#bundle" or the time tag,
       and by pre-parsing the messages a little.   Thus, this code duplicates a lot
       of what's in InsertBundleOrMessage() */

    bufPtr = p->buf;

    for (i = 0; i < numMessages; ++i) {
	/* First the size count of this bundle element */
	*((int4 *) bufPtr) = OSCPaddedStrlen(addresses[i]) + arglens[i];
	bufPtr += sizeof(int4);

	/* Then the address */
	bufPtr = OSCPaddedStrcpy(bufPtr, addresses[i]);

	/* Then the arguments */
	memcpy(bufPtr, args[i], arglens[i]);
	bufPtr += arglens[i];
    }

#ifdef PARANOID
    if (bufPtr != p->buf+bufSizeNeeded) {
	fatal_error("OSCScheduleInternalMessages: internal error");
    }
#endif

    /* Fill in the rest of the packet fields */
    p->n = bufSizeNeeded;
    p->returnAddrOK = FALSE;
    PacketAddRef(p);

    /* Now fill in the queuedData object */
    qd->timetag = when;
    qd->myPacket = p;
    qd->type = BUNDLE;
    qd->data.bundle.length = bufSizeNeeded;
    qd->data.bundle.bytes = p->buf;

    /* Now we can put it into the scheduling queue. */
    OSCQueueInsert(globals.TheQueue, (OSCSchedulableObject) qd);

    return TRUE;
}
