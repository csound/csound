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

#if defined(__APPLE__) && defined(__MACH__)
#define ERR_STR_LEN 255
#include <mach-o/dyld.h>

/* Set and get the error string for use by dlerror */
static const char *error(int setget, const char *str, ...)
{
    static char errstr[ERR_STR_LEN];
    static int err_filled = 0;
    const char *retval;
    NSLinkEditErrors ler;
    int lerno;
    const char *dylderrstr;
    const char *file;
    va_list arg;
    if (setget <= 0) {
      va_start(arg, str);
      strncpy(errstr, "dlsimple: ", ERR_STR_LEN);
      vsnprintf(errstr + 10, ERR_STR_LEN - 10, str, arg);
      va_end(arg);
      /* We prefer to use the dyld error string if getset is 1*/
      if (setget == 0) {
        NSLinkEditError(&ler, &lerno, &file, &dylderrstr);
        fprintf(stderr,"dyld: %s\n",dylderrstr);
        if (dylderrstr && strlen(dylderrstr))
          strncpy(errstr,dylderrstr,ERR_STR_LEN);
      }       
      err_filled = 1;
      retval = NULL;
    }
    else {
      if (!err_filled)
        retval = NULL;
      else
        retval = errstr;
      err_filled = 0;
    }
    return retval;
}

/* dlsymIntern is used by dlsym to find the symbol */
void *dlsymIntern(void *handle, const char *symbol)
{
    NSSymbol *nssym = 0;
    /* If the handle is -1, if is the app global context */
    if (handle == (void *)-1) {
      /* Global context, use NSLookupAndBindSymbol */
      if (NSIsSymbolNameDefined(symbol)) {
        nssym = NSLookupAndBindSymbol(symbol);
      }
    }
    /* Now see if the handle is a struch mach_header* or not,
       use NSLookupSymbol in image for libraries, and
       NSLookupSymbolInModule for bundles */
    else {
      /* Check for both possible magic numbers depending
         on x86/ppc byte order */
      if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
          (((struct mach_header *)handle)->magic == MH_CIGAM)) {
        if (NSIsSymbolNameDefinedInImage((struct mach_header *)handle,
                                         symbol)) {
          nssym =
            NSLookupSymbolInImage((struct mach_header *)handle,
                                  symbol,
                                  NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                  NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
        }
        
      }
      else {
        nssym = NSLookupSymbolInModule(handle, symbol);
      }
    }
    if (!nssym) {
      error(0, "Symbol \"%s\" Not found", symbol);
      return NULL;
    }
    return NSAddressOfSymbol(nssym);
}

void *dlopen(const char *path, int mode)
{
    void *module = 0;
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;
    static int (*make_private_module_public) (NSModule module) = 0;
    unsigned int flags =  NSLINKMODULE_OPTION_RETURN_ON_ERROR |
                          NSLINKMODULE_OPTION_PRIVATE;

    /* If we got no path, the app wants the global namespace,
       use -1 as the marker in this case */
    if (!path)
      return (void *)-1;

    /* Create the object file image, works for things linked
       with the -bundle arg to ld */
    ofirc = NSCreateObjectFileImageFromFile(path, &ofi);
    switch (ofirc) {
    case NSObjectFileImageSuccess:
      /* It was okay, so use NSLinkModule to link in the image */
      module = NSLinkModule(ofi, path,flags);
      /* Don't forget to destroy the object file
         image, unless you like leaks */
      NSDestroyObjectFileImage(ofi);
      /* If the mode was global, then change the module, this avoids
         multiply defined symbol errors to first load private then
         make global. Silly, isn't it. */
      
      if (!make_private_module_public) {
        _dyld_func_lookup("__dyld_NSMakePrivateModulePublic", 
                          (unsigned long *)&make_private_module_public);
      }
      make_private_module_public(module);
      break;
    case NSObjectFileImageInappropriateFile:
      /* It may have been a dynamic library rather
         than a bundle, try to load it */
      module = (void *)NSAddImage(path, 
                                  NSADDIMAGE_OPTION_RETURN_ON_ERROR);
      break;
    case NSObjectFileImageFailure:
      error(0,"Object file setup failure :  \"%s\"", path);
      return 0;
    case NSObjectFileImageArch:
      error(0,"No object for this architecture :  \"%s\"", path);
      return 0;
    case NSObjectFileImageFormat:
      error(0,"Bad object file format :  \"%s\"", path);
      return 0;
    case NSObjectFileImageAccess:
      error(0,"Can't read object file :  \"%s\"", path);
      return 0;       
    }
    if (!module)
      error(0, "Can not open \"%s\"", path);
    return module;
}

int dlclose(void *handle)
{
    if ((((struct mach_header *)handle)->magic == MH_MAGIC) ||
        (((struct mach_header *)handle)->magic == MH_CIGAM)) {
      error(-1, "Can't remove dynamic libraries on darwin");
      return 0;
    }
    if (!NSUnLinkModule(handle, 0)) {
      error(0, "unable to unlink module %s", NSNameOfModule(handle));
      return 1;
    }
    return 0;
}


const char *dlerror(void)
{
    return error(1, (char *)NULL);
}

/* dlsym, prepend the underscore and call dlsymIntern */
void *dlsym(void *handle, const char *symbol)
{
    static char undersym[257];  /* Saves calls to malloc(3) */
    int sym_len = strlen(symbol);
    void *value = NULL;
    char *malloc_sym = NULL;

    if (sym_len < 256) {
      snprintf(undersym, 256, "_%s", symbol);
      value = dlsymIntern(handle, undersym);
    }
    else {
      malloc_sym = malloc(sym_len + 2);
      if (malloc_sym) {
        sprintf(malloc_sym, "_%s", symbol);
        value = dlsymIntern(handle, malloc_sym);
        free(malloc_sym);
      }
      else {
        error(-1, "Unable to allocate memory");
      }
    }
    return value;
}

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
    const char *dle;
#elif WIN32
    long handle;
#else
    void *handle = NULL;
#endif
    char *error;
    char buffer[256];
    OENTRY *(*init)(ENVIRON*);
    long (*size)(void);
    void (*resetter)(ENVIRON*);

    if (cenviron.oplibs_==NULL) return 1;
    printf("Loading libraries %s\n", cenviron.oplibs_);
    strcpy(buffer, cenviron.oplibs_);
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
      if(handle == NULL) {

#ifdef HAVE_LIBDL
        char *errstr;

        errstr = dlerror();
        if (errstr != NULL) {
          printf ("A dynamic linking error occurred: (%s)\n", errstr);
        }
#endif

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
        printf("Length=%ld\n", length);
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
        opcodlst_n = (*init)(&cenviron);
        olength = oplstend-opcodlst;
        resetter = dlsym(handle, "RESET");
        dle = dlerror();
        if (dle==NULL) {
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
        init = (OENTRY *(*)(ENVIRON*))GetProcAddress((HINSTANCE)handle,
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
        opcodlst_n = (*init)(&cenviron);
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
        printf("Got opcodlst %p\noplstend=%p, opcodlst=%p, length=%ld\n",
               opcodlst_n, oplstend, opcodlst, olength);
        printf("Adding %ld(%ld) -- first opcode is %s\n",
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

