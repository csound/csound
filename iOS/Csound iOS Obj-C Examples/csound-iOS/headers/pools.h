/*
    pools.h:

    Copyright (C) 2013 by Victor Lazzarini

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

#ifndef POOLS_H
#define POOLS_H

#define POOL_SIZE 256

typedef struct myflt_pool {
    CS_VAR_MEM* values;
    int max;
    int count;
} MYFLT_POOL;

MYFLT_POOL* myflt_pool_create(CSOUND* csound);
int myflt_pool_indexof(MYFLT_POOL* pool, MYFLT value);
int myflt_pool_find_or_add(CSOUND* csound, MYFLT_POOL* pool, MYFLT value);
int myflt_pool_find_or_addc(CSOUND* csound, MYFLT_POOL* pool, char* s);
void myflt_pool_free(CSOUND *csound, MYFLT_POOL *pool);

#endif

