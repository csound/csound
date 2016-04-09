/*
    csGblMtx.h:

    Copyright (C) 2005 Istvan Varga

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
#ifndef CSOUND_CSGBLMTX_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

static pthread_mutex_t csound_global_lock_ = PTHREAD_MUTEX_INITIALIZER;

static void csound_global_mutex_init_(void)
{
}

static void csound_global_mutex_unlock(void)
{
    pthread_mutex_unlock(&csound_global_lock_);
}

static void csound_global_mutex_lock(void)
{
    pthread_mutex_lock(&csound_global_lock_);
}

static void csound_global_mutex_destroy_(void)
{
}

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_CSGBLMTX_H */

