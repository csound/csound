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
  OSC-address-space.c
  Matt Wright, 3/16/98
*/

#include "OSC-common.h"
#include "OSC-timetag.h"
#include "OSC-address-space.h"
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#define MAX_ALIASES_PER_CONTAINER 3
#define MAX_CHILDREN_PER_CONTAINER 20
#define MAX_METHODS_PER_CONTAINER 30
#define BASE_NUM_TO_REALLOCATE 10


struct OSCContainerStruct {
    struct OSCContainerStruct *parent;
    int numChildren;
    Name childrenNames[MAX_CHILDREN_PER_CONTAINER];
    struct OSCContainerStruct *children[MAX_CHILDREN_PER_CONTAINER];
    int numMethods;
    Name methodNames[MAX_METHODS_PER_CONTAINER];
    OSCMethod methods[MAX_METHODS_PER_CONTAINER];
    struct OSCContainerQueryResponseInfoStruct QueryResponseInfo;
    struct OSCContainerStruct *next;
};

struct OSCMethodStruct {
    methodCallback callback;
    void *context;
    struct OSCMethodQueryResponseInfoStruct QueryResponseInfo;
    struct OSCMethodStruct *next;
};

/* Globals */
static OSCBoolean Initialized = FALSE;
static OSCcontainer OSCTopLevelContainer;
static OSCcontainer freeContainers;   /* Linked list via next field. */
static OSCMethod freeMethods;         /* Linked list via next field. */
static void *(*RealTimeMemoryAllocator)(int numBytes);


/* Note:  The free list of containers should actually be a "free forest", so 
   that all the subcontainers recursively under a freed container are 
   automatically freed.

	FREE: just stick the freed subtree on the front of the list.

	ALLOC: Take all the children of the first container on the list and
             insert them in the free list, then return that first container.

*/



/************************ Initialization and Memory  ************************/

static void MakeFreeContainersList(int n) {
    int i;

    for (i = 0; i+1 < n; ++i) {
        freeContainers[i].next = &(freeContainers[i+1]);
    }
    freeContainers[n-1].next = 0;
}

static void MakeFreeMethodsList(int n) {
    int i;

    for (i = 0; i+1 < n; ++i) {
        freeMethods[i].next = &(freeMethods[i+1]);
    }
    freeMethods[n-1].next = 0;
}

OSCcontainer OSCInitAddressSpace(struct OSCAddressSpaceMemoryTuner *t) {
    int bytesNeeded, i;

    if (Initialized)
	fatal_error("OSCInitAddressSpace: already initialized!");
    Initialized = TRUE;

    RealTimeMemoryAllocator = t->RealTimeMemoryAllocator;

    bytesNeeded = (1 + t->initNumContainers) * sizeof(*freeContainers);
    freeContainers = (OSCcontainer) (*(t->InitTimeMemoryAllocator))(bytesNeeded);
    if (freeContainers == 0) {
	fatal_error("OSCInitAddressSpace: couldn't allocate %d bytes for %d containers",
		    bytesNeeded, t->initNumContainers);
    }

    OSCTopLevelContainer = &freeContainers[t->initNumContainers];
    MakeFreeContainersList(t->initNumContainers);

    bytesNeeded = t->initNumMethods * sizeof(*freeMethods);
    freeMethods = (OSCMethod) (*(t->InitTimeMemoryAllocator))(bytesNeeded);
    if (freeMethods == 0) {
        fatal_error("OSCInitAddressSpace: couldn't allocate %d bytes for %d methods",
		    bytesNeeded, t->initNumMethods);
    }
    MakeFreeMethodsList(t->initNumMethods);

    /* Initialize the top-level container */
    OSCTopLevelContainer->parent = 0;
    OSCTopLevelContainer->numChildren = 0;
    OSCTopLevelContainer->numMethods = 0;
    OSCTopLevelContainer->QueryResponseInfo.comment = "OSC top-level container";
    OSCTopLevelContainer->next = 0;

    return OSCTopLevelContainer;
}


/* Container and method memory management: linked lists of free objects */

static OSCcontainer AllocContainer(void) {
    static int numExtraAllocs = 0;

    OSCcontainer result;
    if (freeContainers != 0) {
	result = freeContainers;
	freeContainers = freeContainers->next;
	return result;
    }

    OSCWarning("Out of memory for containers; trying to allocate more in real time");
    {
	int num = BASE_NUM_TO_REALLOCATE * ++numExtraAllocs;
	freeContainers = (*RealTimeMemoryAllocator)(num * sizeof(*freeContainers));
	if (freeContainers == 0) {
	    OSCWarning("Real-time allocation failed");
	    return 0;
	}
	MakeFreeContainersList(num);
	return AllocContainer();
    }
}

