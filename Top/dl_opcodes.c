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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "cs.h"

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#elif WIN32
#define PUBLIC __declspec(dllexport)
#define LIBRARY_CALL WINAPI
#include <windows.h>
#endif

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#endif

#if defined(WIN32) && !defined(__CYGWIN__)

#include <io.h>
#include <direct.h>
#include <windows.h>

void *csoundOpenLibrary(const char *libraryPath)
{
    void *library = 0;
    if(strstr(libraryPath, ".dll") || strstr(libraryPath, ".DLL")) {
      library = (void *) LoadLibrary(libraryPath);
    }
    return library;
}

void *csoundCloseLibrary(void *library)
{
    return FreeLibrary((HINSTANCE) library);
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    void *procedureAddress = 0;
    procedureAddress = GetProcAddress((HINSTANCE) library, procedureName);
    return procedureAddress;
}

#elif defined(LINUX) || defined(__CYGWIN__)

#include <dlfcn.h>
#include <dirent.h>
#include <string.h>

void *csoundOpenLibrary(const char *libraryPath)
{
    void *library = 0;
#if defined(LINUX)
    if
      (strstr(libraryPath, ".so")
#elif defined(__CYGWIN__)
       (strstr(libraryPath, ".dll") || strstr(libraryPath, ".DLL"))
/* #elif defined(__MACH__) */
/*        (strstr(libraryPath, ".dylib")) */
#endif
       ) {
      library = dlopen(libraryPath, RTLD_NOW | RTLD_GLOBAL );
      if(!library) {
        fprintf(stderr, "Error '%s' in dlopen(%s).\n", dlerror(), libraryPath);
      }
    }
    return library;
}

void *csoundCloseLibrary(void *library)
{
  void *returnValue = 0;
  returnValue = (void *)dlclose(library);
  return returnValue;
}

void *csoundGetLibrarySymbol(void *library,
                             const char *procedureName)
{
    void *procedureAddress = dlsym(library, procedureName);
    if (!procedureAddress) {
      printf("Failed to find %s in library: %s\n", procedureName, dlerror());
    }
    return procedureAddress;
}

