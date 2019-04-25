/*
    array.h:

    Copyright (C) 2011, 2017 John ffitch and Stephen Kyne

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef __ARRAY_H__
#define __ARRAY_H__

/**
   Sets array to one-dimension of given size. Grows allocated array data space if
   currently allocated space is too small for new size.
 */
static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
  size_t ss = p->arrayMemberSize*size;
  if (p->data == NULL) {
    CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
    p->arrayMemberSize = var->memBlockSize;
    p->data = (MYFLT*)csound->Calloc(csound, ss);
    p->allocated = ss;
  }
  else if (ss > p->allocated) {
    p->data = (MYFLT*)csound->ReAlloc(csound, p->data, ss);
    p->allocated = ss;
  }
  p->sizes[0] = size;
  p->dimensions = 1;
}


#endif /* end of include guard: __ARRAY_H__ */