static void FreeContainer(OSCcontainer c) {
    c->next = freeContainers;
    freeContainers = c;
}

static OSCMethod AllocMethod(void) {
    static int numExtraAllocs = 0;
    OSCMethod result;

    if (freeMethods != 0) {
        result = freeMethods;
	freeMethods = freeMethods->next;
	return result;
    }

    OSCWarning("Out of memory for methods; trying to allocate more in real time");
    {
        int num = BASE_NUM_TO_REALLOCATE * ++numExtraAllocs;
        freeMethods = (*RealTimeMemoryAllocator)(num * sizeof(*freeMethods));
        if (freeMethods == 0) {
            OSCWarning("Real-time allocation failed");
            return 0;
	}
        MakeFreeMethodsList(num);
        return AllocMethod();
    }
}

static void FreeMethod(OSCMethod c) {
    c->next = freeMethods;
    freeMethods = c;
}


/**************************** Containers  ****************************/    

/* Managing the tree of containers and subcontainers, with aliases */

void AddSubContainer(OSCcontainer parent, OSCcontainer child, Name name) {
    if (parent->numChildren >= MAX_CHILDREN_PER_CONTAINER) {
	fatal_error("AddSubContainer: exceeded MAX_CHILDREN_PER_CONTAINER (%d)\n"
		    "Increase the value in OSC-address-space.c and recompile.",
		    MAX_CHILDREN_PER_CONTAINER);
    }

    parent->childrenNames[parent->numChildren] = name;
    parent->children[parent->numChildren] = child;
    ++(parent->numChildren);
}


OSCBoolean OSCAddContainerAlias(OSCcontainer container, Name otherName) {
    if (container->parent->numChildren >= MAX_CHILDREN_PER_CONTAINER) {
	return FALSE;
    }
    AddSubContainer(container->parent, container, otherName);
    return TRUE;
}

void RemoveSubContainer(OSCcontainer parent, OSCcontainer child) {
    int i, numRemoved;

    /* Remove every pointer to the container, even if it has multiple aliases */

    numRemoved = 0;
    for (i = 0; i < parent->numChildren; ++i) {
	if (parent->children[i] != child) {
	    parent->children[i-numRemoved] = parent->children[i];
	    parent->childrenNames[i-numRemoved] = parent->childrenNames[i];
	} else {
	    ++numRemoved;
	}
    }

    parent->numChildren -= numRemoved;

    if (numRemoved ==  0) {
	fatal_error("RemoveSubContainer: subcontainer not found!\n");
    }
}



   

OSCBoolean OSCRemoveContainerAlias(OSCcontainer container, Name otherName) {
    int i, j;
    OSCcontainer parent = container->parent;
    OSCBoolean found = FALSE;

    for (i = 0; i < parent->numChildren; ++i) {
        if (parent->childrenNames[i] == otherName) {
	    if (parent->children[i] != container) {
		fatal_error("OSCRemoveContainerAlias: %s is actually a sibling's name!",
			    otherName);
	    }
	    found = TRUE;
	    for (j = i+1; j < parent->numChildren; ++j) {
		parent->children[j-1] = parent->children[j];
		parent->childrenNames[j-1] = parent->childrenNames[j];
		--(parent->numChildren);
	    }
	}
    }
    if (!found) {
	fatal_error("OSCRemoveContainerAlias: %s not found!", otherName);
    }

    /* Now make sure the child still exists under another name */
    for (i = 0; i < parent->numChildren; ++i) {
	if (parent->children[i] == container) return TRUE;
    }

    OSCWarning("OSCRemoveContainerAlias: %s was the last name for that subcontainer");

    /* xxx should recursively free the container and its children... */
    return TRUE;
}
	    

OSCcontainer OSCNewContainer(Name name, OSCcontainer parent,
                             struct OSCContainerQueryResponseInfoStruct *QueryResponseInfo) {
    OSCcontainer me;

    me = AllocContainer();
    if (me == 0) return 0;

    if (strchr(name, '/') != NULL) {
	OSCProblem("Container name \"%s\" contains a slash --- not good.",
		   name);
	return 0;
    }
	
    me->parent = parent;
    AddSubContainer(me->parent, me, name);
    me->numChildren = 0;
    me->numMethods = 0;
    me->QueryResponseInfo = (*QueryResponseInfo);
    return me;
}