#elif defined(__MACH__)

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
    else  {
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
          nssym = NSLookupSymbolInImage(
                            (struct mach_header *)handle,
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

void *csoundOpenLibrary(const char *libraryPath)
{
    void *module = 0;
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;
    static int (*make_private_module_public) (NSModule module) = NULL;
    unsigned int flags =  NSLINKMODULE_OPTION_RETURN_ON_ERROR |
                          NSLINKMODULE_OPTION_PRIVATE;
    if(O.odebug) {
      printf("csoundOpenLibrary\n");
    }
    /* If we got no path, the app wants the global namespace,
       use -1 as the marker in this case */
    if (!libraryPath)
      return (void *)-1;
    /* Create the object file image, works for things linked
       with the -bundle arg to ld */
    ofirc = NSCreateObjectFileImageFromFile(libraryPath, &ofi);
    if(O.odebug) {
      printf("ofirc=%d\n", ofirc);
    }
    switch (ofirc) {
    case NSObjectFileImageSuccess:
      if(O.odebug) {
        printf("ofirc=NSObjectFileImageSuccess\n");
      }
      /* It was okay, so use NSLinkModule to link in the image */
      module = NSLinkModule(ofi, libraryPath,flags);
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
      if(O.odebug) {
        printf("ofirc=NSObjectFileImageInappropriateFile\n");
      }
      /* It may have been a dynamic library rather
         than a bundle, try to load it */
      module = (void *)NSAddImage(libraryPath,
                                  NSADDIMAGE_OPTION_RETURN_ON_ERROR);
      break;
    case NSObjectFileImageFailure:
      error(0,"Object file setup failure :  \"%s\"", libraryPath);
      return 0;
    case NSObjectFileImageArch:
      error(0,"No object for this architecture :  \"%s\"", libraryPath);
      return 0;
    case NSObjectFileImageFormat:
      error(0,"Bad object file format :  \"%s\"", libraryPath);
      return 0;
    case NSObjectFileImageAccess:
      error(0,"Can't read object file :  \"%s\"", libraryPath);
      return 0;
    }
    if (!module)
      error(0, "Can not open \"%s\"", libraryPath);
    return module;
}

void *csoundCloseLibrary(void *library)
{
    if ((((struct mach_header *)library)->magic == MH_MAGIC) ||
        (((struct mach_header *)library)->magic == MH_CIGAM)) {
      error(-1, "Can't remove dynamic libraries on darwin");
      return NULL;
    }
    if (!NSUnLinkModule(library, 0)) {
      error(0, "unable to unlink module %s", NSNameOfModule(library));
      return NULL;
    }
    return NULL;
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    static char undersym[257];  /* Saves calls to malloc(3) */
    int sym_len = strlen(procedureName);
    void *value = NULL;
    char *malloc_sym = NULL;
    if (sym_len < 256) {
      snprintf(undersym, 256, "_%s", procedureName);
      value = dlsymIntern(library, undersym);
    }
    else {
      malloc_sym = malloc(sym_len + 2);
      if (malloc_sym) {
        sprintf(malloc_sym, "_%s", procedureName);
        value = dlsymIntern(library, malloc_sym);
        free(malloc_sym);
      }
      else {
        error(-1, "Unable to allocate memory");
      }
    }
    return value;
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
      if(O.odebug) {
        printf("ofirc=NSObjectFileImageSuccess\n");
      }
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
       if(O.odebug) {
        printf("ofirc=NSObjectFileImageInappropriateFile\n");
      }
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

#else   /* case for platforms without shared libraries -- added 062404, akozar */

void *csoundOpenLibrary(const char *libraryPath)
{
	void *library = NULL;
	return library;
}

void *csoundCloseLibrary(void *library)
{
        void *returnValue = NULL;
        return returnValue;
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
        void *procedureAddress = NULL;
        return procedureAddress;
}


#endif

int csoundLoadExternal(void *csound, const char* libraryPath)
{
    void *handle;
    OENTRY *opcodlst_n;
    long length, olength;
    OENTRY *(*init)(ENVIRON*);
    long (*size)(void);
    if(O.odebug) {
      printf("Trying to open file '%s' as library.\n", libraryPath);
    }
    handle = csoundOpenLibrary(libraryPath);
    if(!handle) {
      return -1;
    }
    if(O.odebug) {
      printf("Found library handle.\n");
    }
    size = csoundGetLibrarySymbol(handle, "opcode_size");
    if(!size) {
      return -1;
    }
    if(O.odebug) {
      printf("Found 'opcode_size' function.\n");
    }
    length = (*size)();
    if(O.odebug) {
      printf("Length=%d\n", length);
    }
    init = csoundGetLibrarySymbol(handle, "opcode_init");
    if(!init) {
      return -1;
    }
    if(O.odebug) {
      printf("Found 'opcode_init' function.\n");
      printf("Calling 'opcode_init'.\n");
    }
    opcodlst_n = (*init)(&cenviron);
    olength = oplstend-opcodlst;
    if(O.odebug) {
      printf("Got opcodlst 0x%x\noplstend=0x%x, opcodlst=0x%x, length=%d.\n",
             opcodlst_n, oplstend, opcodlst, olength);
      printf("Adding %d bytes (%d opcodes) -- first opcode is '%s'.\n",
             length, length/sizeof(OENTRY), opcodlst_n[0].opname);
    }
    opcodlst = (OENTRY*) mrealloc(opcodlst, olength*sizeof(OENTRY) + length);
    memcpy(opcodlst+olength, opcodlst_n, length);
    oplstend = opcodlst + olength + length/sizeof(OENTRY);
    return 0;
}

int csoundLoadExternals(void *csound)
{
    char *libname;
    char buffer[256];
#if defined(HAVE_DIRENT_H)
    {
      DIR *directory;
      struct dirent *file;
      char buffer[0x500];
      char *opcodedir = getenv("OPCODEDIR");
      if(O.odebug) {
	printf("OPCODEDIR='%s'.\n", opcodedir?opcodedir:"(null)");
      }
      if(!opcodedir) {
        opcodedir = ".";
      }
      if(O.odebug) {
	printf("OPCODEDIR='%s'.\n", opcodedir?opcodedir:"(null)");
      }
      directory = opendir(opcodedir);
      while((file = readdir(directory)) != 0) {
        sprintf(buffer, "%s%c%s", opcodedir, DIRSEP, file->d_name);
        csoundLoadExternal(csound, buffer);
      }
      closedir(directory);
    }
#endif
    if(((ENVIRON *)csound)->oplibs_ == NULL) {
      return 1;
    }
    printf("Loading command-line libraries '%s'.\n", ((ENVIRON *)csound)->oplibs_);
    strcpy(buffer, ((ENVIRON *)csound)->oplibs_);
    libname = strtok(buffer, ",");
    while(libname != NULL) {
      csoundLoadExternal(csound, libname); /* error code not currently checked!*/
      libname = strtok(NULL, ",");
    }
    return 1;
}

