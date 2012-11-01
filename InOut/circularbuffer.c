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
  MYFLT *buffer;
  int  wp;
  int rp;
  int size;
} circular_buffer;

void *csoundCreateCircularBuffer(CSOUND *csound, int size){
  circular_buffer *p;
  if ((p = (circular_buffer *) csound->Malloc(csound, sizeof(circular_buffer))) == NULL) {
    return NULL;
  }
  p->size = size;
  p->wp = p->rp = 0;
   
  if ((p->buffer = (MYFLT *) csound->Malloc(csound, size*sizeof(MYFLT))) == NULL) {
    free (p);
    return NULL;
  }
  memset(p->buffer, 0, size*sizeof(MYFLT));
  return (void *)p;
}

static int checkspace(circular_buffer *p, int writeCheck){
  int wp = p->wp, rp = p->rp, size = p->size;
  if(writeCheck){
    if (wp > rp) return rp - wp + size - 1;
    else if (wp < rp) return rp - wp - 1;
    else return size - 1;
  }
  else {
    if (wp > rp) return wp - rp;
    else if (wp < rp) return wp - rp + size;
    else return 0;
  }	
}

int csoundReadCircularBuffer(CSOUND *csound, void *p, MYFLT *out, int items){
  int remaining;
  int itemsread, size = ((circular_buffer *)p)->size;
  int i=0, rp = ((circular_buffer *)p)->rp;
  MYFLT *buffer = ((circular_buffer *)p)->buffer;
  if(p == NULL) return 0;
  if ((remaining = checkspace(p, 0)) == 0) {
    return 0;
  }
  itemsread = items > remaining ? remaining : items;
  for(i=0; i < itemsread; i++){
    out[i] = buffer[rp++];
    if(rp == size) rp = 0;
  }
  ((circular_buffer *)p)->rp = rp;
  return itemsread;
}

int csoundWriteCircularBuffer(CSOUND *csound, void *p, const MYFLT *in, int items){
  int remaining;
  int itemswrite, size = ((circular_buffer *)p)->size;
  int i=0, wp = ((circular_buffer *)p)->wp;
  MYFLT *buffer = ((circular_buffer *)p)->buffer;
  if(p == NULL) return 0;
  if ((remaining = checkspace(p, 1)) == 0) {
    return 0;
  }
  itemswrite = items > remaining ? remaining : items;
  for(i=0; i < itemswrite; i++){
    buffer[wp++] = in[i];
    if(wp == size) wp = 0;
  }
  ((circular_buffer *)p)->wp = wp;
  return itemswrite;
}

void csoundFreeCircularBuffer(CSOUND *csound, void *p){
  if(p == NULL) return;
  csound->Free(csound, ((circular_buffer *)p)->buffer);
  csound->Free(csound, p);
}