static const char *ContainerName(OSCcontainer c) {
    /* Return the first name associated with me in my parent's child list.
       (Assume all later ones are aliases.) */
    int i;

    for (i = 0; i < c->parent->numChildren; ++i) {
	if (c->parent->children[i] == c) {
	    return c->parent->childrenNames[i];
	}
    }
    fatal_error("ContainerName: Container %p isn't in its parent's child list.", c);
    return 0;
}


OSCBoolean OSCGetAddressString(char *target, int maxLength, OSCcontainer c) {
    int lenNeeded;

    if (maxLength <= 1) return FALSE;

    lenNeeded = gasHelp(target, maxLength-1, c) + 1; /* -1, +1 are for null char. */
    if (lenNeeded > maxLength) {
	OSCProblem("Address string too long (room for %d chars; need %d)",
		   maxLength, lenNeeded);
	target[0] = '\0';
	return FALSE;
    }
    return TRUE;
}

static int gasHelp(char *target, int maxLength, OSCcontainer c) {
    int sublength, length;
    const char *myName;

/*     printf("*** gasHelp %s %d %p %s\n", target, maxLength, c, c->name); */

    if (c == OSCTopLevelContainer) {
	target[0] = '/';
	target[1] = '\0';
	return 1;
    }

    myName = ContainerName(c);
    sublength = gasHelp(target, maxLength, c->parent);
    length = sublength + strlen(myName) + 1;  /* +1 is for trailing slash */
    if (length > maxLength) {
	return length;
    }
    
    strcpy(target+sublength, myName);
    target[length-1] = '/';
    target[length] = '\0';

    return length;
}


/**************************** Methods  ****************************/

#define LONG_ADDR_SIZE 1000 /* Just for error messages */

OSCMethod OSCNewMethod(Name name, OSCcontainer me, methodCallback callback, 
		       void *context, struct OSCMethodQueryResponseInfoStruct *QueryResponseInfo) {

    char addr[LONG_ADDR_SIZE];
    OSCMethod m;

#ifdef DEBUG
    printf("OSCNewMethod(name %s, container %p, callback %p, context %p)\n",
	   name, me, callback, context);
#endif

    if (strchr(name, '/') != NULL) {
	OSCProblem("Method name \"%s\" contains a slash --- not good.",
		   name);
	return 0;
    }


    if (me->numMethods >= MAX_METHODS_PER_CONTAINER) {
	addr[0] = '\0';
	OSCGetAddressString(addr, LONG_ADDR_SIZE, me);
	OSCProblem("OSCNewMethod: container %s already has %d methods; can't add another\n"
		   "Change MAX_METHODS_PER_CONTAINER in OSC-address-space.c and recompile.",
		   addr, me->numMethods);
	return 0;
    }

    m = AllocMethod();
    if (!m) return 0;

    m->callback = callback;
    m->context = context;
    m->QueryResponseInfo = *QueryResponseInfo;

    me->methodNames[me->numMethods] = name;
    me->methods[me->numMethods] = m;
    ++(me->numMethods);
    return m;
}

/**************************** Queries  ****************************/

void OSCInitContainerQueryResponseInfo(struct OSCContainerQueryResponseInfoStruct *i) {
    i->comment = 0;
}

void OSCInitMethodQueryResponseInfo(struct OSCMethodQueryResponseInfoStruct *i) {
    i->description = 0;
    i->pvq = 0;
} 

/******************************* Debug  ********************************/

#ifdef DEBUG

static int ContainerAliases(OSCcontainer c, char *target) {
    /* Write a space-delimited list of alias names in the given string,
       and return the number */
    int i, n;

    if (c == OSCTopLevelContainer) return 0;
    target[0] = '\0';
    n = 0;

    for (i = 0; i < c->parent->numChildren; ++i) {
	if (c->parent->children[i] == c) {
	    if (n > 0) {
		strcat(target, " ");
		strcat(target, c->parent->childrenNames[i]);
	    }
	    ++n;
	}
    }
    if (n == 0) fatal_error("ContainerAliases: internal inconsistency");
    
    return n-1;
}

#define BIG_ADDRESS 50

