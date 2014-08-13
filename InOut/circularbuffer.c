/*
  circularbuffer.c:

  Copyright (C) 2012 Victor Lazzarini

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/

#include <csoundCore.h>

typedef struct _circular_buffer {
  char *buffer;
  int  wp;
  int rp;
  int numelem;
  int elemsize; /* in number of bytes */
} circular_buffer;

void *csoundCreateCircularBuffer(CSOUND *csound, int numelem, int elemsize){
    circular_buffer *p;
    if ((p = (circular_buffer *)
         csound->Malloc(csound, sizeof(circular_buffer))) == NULL) {
      return NULL;
    }
    p->numelem = numelem;
    p->wp = p->rp = 0;
    p->elemsize = elemsize;

    if ((p->buffer = (char *) csound->Malloc(csound, numelem*elemsize)) == NULL) {
      return NULL;
    }
    memset(p->buffer, 0, numelem*elemsize);
    return (void *)p;
}

static int checkspace(circular_buffer *p, int writeCheck){
    int wp = p->wp, rp = p->rp, numelem = p->numelem;
    if(writeCheck){
      if (wp > rp) return rp - wp + numelem - 1;
      else if (wp < rp) return rp - wp - 1;
      else return numelem - 1;
    }
    else {
      if (wp > rp) return wp - rp;
      else if (wp < rp) return wp - rp + numelem;
      else return 0;
    }
}

int csoundReadCircularBuffer(CSOUND *csound, void *p, void *out, int items)
{
    IGN(csound);
    if (p == NULL) return 0;
    {
      int remaining;
      int itemsread, numelem = ((circular_buffer *)p)->numelem;
      int elemsize = ((circular_buffer *)p)->elemsize;
      int i=0, rp = ((circular_buffer *)p)->rp;
      char *buffer = ((circular_buffer *)p)->buffer;
      if ((remaining = checkspace(p, 0)) == 0) {
        return 0;
      }
      itemsread = items > remaining ? remaining : items;
      for (i=0; i < itemsread; i++){
        memcpy((char *) out + (i * elemsize),
               &(buffer[elemsize * rp++]),  elemsize);
        if (rp == numelem) {
          rp = 0;
        }
      }
#ifdef HAVE_ATOMIC_BUILTIN
      __sync_lock_test_and_set(&((circular_buffer *)p)->rp,rp);
#else
      ((circular_buffer *)p)->rp = rp;
#endif
      return itemsread;
    }
}

int csoundPeekCircularBuffer(CSOUND *csound, void *p, void *out, int items)
{
    IGN(csound);
    if (p == NULL) return 0;
    int remaining;
    int itemsread, numelem = ((circular_buffer *)p)->numelem;
    int elemsize = ((circular_buffer *)p)->elemsize;
    int i=0, rp = ((circular_buffer *)p)->rp;
    char *buffer = ((circular_buffer *)p)->buffer;
    if ((remaining = checkspace(p, 0)) == 0) {
        return 0;
    }
    itemsread = items > remaining ? remaining : items;
    for(i=0; i < itemsread; i++){
        memcpy((char *) out + (i * elemsize),
               &(buffer[elemsize * rp++]),  elemsize);
        if (rp == numelem) {
            rp = 0;
        }
    }
    return itemsread;
}

void csoundFlushCircularBuffer(CSOUND *csound, void *p)
{
    IGN(csound);
    if (p == NULL) return;
    int remaining;
    int itemsread, numelem = ((circular_buffer *)p)->numelem;
    int i=0, rp = ((circular_buffer *)p)->rp;
    //MYFLT *buffer = ((circular_buffer *)p)->buffer;
    if ((remaining = checkspace(p, 0)) == 0) {
        return;
    }
    itemsread = numelem > remaining ? remaining: numelem;
    for (i=0; i < itemsread; i++){
        rp++;
        if(rp == numelem) rp = 0;
    }
#ifdef HAVE_ATOMIC_BUILTIN
      __sync_lock_test_and_set(&((circular_buffer *)p)->rp,rp);
#else
      ((circular_buffer *)p)->rp = rp;
#endif
}


int csoundWriteCircularBuffer(CSOUND *csound, void *p, const void *in, int items)
{
    IGN(csound);
    if (p == NULL) return 0;
    int remaining;
    int itemswrite, numelem = ((circular_buffer *)p)->numelem;
    int elemsize = ((circular_buffer *)p)->elemsize;
    int i=0, wp = ((circular_buffer *)p)->wp;
    char *buffer = ((circular_buffer *)p)->buffer;
    if ((remaining = checkspace(p, 1)) == 0) {
        return 0;
    }
    itemswrite = items > remaining ? remaining : items;
    for(i=0; i < itemswrite; i++){
        memcpy(&(buffer[elemsize * wp++]),
                ((char *) in) + (i * elemsize),  elemsize);
        if(wp == numelem) wp = 0;
    }
#ifdef HAVE_ATOMIC_BUILTIN
      __sync_lock_test_and_set(&((circular_buffer *)p)->wp,wp);
#else
      ((circular_buffer *)p)->wp = wp;
#endif
    return itemswrite;
}

void csoundDestroyCircularBuffer(CSOUND *csound, void *p){
    if(p == NULL) return;
    csound->Free(csound, ((circular_buffer *)p)->buffer);
    csound->Free(csound, p);
}
