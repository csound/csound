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
  OSC-callbacklist.c
  Linked lists of methods

  Matt Wright, 11/20/97

  Allocator is a simple linked list of free nodes.

*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include "OSC-dispatch.h"
#include "OSC-callbacklist.h"

static callbackList allNodes;
static callbackList freeNodes;

/* Call this before you call anything else */
OSCBoolean InitCallbackListNodes(int numNodes, void *(*InitTimeMalloc)(int numBytes)) {
    int i;

    allNodes = (*InitTimeMalloc)(numNodes * sizeof(*allNodes));
    if (allNodes == 0) return FALSE;

    /* Initialize list of freeNodes */
    freeNodes = &(allNodes[0]);
    for (i = 0; i < numNodes-1; ++i) {
        allNodes[i].next = &(allNodes[i+1]);
    }
    allNodes[numNodes-1].next = 0;
    return TRUE;
}

callbackList AllocCallbackListNode(methodCallback callback, void *context,
                                   struct callbackListNode *next) {
    callbackList result;
    if (freeNodes == 0) {
        /* OSCProblem("Out of memory for callback lists!"); */
        return 0;
    }

    result = freeNodes;
    freeNodes = freeNodes->next;

    result->callback = callback;
    result->context = context;
    result->next = next;
#ifdef DEBUG_CBL
    printf("AllocCallbackListNode: returning %p (cb %p, context %p, next %p)\n",
           result, result->callback, result->context, result->next);
#endif
    return result;
}

void FreeCallbackListNode(callbackList cb) {
#ifdef DEBUG_CBL
    printf("FreeCallbackListNode(%p)\n", cb);
#endif
    cb->next = freeNodes;
    freeNodes = cb;
}