static void PrintHelp(OSCcontainer c) {
    char addr[BIG_ADDRESS];
    char aliasNames[1000];

    int i, j, numAliases;

    if (OSCGetAddressString(addr, BIG_ADDRESS, c) == FALSE) {
	printf("  /.../%s", ContainerName(c));
    } else {
	printf("  %s", addr);
    }

    numAliases = ContainerAliases(c, aliasNames);
    if (numAliases > 0) {
	printf(" (%d aliases:%s)", numAliases, aliasNames);
    }
    printf("\n");

    for (i = 0; i < c->numMethods; ++i) {
	printf("    %s%s: %s\n", addr, c->methodNames[i], 
	       c->methods[i]->QueryResponseInfo.description);
    }

    /* Forgive this quadratic kludge: */
    for (i = 0; i < c->numChildren; ++i) {
	for (j = 0; j < i; ++j) {
	    if (c->children[j] == c->children[i]) {
		/* c->children[i] is just an alias to c->children[j], 
		   which we already printed, so ignore it. */
		goto SkipAlias;
	    }
	}
	PrintHelp(c->children[i]);
	SkipAlias:
    }
}

void OSCPrintWholeAddressSpace(void) {
    printf("\n----- The OSC address space -----\n");
    PrintHelp(OSCTopLevelContainer);
    printf("...end of OSC address space.\n\n\n");
}


#endif /* DEBUG */


/***************************** Dispatching  *****************************/

#include "OSC-dispatch.h"
#include "OSC-callbacklist.h"
#include "OSC-pattern-match.h"

/* To do quick concatenation of singly-linked lists, we pass around 
   this data structure that points to the first and last elements: */

typedef struct callbackListEnds_struct {
    callbackList begin;
    callbackList end;
} callbackListEnds;

/* Helper proc. declarations */
static callbackListEnds DispatchSubMessage(char *pattern, OSCcontainer c);
static char *NextSlashOrNull(char *p);


callbackList OSCDispatchMessage(char *pattern) {
    callbackListEnds result;

    if (pattern[0] != '/') {
        OSCProblem("Invalid address \"%s\" does not begin with /", pattern);
        return 0;
    }

    result = DispatchSubMessage(pattern+1, OSCTopLevelContainer);

    return result.begin;
}

#define LONG_ADDR_LEN 100


static callbackListEnds DispatchSubMessage(char *pattern, OSCcontainer c) {
    callbackListEnds result;
    char *nextSlash, *restOfPattern;
    char offendingAddr[LONG_ADDR_LEN];
    int i, j;

    result.begin = result.end = 0;
    nextSlash = NextSlashOrNull(pattern);

    if (*nextSlash == '\0') {
	/* Base case: the pattern names methods of this container. */
	for (i = 0; i < c->numMethods; i++) {
	    if (PatternMatch(pattern, c->methodNames[i])) {
		callbackList node = AllocCallbackListNode(c->methods[i]->callback,
							  c->methods[i]->context,
							  result.begin);
		if (node == 0) {
		    /* Excuse the hairyness of the code to generate the error message. */
		    if (OSCGetAddressString(offendingAddr, 
					    LONG_ADDR_LEN-strlen(c->methodNames[i]),
					    c)) {
			strcat(offendingAddr, c->methodNames[i]);
		    } else {
			strcpy(offendingAddr, c->methodNames[i]);
		    }
			
		    OSCWarning("No memory for callback node; not invoking %s",
			       offendingAddr);
		} else {
		    if (result.end == 0) {
			result.end = node;
		    }
		    result.begin = node;
		}
	    }
	}
    } else {
	/* Recursive case: in the middle of an address, so the job at this 
	   step is to look for containers that match.  We temporarily turn
           the next slash into a null so pattern will be a null-terminated
	   string of the stuff between the slashes. */
	*nextSlash = '\0';
	restOfPattern = nextSlash + 1;

	for (i = 0; i < c->numChildren; ++i) {
	    if (PatternMatch(pattern, c->childrenNames[i])) {
		callbackListEnds subResult =	
		    DispatchSubMessage(restOfPattern, c->children[i]);
		if (result.end == 0) {
		    result = subResult;
		} else {
		    subResult.end->next = result.begin;
		    result.begin = subResult.begin;
		}
	    }
	}
	*nextSlash = '/';
    }
    return result;
}


static char *NextSlashOrNull(char *p) {
    while (*p != '/' && *p != '\0') {
	p++;
    }
    return p;
}

