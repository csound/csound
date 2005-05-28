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

/* OSC-priority-queue.h
   Interface to priority queue used by OSC time tag scheduler

   Matt Wright, 3/13/98

*/

/* include OSC-timetag.h before this file. */

/* This queue manages pointers to data objects.  It doesn't care what's in the
   objects except that the first element has to be an OSCTimeTag.  So whatever
   data you want to store, cast your pointer to it to a pointer to this type. */

typedef struct {
    OSCTimeTag timetag;
    /* There will be other stuff... */
} *OSCSchedulableObject;

typedef struct OSCQueueStruct *OSCQueue;

/* Make a new queue, or return 0 for failure. */
OSCQueue OSCNewQueue(int maxItems, void *(*InitTimeMalloc)(int numBytes));

/* Put something into the queue.  Return FALSE if quque is full. */
OSCBoolean OSCQueueInsert(OSCQueue q, OSCSchedulableObject o);

/* What's the time tag of the earliest item in the queue?
   Return OSCTT_BiggestPossibleTimeTag() if queue is empty. */
OSCTimeTag OSCQueueEarliestTimeTag(OSCQueue q);

/* Remove the item from the front of the queue.  Fatal error
   if the queue is empty.  */
OSCSchedulableObject OSCQueueRemoveEarliest(OSCQueue q);

/* Interface for examining items currently stored on the queue:

   - To start, call OSCQueueScanStart().

   - Then each subsequent call to OSCQueueScanNext() returns a pointer to an
     OSCSchedulableObject that is stored on the queue, until
     OSCQueueScanNext() returns 0 to indicate that all objects on the queue
     have been scanned.

   The objects returned by OSCQueueScanNext() come in chronological order (or
   approximately chronological order, depending on the underlying queue data
   structure).

   If you call OSCQueueRemoveCurrentScanItem(), the object most recently
   returned by OSCQueueScanNext() will be removed from the queue.

   If there are any insertions or deletions to the queue, the sequence of
   scanned objects must still include every object in the queue.  This may
   cause a particular object to be returned more than once by
   OSCQueueScanNext().
*/

void OSCQueueScanStart(OSCQueue q);
OSCSchedulableObject OSCQueueScanNext(OSCQueue q);
void OSCQueueRemoveCurrentScanItem(OSCQueue q);
