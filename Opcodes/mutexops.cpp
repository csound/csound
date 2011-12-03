/*
    mutexops.cpp:

    Copyright (C) 2007 by Steven Yi

    Mutex lock and unlocking opcodes, used for when running Csound with
    multiple threads

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
#include "csdl.h"
#include "csGblMtx.h"
#include <map>

static std::map<CSOUND *, std::map<size_t, std::map<size_t, void *> > > mutexes;

typedef struct {
        OPDS    h;
        MYFLT   *ilocknum;
        void    *mutex;
} OPCODE_MUTEX;

static void * getMutex(CSOUND *csound, size_t instrNum, size_t lockNum)
{

    csound_global_mutex_lock();
    if(mutexes.find(csound) == mutexes.end()) {
        std::map<size_t, std::map<size_t, void *> > values;
        mutexes[csound] = values;
    }

    if(mutexes[csound].find(instrNum) == mutexes[csound].end()) {
        std::map<size_t, void*> mutexMap;
        mutexes[csound][instrNum] = mutexMap;
    }

    if (mutexes[csound][instrNum].find(lockNum) ==
        mutexes[csound][instrNum].end()) {
        void * mutex = csound->Create_Mutex(0);
        mutexes[csound][instrNum][lockNum] = mutex;

        csound->Message(csound, "Created new mutex [%ld:%ld]\n",
                        (long)instrNum, (long)lockNum);
    }
    csound_global_mutex_unlock();

    return mutexes[csound][instrNum][lockNum];

}

static int mutexLock(CSOUND *csound, OPCODE_MUTEX *p)
{

    if(p->mutex == NULL) {
        size_t instrNum = static_cast<size_t>(p->h.insdshead->p1);
        size_t lockNum = static_cast<size_t>(*p->ilocknum);

        p->mutex = getMutex(csound, instrNum, lockNum);
    }

    csound->LockMutex(p->mutex);

    return OK;
}

static int mutexUnlock(CSOUND *csound, OPCODE_MUTEX *p)
{
    if(p->mutex == NULL) {
        size_t instrNum = static_cast<size_t>(p->h.insdshead->p1);
        size_t lockNum = static_cast<size_t>(*p->ilocknum);

        p->mutex = getMutex(csound, instrNum, lockNum);
    }

    csound->UnlockMutex(p->mutex);

    return OK;
}

#define S(x)    sizeof(x)

OENTRY mutex_localops[] = {
  { (char*)"mutex_lock",   S(OPCODE_MUTEX),  2, (char*)"", (char*)"i",
    NULL, (SUBR)mutexLock, NULL   },
  { (char*)"mutex_unlock",  S(OPCODE_MUTEX), 2, (char*)"", (char*)"i",
    NULL, (SUBR)mutexUnlock, NULL },
  { (char*)"mutex_locki",  S(OPCODE_MUTEX),  1, (char*)"", (char*)"i",
    (SUBR)mutexLock, NULL, NULL   },
  { (char*)"mutex_unlocki", S(OPCODE_MUTEX), 1, (char*)"", (char*)"i",
    (SUBR)mutexUnlock, NULL, NULL }
};


LINKAGE1(mutex_localops)
