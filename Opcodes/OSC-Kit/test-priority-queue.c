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

/* test-priority-queue.c
   Test shell for priority-queue implementations.

   Matt Wright, 3/16/98
*/

#include <stdio.h>
#include <stdlib.h>
#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-priority-queue.h"

typedef struct {
    OSCTimeTag timetag;
    char *data;
} myObj;

#define NUM_OBJS 100
myObj objects[NUM_OBJS];

void *Allocator(int numbytes) {
    return malloc(numbytes);
}


void ScanTest(OSCQueue q) {
    myObj *item;

    printf("Start scan.\n");
    OSCQueueScanStart(q);

    while(1) {
	item = (myObj *) OSCQueueScanNext(q);
	if (item == 0) {
	    printf("End of scan.\n");
	    break;
	}
	printf("  Next scan object:  time tag %llu, data %s\n",
	       item->timetag, item->data);
    }

    printf("Queue should be unchanged after scan:   ");
    OSCQueuePrint(q);

    printf("Another scan, this time removing data that begins with 's'.\n");
    OSCQueueScanStart(q);

    while(1) {
        item = (myObj *) OSCQueueScanNext(q);
        if (item == 0) {
            printf("End of scan.\n");
            break;
	}
	if (item->data[0] == 's') {
	    printf("  Scan object data begins with 's':  time tag %llu, data %s\n",
               item->timetag, item->data);
	    printf("How the queue looks before removal:   ");
	    OSCQueuePrint(q);
	    OSCQueueRemoveCurrentScanItem(q);
	    printf("How the queue looks after removal:   ");
	    OSCQueuePrint(q);
	}
    }
}


void StressTest(OSCQueue q) {
    int i,j;
    myObj *item;

    printf("\n\nStress Test...\n\n");

    for (i = 1; i < NUM_OBJS; ++i) {
	OSCTT_SetFromInt(&objects[i].timetag, rand());
	objects[i].data = "stress test";

	if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[i])) == FALSE) {
	    printf("OSCQueueInsert() returned FALSE!\n");
	    return;
	}

	if (i % 13 == 0) {
	    item = (myObj *) OSCQueueRemoveEarliest(q);
	    printf("First earliest: %llu\n", item->timetag);
	    item = (myObj *) OSCQueueRemoveEarliest(q);
	    printf("Second earliest: %llu\n", item->timetag);
	    item = (myObj *) OSCQueueRemoveEarliest(q);
	    printf("Third earliest: %llu\n\n", item->timetag);
	}
    }

    while (OSCTT_Compare(OSCQueueEarliestTimeTag(q), OSCTT_BiggestPossibleTimeTag())) {
	item = (myObj *) OSCQueueRemoveEarliest(q);
	printf("next from queue: %llu\n", item->timetag);
    }
}
	

int main (void) {
    OSCQueue q;
    myObj *item;

    q = OSCNewQueue(100, Allocator);
    if (q == 0) {
	printf("OSCNewQueue() returned 0!\n");
	return;
    }

    printf("Made an empty queue:  ");
    OSCQueuePrint(q);

    printf("Inserting three objects.\n");
    OSCTT_SetFromInt(&objects[0].timetag, 5);
    objects[0].data = "five";
    OSCTT_SetFromInt(&objects[1].timetag, 2);
    objects[1].data = "two";
    OSCTT_SetFromInt(&objects[2].timetag, 7);
    objects[2].data = "seven";

    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[0])) == FALSE) {
	printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }

    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[1])) == FALSE) {
	printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }

    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[2])) == FALSE) {
	printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }

    printf("Queue with three objects:   ");
    OSCQueuePrint(q);

    printf("Earliest time tag is %llu.\n", OSCQueueEarliestTimeTag(q));

    printf("Remove front item:\n");
    
    item = (myObj *) OSCQueueRemoveEarliest(q);
    printf("Time tag %llu, data %s\n", item->timetag, item->data);

    printf("Queue with two objects:   ");
    OSCQueuePrint(q);

    printf("Inserting three more objects.\n");
    OSCTT_SetFromInt(&objects[3].timetag, 11);
    objects[3].data = "eleven";
    OSCTT_SetFromInt(&objects[4].timetag, 6);
    objects[4].data = "six";
    OSCTT_SetFromInt(&objects[5].timetag, 3);
    objects[5].data = "three";
    
    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[3])) == FALSE) {
        printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }

    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[4])) == FALSE) {
        printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }

    if (OSCQueueInsert(q, (OSCSchedulableObject) &(objects[5])) == FALSE) {
        printf("OSCQueueInsert() returned FALSE!\n");
        return;
    }


    printf("Queue with five objects:   ");
    OSCQueuePrint(q);


    ScanTest(q);
    StressTest(q);

    printf("Done!\n");

    return 0;
}
