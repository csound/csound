/*  
    dl_opcodes.c:

    Copyright (C) 2002 John ffitch

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

#include "cs.h"
#ifdef HAVE_LIBDL
#include <dlfcn.h>
#elif WIN32
#define PUBLIC __declspec(dllexport)
#define LIBRARY_CALL WINAPI
#include <windows.h>
#endif

int csoundLoadExternal(void)
{
    return 0;
}

int csoundLoadExternals(void)
{
    char *libname;
#ifdef HAVE_LIBDL
    void *handle;
#elif WIN32
    long handle;
#else
    void *handle = NULL;
#endif
    char *error;
    char buffer[256];
    OENTRY *(*init)(GLOBALS*);
    long (*size)(void);
    void (*resetter)(void);

    if (cglob.oplibs==NULL) return 1;
    printf("Loading libraries %s\n", cglob.oplibs);
    strcpy(buffer, cglob.oplibs);
    libname = strtok(buffer, ",");
#ifdef BETA
    printf("libname %s\n", libname);
#endif
    while (libname!=NULL) {
#ifdef HAVE_LIBDL
      handle = dlopen (libname, RTLD_NOW | RTLD_GLOBAL);
#elif WIN32
      handle = (long) LoadLibrary(libname);
#endif
      if (!handle) {
        printf("Failed to load %s\n", libname);
      }
      else {
        OENTRY *opcodlst_n;
        long length, olength;
#ifdef BETA
        printf("Found handle\n");
#endif
#ifdef HAVE_LIBDL
        size = dlsym(handle, "opcode_size");
        if ((error = dlerror()) != NULL)  {
          printf("Failed to initialise %s: %s", libname, error);
          goto next;
        }
#ifdef BETA
        printf("Found size\n");
#endif
        length = (*size)();
#ifdef BETA
        printf("Length=%d\n", length);
#endif
        init = dlsym(handle, "opcode_init");
#ifdef BETA
        printf("Found init\n");
#endif
        if ((error = dlerror()) != NULL)  {
          printf("Failed to initialise %s: %s", libname, error);
          goto next;
        }
#ifdef BETA
        printf("Calling init\n");
#endif
        opcodlst_n = (*init)(&cglob);
        olength = oplstend-opcodlst;
        resetter = dlsym(handle, "RESET");
        if (dlerror()==NULL) {
          RESETTER *x = (RESETTER*)mmalloc(sizeof(RESETTER));
          x->fn = resetter;
          x->next = reset_list;
          reset_list = x;
        }
#elif WIN32
        size = (long(*)(void))GetProcAddress((HINSTANCE)handle, "opcode_size");
        if (size == NULL)  {
          printf("Failed to initialise %s", libname);
          goto next;
        }
#ifdef BETA
        printf("Found size\n");
#endif
        length = (*size)();
#ifdef BETA
        printf("Length=%d\n", length);
#endif
        init = (OENTRY *(*)(GLOBALS*))GetProcAddress((HINSTANCE)handle,
                                                     "opcode_init");
#ifdef BETA
        printf("Found init\n");
#endif
        if (init == NULL)  {
          printf("Failed to initialise %s", libname);
          goto next;
        }
#ifdef BETA
        printf("Calling init\n");
#endif
        opcodlst_n = (*init)(&cglob);
        olength = oplstend-opcodlst;
        resetter = (void(*)(void))GetProcAddress((HINSTANCE)handle, "RESET");
        if (resetter==NULL) {
          RESETTER *x = (RESETTER*)mmalloc(sizeof(RESETTER));
          x->fn = resetter;
          x->next = reset_list;
          reset_list = x;
        }
#endif

#ifdef BETA
        printf("Got opcodlst %p\noplstend=%p, opcodlst=%p, length=%d\n",
               opcodlst_n, oplstend, opcodlst, olength);
        printf("Adding %d(%d) -- first opcode is %s\n",
               length, length/sizeof(OENTRY), opcodlst_n[0].opname);
#endif
        opcodlst = (OENTRY*) mrealloc(opcodlst, olength*sizeof(OENTRY) + length);
        memcpy(opcodlst+olength, opcodlst_n, length);
        oplstend = opcodlst +  olength + length/sizeof(OENTRY);
      }
    next:
      libname = strtok(NULL, ",");
    }
#ifdef BETA
    printf("All loaded\n");
#endif
    return 1;
}

