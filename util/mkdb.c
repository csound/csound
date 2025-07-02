/*
    mkdb.c:

    Copyright (C) 2020 John ffitch

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

/* Linux Compile with gcc -g -DLINUX mkdb.c -lm -dl -o ../mkdb
 * Windows cc -DWIN32 mkdb.c -o ../mkdb.exe
 * Mac ??
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>

typedef void (*sighandler_t)(int);

sigjmp_buf env;

void handler(int32_t sig)
{
    siglongjmp(env, 1);
}

#if defined(WIN32)

#define DIRSEP '\\'

int32_t csOpenLibrary(void **library, const char *libraryPath)
{
    *library = (void*) LoadLibrary(libraryPath);
    return (*library != NULL ? 0 : -1);
}

int32_t csCloseLibrary(void *library)
{
    return (int) (FreeLibrary((HMODULE) library) == FALSE ? -1 : 0);
}

void *csGetLibrarySymbol(void *library, const char *procedureName)
{
    return (void*) GetProcAddress((HMODULE) library, procedureName);
}

#elif  (defined(LINUX) || defined(NEW_MACH_CODE) || defined(__HAIKU__))

#define DIRSEP '/'

int32_t csOpenLibrary(void **library, const char *libraryPath)
{
    int32_t flg = RTLD_NOW;
    if (libraryPath != NULL) {
      int32_t len = (int) strlen(libraryPath);
      /* ugly hack to fix importing modules in Python opcodes */
      if (len >= 9 && strcmp(&(libraryPath[len - 9]), "/libpy.so") == 0)
        flg |= RTLD_GLOBAL;
      if (len >= 12 && strcmp(&(libraryPath[len - 12]), "/libpy.dylib") == 0)
        flg |= RTLD_GLOBAL;
    }
    *library = (void*) dlopen(libraryPath, flg);
    return (*library != NULL ? 0 : -1);
}

int32_t csCloseLibrary(void *library)
{
    return (int) dlclose(library);
}

void *csGetLibrarySymbol(void *library, const char *procedureName)
{
    return (void*) dlsym(library, procedureName);
}

#else /* case for platforms without shared libraries -- added 062404, akozar */

int32_t csOpenLibrary(void **library, const char *libraryPath)
{
    *library = NULL;
    return -1;
}

int32_t csCloseLibrary(void *library)
{
    return 0;
}

void *csGetLibrarySymbol(void *library, const char *procedureName)
{
    return NULL;
}

#endif

/* Copied from csoundCore.h */
typedef struct oentry {
        char    *opname;
        uint16_t dsblksiz;
        uint16_t flags;
        char    *outypes;
        char    *intypes;
        int32_t     (*init)(void *, void *p);
        int32_t     (*perf)(void *, void *p);
        int32_t     (*deinit)(void *, void *p);
        void    *useropinfo;    /* user opcode parameters */
} OENTRY;

typedef int32_t (*SUBR)(void *, void *);

typedef struct _cs {
    SUBR dummy[190];
  int32_t (*AppendOpcode)(void *, char *);
  int32_t (*AppendOpcodes)(void *, OENTRY *, int);
      } CS;

char *gfname;                   /* Global so can be printed in fn below */

int32_t csAppendOpcode(void *cs, char* op)
{
    printf("Opcode %s in %s\n", op, gfname);
}

int32_t csAppendOpcodes(void *cs, OENTRY *o, int32_t n)
{
    while (n>0) {
      csAppendOpcode(cs, o->opname);
      o++; n--;
    }
}


/* load a single plugin library, and run csModuleCreate() if present */
/* returns zero on success */

static int32_t csLoadExternal(const char *libraryPath)
{
    char            *fname;
    void            *h, *p;
    int32_t             err;
    OENTRY          *O;
    long (*opcode_init)(void *, OENTRY **);

    /* check for a valid name */
    if (libraryPath == NULL || libraryPath[0] == '\0')
      return 1;
    /* remove leading directory components from name */
    fname = (char*) libraryPath + (int) strlen(libraryPath);
    for ( ; fname[0] != DIRSEP && fname != (char*) libraryPath; fname--)
      ;
    if (fname[0] == DIRSEP)
      fname++;
    if (fname[0] == '\0')
      return 2;
    /* load library */
    printf("Library '%s'\n", libraryPath);
    err = csOpenLibrary(&h, libraryPath);
    if (err) {
 #if (defined(LINUX) || defined(__HAIKU__))
      fprintf(stderr, "could not open library '%s' (%s)\n",
               libraryPath, dlerror());
 #else
      fprintf(stderr, "could not open library '%s' (%d)\n",
               libraryPath, err);
 #endif
      return 3;
    }
    /* find out if it is a Cs plugin */
    opcode_init =
          (long (*)(void *, OENTRY **))
              csGetLibrarySymbol(h, "csound_opcode_init");
    if (opcode_init != NULL) {
      opcode_init(NULL, &O);
      while (O && O->opname!=NULL) {
        printf("Opcode %s in %s\n", O->opname, fname);
        O += 1;
      }
    }
    else {                      /* Could be C++ type.... */
      int32_t (*init_func)(void *);
      CS cs;
      //printf("no init\n");
      init_func = (int32_t (*)(void*))csGetLibrarySymbol(h, "csoundModuleInit");
      if (init_func == NULL)  return 4;
      // printf("Possibly C++ linkage\n");
      // We may be able to call this with a dummy argument to give access
      // to a version of AppendOpcode that just prints the second argument
      // Constructing the dummy CSOUND obect might be a pain
      memset(&cs, '\0', sizeof(CS));
      cs.AppendOpcode = csAppendOpcode;
      cs.AppendOpcodes = csAppendOpcodes;
      /* printf("&cs = %p, fuctions %p, %p calling %p\n", */
      /*        &cs, csAppendOpcode, csAppendOpcodes, init_func); */
      /* printf("fuctions %p %p\n", (&cs)->AppendOpcode, (&cs)->AppendOpcodes); */
      gfname = fname;
      init_func(&cs);
    }
    return 0;
}

int32_t main(int32_t argc, char**argv)
{
    int32_t i = 1;
    signal(SIGSEGV, handler);
    while (argc>1) {
      // This needs protection
      if (sigsetjmp(env, 1) == 0)
        csLoadExternal(argv[i]);
      argc--; i++;
    }
}
