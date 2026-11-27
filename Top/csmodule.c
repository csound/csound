/*
  csmodule.c:

  Copyright (C) 2005 Istvan Varga
  based on dl_opcodes.c, Copyright (C) 2002 John ffitch

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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/******************************************************************************
 * NEW PLUGIN INTERFACE                                                       *
 * ====================                                                       *
 *                                                                            *
 * Plugin libraries are loaded from the directory defined by the environment  *
 * variable OPCODE7DIR (or the current directory if OPCODE7DIR is unset) by   *
 * csoundPreCompile() while initialising a Csound instance, and are unloaded  *
 * at the end of performance by csoundReset().                                *
 * A library may export any of the following five interface functions,        *
 * however, the presence of csoundModuleCreate() is required for identifying  *
 * the file as a Csound plugin module.                                        *
 *                                                                            *
 * int32_t csoundModuleCreate(CSOUND *csound)       (required)                *
 * --------------------------------------                                     *
 *                                                                            *
 * Pre-initialisation function, called by csoundPreCompile().                 *
 *                                                                            *
 * int32_t csoundModuleInit(CSOUND *csound)         (optional)                *
 * ------------------------------------                                       *
 *                                                                            *
 * Called by Csound instances before orchestra translation. One possible use  *
 * of csoundModuleInit() is adding new opcodes with csoundAppendOpcode().     *
 *                                                                            *
 * int32_t csoundModuleDestroy(CSOUND *csound)      (optional)                *
 * ---------------------------------------                                    *
 *                                                                            *
 * Destructor function for Csound instance 'csound', called at the end of     *
 * performance, after closing audio output.                                   *
 *                                                                            *
 * const char *csoundModuleErrorCodeToString(int32_t errcode)   (optional)    *
 * ------------------------------------------------------                     *
 *                                                                            *
 * Converts error codes returned by any of the initialisation or destructor   *
 * functions to a string message.                                             *
 *                                                                            *
 * int32_t csoundModuleInfo(void)                   (required)                *
 * --------------------------                                                 *
 *                                                                            *
 * Return CSOUND_MODULE_INFO (from csdl.h) to identify the plugin ABI.         *
 * Bits 16 and up hold CS_VERSION; bits 8-15 hold CS_SUBVER.                   *
 * Bits 0-6 hold sizeof(cs_float); bit 7 marks USE_FLOAT (32-bit cs_double).    *
 * A missing function, or a return value of 0, cannot be validated against    *
 * this Csound, so such a plugin is not loaded.                               *
 *                                                                            *
 * csoundModuleMetadata                             (optional)                *
 * --------------------                                                       *
 *                                                                            *
 * A data record with the plugin's own version and a free text, defined with  *
 * CSOUND_PLUGIN_METADATA (from csdl.h). The loader does not read it: Csound  *
 * and other tools read it from the library file (Top/plugin_metadata.c).     *
 *                                                                            *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !(defined (__wasi__))
#include <setjmp.h>
#include <errno.h>
#endif

#include "csoundCore.h"
#include "csmodule.h"
#include "plugin_metadata.h"
#ifdef HAVE_WASMTIME
#include "wasm_plugins.h"
#endif

#if defined(__MACH__)
#include <TargetConditionals.h>
#if (TARGET_OS_IPHONE == 0) && (TARGET_IPHONE_SIMULATOR == 0)
#if defined(MAC_OS_X_VERSION_10_6) &&                           \
  (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_6)
#define NEW_MACH_CODE
#endif
#endif
#endif

#if !(defined (__wasi__))
#if defined(LINUX) || defined(BSD) || defined(NEW_MACH_CODE) || defined(__HAIKU__)
#include <dlfcn.h>
#elif defined(WIN32)
#include <windows.h>
#endif
#endif


#if defined(HAVE_DIRENT_H)
#  include <dirent.h>
#  if 0 && defined(__MACH__)
typedef void*   DIR;
DIR             opendir(const char *);
struct dirent   *readdir(DIR*);
int32_t         closedir(DIR*);
#  endif
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#  include <direct.h>
#endif

int32_t     allocgen(CSOUND *, char *, int32_t (*)(FGDATA *, FUNC *));

/* separates the directories of a plugin search path */
#ifdef WIN32
#  define PLUGIN_PATH_SEP ';'
#else
#  define PLUGIN_PATH_SEP ':'
#endif

/* a path separator; Windows accepts '/' as well as '\' */
#ifdef WIN32
#  define IS_DIRSEP(c)  ((c) == DIRSEP || (c) == '/')
#else
#  define IS_DIRSEP(c)  ((c) == DIRSEP)
#endif

/* module interface function names */

static  const   char    *opcode_init_Name =   "csound_opcode_init";
static  const   char    *fgen_init_Name =     "csound_fgen_init";

static  const   char    *PreInitFunc_Name =   "csoundModuleCreate";
static  const   char    *InitFunc_Name =      "csoundModuleInit";
static  const   char    *DestFunc_Name =      "csoundModuleDestroy";
static  const   char    *ErrCodeToStr_Name =  "csoundModuleErrorCodeToString";

static  const   char    *InfoFunc_Name =      "csoundModuleInfo";

/* environment variable storing path to plugin libraries */
static  const   char    *plugindir_envvar =   "OPCODE7DIR";
static  const   char    *plugindir64_envvar = "OPCODE7DIR64";

/* default directory to load plugins from if environment variable is not set */
#if  !(defined (__wasi__))
#ifdef __HAIKU__
# ifndef USE_DOUBLE
static char haikudirs[] = "/boot/system/lib/csound7/plugins:"
  "/boot/home/config/lib/csound7/plugins:"
  "/boot/system/non-packaged/lib/csound7/plugins:"
  "/boot/home/config/non-packaged/lib/csound7/plugins";
# else
static char haikudirs[] = "/boot/system/lib/csound7/plugins64:"
  "/boot/home/config/lib/csound7/plugins64:"
  "/boot/system/non-packaged/lib/csound7/plugins64:"
  "/boot/home/config/non-packaged/lib/csound7/plugins64";
# endif
# define CS_DEFAULT_PLUGINDIR  haikudirs
#elif !(defined(USE_DEFAULT_OPCODEDIR) && (defined(LINUX) || defined(__MACH__)))
#  define ENABLE_OPCODEDIR_WARNINGS 1
#  ifdef CS_DEFAULT_PLUGINDIR
#    undef CS_DEFAULT_PLUGINDIR
#  endif
#  define CS_DEFAULT_PLUGINDIR      "."
#else
#  define ENABLE_OPCODEDIR_WARNINGS 0
#  ifndef CS_DEFAULT_PLUGINDIR
#    ifndef USE_DOUBLE
#      define CS_DEFAULT_PLUGINDIR  "/usr/local/lib/csound/plugins"
#    else
#      define CS_DEFAULT_PLUGINDIR  "/usr/local/lib/csound/plugins64"
#    endif
#  endif
#endif
#endif

#if (TARGET_OS_IPHONE != 0) && (TARGET_IPHONE_SIMULATOR != 0)
#  define ENABLE_OPCODEDIR_WARNINGS 0
#endif

typedef struct opcodeLibFunc_s {
  int64_t  (*opcode_init)(CSOUND *, OENTRY **);  /* list of opcode entries  */
  NGFENS  *(*fgen_init)(CSOUND *);        /* list of named GEN routines    */
  void    (*dummy)(void);                 /* unused                        */
} opcodeLibFunc_t;

typedef struct pluginLibFunc_s {
  int32_t         (*InitFunc)(CSOUND *);      /* initialisation routine        */
  int32_t         (*DestFunc)(CSOUND *);      /* destructor routine            */
  const char  *(*ErrCodeToStr)(int32_t);      /* convert error code to string  */
} pluginLibFunc_t;

typedef struct csoundModule_s {
  struct csoundModule_s *nxt;             /* pointer to next link in chain */
  void        *h;                         /* library handle                */
  int32_t         (*PreInitFunc)(CSOUND *);   /* pre-initialisation routine    */
  /*   (always NULL if opcode lib) */
  NGFENS  *(*fgen_init)(CSOUND *);        /* list of named GEN routines    */
  union {
    pluginLibFunc_t   p;                  /* generic plugin interface      */
    opcodeLibFunc_t   o;                  /* opcode library interface      */
  } fn;
  char        name[1];                    /* name of the module            */
} csoundModule_t;

static CS_NOINLINE void print_module_error(CSOUND *csound,
                                           const char *fmt, const char *fname,
                                           const csoundModule_t *m, int32_t err)
{
  csoundMessageS(csound, CSOUNDMSG_ERROR, Str(fmt), fname);
  if (m != NULL && m->fn.p.ErrCodeToStr != NULL)
    csoundMessageS(csound, CSOUNDMSG_ERROR,
                   ": %s\n", Str(m->fn.p.ErrCodeToStr(err)));
  else
    csoundMessageS(csound, CSOUNDMSG_ERROR, "\n");
}

static int32_t check_plugin_compatibility(CSOUND *csound, const char *fname, int32_t n)
{
  int32_t     sampleSize, minorVersion, majorVersion;
  int32_t floatABI = sizeof(cs_double) == sizeof(float) ? CSOUND_MODULE_USE_FLOAT : 0;

  /* Plugins that report no ABI metadata cannot be validated. */
  if (UNLIKELY(n == 0)) {
    csoundWarning(csound, Str("not loading '%s' (missing or incompatible "
                              "version information; rebuild the plugin with "
                              "this version of Csound)\n"), fname);
    return -1;
  }
  if (UNLIKELY((n & CSOUND_MODULE_USE_FLOAT) != floatABI)) {
    csoundWarning(csound, Str("not loading '%s' (incompatible cs_double precision; "
                              "rebuild the plugin with matching USE_FLOAT)\n"), fname);
    return -1;
  }
  sampleSize = n & (0xFF ^ CSOUND_MODULE_USE_FLOAT);
  if (UNLIKELY(sampleSize != 0 && sampleSize != (int32_t) sizeof(cs_float))) {
    csoundWarning(csound, Str("not loading '%s' (uses incompatible "
                              "floating point type)\n"), fname);
    return -1;
  }
  if (UNLIKELY(n & (~0xFF))) {
    minorVersion = (n & 0xFF00) >> 8;
    majorVersion = (n & (~0xFFFF)) >> 16;
    if (majorVersion != (int32_t) CS_VERSION ||
        (minorVersion > (int32_t) CS_SUBVER)) { /* NOTE **** REFACTOR *** */
      csoundWarning(csound, Str("not loading '%s' (incompatible "
                                "with this version of Csound (%d.%d/%d.%d)\n"),
                    fname, majorVersion,minorVersion,
                    CS_VERSION,CS_SUBVER);
      return -1;
    }
  }
  return 0;
}

static int32_t open_library(void **library, const char *libraryPath);
static int32_t close_library(void *library);
static void *get_library_symbol(void *library, const char *symbolName);

/**
 * Initialise a single module.
 * Return value is CSOUND_SUCCESS if there was no error.
 */
static CS_NOINLINE int32_t init_module(CSOUND *csound, csoundModule_t *m)
{
  int32_t     i;

  if (m->PreInitFunc != NULL) {
    /* fgens in generic module */
    if (m->fgen_init != NULL) {
      NGFENS  *names = m->fgen_init(csound);
      for (i = 0; names[i].name != NULL; i++)
        allocgen(csound, names[i].name, names[i].fn);
    }   
    if (m->fn.p.InitFunc != NULL) {
      i = m->fn.p.InitFunc(csound);
      if (UNLIKELY(i != 0)) {
        print_module_error(csound, Str("Error starting module '%s'"),
                           &(m->name[0]), m, i);
        return CSOUND_ERROR;
      }
    }

    
  }
  else {
    /* deal with fgens if there are any */
    if (m->fn.o.fgen_init != NULL) {
      NGFENS  *names = m->fn.o.fgen_init(csound);
      for (i = 0; names[i].name != NULL; i++)
        allocgen(csound, names[i].name, names[i].fn);
    }
    if (m->fn.o.opcode_init != NULL) {
      OENTRY  *opcodlst_n;
      int64_t    length;
      /* load opcodes */
      if (UNLIKELY((length = m->fn.o.opcode_init(csound, &opcodlst_n)) < 0L))
        return CSOUND_ERROR;
      else {
        length /= (long) sizeof(OENTRY);
        if (length) {
          if (UNLIKELY(csoundAppendOpcodes(csound, opcodlst_n,
                                           (int32_t) length) != 0))
            return CSOUND_ERROR;
        }
      }
    }
  }
  return CSOUND_SUCCESS;
}

#ifdef __wasi__
__attribute__((used))
void csoundWasiLoadPlugin(CSOUND *csound, void *preInitFunc, void *initFunc, void *destFunc, void *errCodeToStr) {
  csoundModule_t *module = csound->Calloc(csound, sizeof(csoundModule_t) + 1);
  module->h = (void*) NULL;

  // The javascript host must assert that this is provided
  module->PreInitFunc = (int32_t (*)(CSOUND *)) preInitFunc;
  if (initFunc) {
    module->fn.p.InitFunc = (int32_t (*)(CSOUND *)) initFunc;
  }
  if (destFunc) {
    module->fn.p.DestFunc = (int32_t (*)(CSOUND *)) destFunc;
  }
  if (errCodeToStr) {
    module->fn.p.ErrCodeToStr = (const char *(*)(int32_t)) errCodeToStr;
  }

  module->nxt = (csoundModule_t*) csound->csmodule_db;
  csound->csmodule_db = module;

  module->PreInitFunc(csound);
}

__attribute__((used))
void csoundWasiLoadOpcodeLibrary(CSOUND *csound, void *fgenInitFunc, void *opcodeInitFunc) {
  csoundModule_t *module = csound->Calloc(csound, sizeof(csoundModule_t) + 1);
  module->h = (void*) NULL;

  if (fgenInitFunc) {
    module->fn.o.fgen_init = (NGFENS *(*)(CSOUND *)) fgenInitFunc;
  }

  if (opcodeInitFunc) {
    module->fn.o.opcode_init = (int64_t (*)(CSOUND *, OENTRY **)) opcodeInitFunc;
  }

  module->nxt = (csoundModule_t*) csound->csmodule_db;
  csound->csmodule_db = module;
}

__attribute__((used))
int32_t csoundDestroyModules(CSOUND *csound) {
  csoundModule_t  *m;
  int32_t i;
  int32_t retval = CSOUND_SUCCESS;

  while (csound->csmodule_db != NULL) {
    m = (csoundModule_t*) csound->csmodule_db;
    /* call destructor functions */
    if (m->PreInitFunc != NULL && m->fn.p.DestFunc != NULL) {
      i = m->fn.p.DestFunc(csound);
      if (UNLIKELY(i != 0)) {
        print_module_error(csound, Str("Error de-initialising module '%s'"),
                           &(m->name[0]), m, i);
        retval = CSOUND_ERROR;
      }
    }
    csound->csmodule_db = (void*) m->nxt;
    /* free memory used by database */
    csound->Free(csound, (void*) m);
  }
  /* return with error code */
  return retval;
}

/* no library is loaded here, so none is kept */
void csoundCloseFailedModules(CSOUND *csound) {
  (void) csound;
}

__attribute__((used))
int32_t csoundInitModules(CSOUND *csound) {
  csoundModule_t  *m;
  int32_t i, retval = CSOUND_SUCCESS;
  for (m = (csoundModule_t*) csound->csmodule_db; m != NULL; m = m->nxt) {
    i = init_module(csound, m);
    if (UNLIKELY(i != CSOUND_SUCCESS && i < retval))
      retval = i;
  }
  /* return with error code */
  return retval;
}

#if defined(CSOUND_WASI_BROWSER) && defined(CSOUND_WASI_CLI)
#error "WASI browser and standalone host modes are mutually exclusive"
#elif defined(CSOUND_WASI_BROWSER)
// In browser-wasi, this function is provided by the JS host.
int32_t csoundLoadModulesHost(CSOUND *csound)
    __attribute__((used,
                   __import_module__("env"),
                   __import_name__("csoundLoadModules")));

int32_t csoundLoadModules(CSOUND *csound) {
  return csoundLoadModulesHost(csound);
}

int32_t csoundLoadExternalsHost(CSOUND *csound, const char *libraries)
    __attribute__((used,
                   __import_module__("env"),
                   __import_name__("csoundLoadExternals")));
#elif defined(CSOUND_WASI_CLI)
// Standalone WASI commands use only statically linked opcodes.
int32_t csoundLoadModules(CSOUND *csound) {
  (void) csound;
  return CSOUND_SUCCESS;
}
#else
#error "A WASI host mode must be selected by the build system"
#endif

static const char noRequestedPlugins[] = "";

__attribute__((used))
int32_t isRequestingPlugins(CSOUND *csound) {
  return csound != NULL && csound->dl_opcodes_oplibs != NULL &&
         csound->dl_opcodes_oplibs[0] != '\0';
}

__attribute__((used))
const char *getRequestedPlugins(CSOUND *csound) {
  if (!isRequestingPlugins(csound))
    return noRequestedPlugins;
  return csound->dl_opcodes_oplibs;
}

int32_t csoundLoadExternals(CSOUND *csound) {
#if defined(CSOUND_WASI_BROWSER)
  char *libraries = csound->dl_opcodes_oplibs;
  int32_t retval;

  /* Ensure default host modules are loaded even when no explicit
     --opcode-lib was requested (e.g. csoundStart() called directly). */
  if (UNLIKELY(csoundLoadDefaultModules(csound) != CSOUND_SUCCESS))
    return CSOUND_ERROR;

  if (libraries == NULL || libraries[0] == '\0')
    return CSOUND_SUCCESS;

  retval = csoundLoadExternalsHost(csound, libraries);
  if (LIKELY(retval == CSOUND_SUCCESS)) {
    csound->dl_opcodes_oplibs = NULL;
    csound->Free(csound, libraries);
  }
  return retval;
#else
  return isRequestingPlugins(csound) ? CSOUND_ERROR : CSOUND_SUCCESS;
#endif
}

int32_t csoundLoadAndInitModules(CSOUND *csound, const char *opdir) {
  return 0;
}

/* WASI stub: keep in sync with the native implementation below
   (after #else __wasi__). Any behavioural change must be applied to both. */
int32_t csoundLoadDefaultModules(CSOUND *csound) {
  int32_t err;
  if (LIKELY(csound->default_modules_loaded))
    return CSOUND_SUCCESS;
  /* Host MIDI callbacks belong to the embedding application. Do this before
     module initialization so a matching MIDI backend does not replace them. */
  if (csound->enableHostImplementedMIDIIO) {
    char *rtmidi = (char *)csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (rtmidi != NULL)
      strcpy(rtmidi, "hostbased");
    csoundSetConfigurationVariable(csound, "rtmidi", "hostbased");
  }
  /* Mark as in progress before loading. A plugin's csoundModuleInit() may
     call csoundGetModule(), which re-enters here; that call must read the
     module list instead of starting a second scan (csoundLoadModules()
     rejects the non-empty module database and would call Die). The flag is
     cleared on failure so that a later call can retry. */
  csound->default_modules_loaded = 1;
  err = csoundLoadModules(csound);
  if (csound->delayederrormessages &&
      csound->printerrormessagesflag == NULL) {
    csound->Warning(csound, "%s\n", csound->delayederrormessages);
    csound->Free(csound, csound->delayederrormessages);
    csound->delayederrormessages = NULL;
  }
  if (UNLIKELY(err != CSOUND_SUCCESS)) {
    csound->default_modules_loaded = 0;
    csound->Die(csound, Str("Failed during csoundLoadModules\n"));
  }
  /* The native copy starts the modules through init_default_modules(),
     which clears the flag when a module's csoundModuleInit() jumps. Not
     here: WASI's csoundLoadModules() does not refuse a module list that is
     not empty, as the native one does, so a later call would not fail, and
     with the realtime compile context a jump goes to that context's target,
     not to exitjmp. */
  if (csoundInitModules(csound) != 0) {
    csound->default_modules_loaded = 0;
    csound->LongJmp(csound, 1);
  }
  return CSOUND_SUCCESS;
}

#else /* __wasi__ */

/* Starts the modules of the plugin directory's scan. A module's
   csoundModuleInit() may stop Csound with a jump instead of returning an
   error: the scan has failed all the same, so the jump clears
   default_modules_loaded on its way, as an error does, and a later
   csoundLoadDefaultModules() fails too, where it took the half-started
   modules for loaded. */
static int32_t init_default_modules(CSOUND *csound)
{
  API_CALL          call;
  int32_t           n;

  api_call_catch(csound, &call);
  if (UNLIKELY((n = setjmp(call.target)) != 0)) {
    api_call_end(&call);
    csound->default_modules_loaded = 0;
    jump_on(csound, n);
  }
  n = csoundInitModules(csound);
  api_call_end(&call);
  return n;
}

/* The library of a module whose csoundModuleCreate() failed. What the module
   registered before it failed, such as a callback, may point into it and
   outlive a reset, so the instance keeps the library open, without starting
   or destroying the module, until csoundCloseFailedModules() closes it when
   the instance is destroyed. */
typedef struct failedModule_s {
  struct failedModule_s *nxt;
  void        *h;
} failedModule_t;

/* f is the node load_external() reserved before csoundModuleCreate() ran,
   with malloc, not csound->Malloc, which a reset frees: keeping the library
   cannot fail once the module may have registered something. */
static void keep_failed_module(CSOUND *csound, void *h, failedModule_t *f)
{
  failedModule_t  *kept;

  for (kept = (failedModule_t*) csound->failed_modules; kept != NULL;
       kept = kept->nxt) {
    if (kept->h == h) {
      /* kept open already: drop the reference this load added */
      close_library(h);
      free(f);
      return;
    }
  }
  f->h = h;
  f->nxt = (failedModule_t*) csound->failed_modules;
  csound->failed_modules = (void*) f;
}

void csoundCloseFailedModules(CSOUND *csound)
{
  while (csound->failed_modules != NULL) {
    failedModule_t  *f = (failedModule_t*) csound->failed_modules;
    csound->failed_modules = (void*) f->nxt;
    close_library(f->h);
    free(f);
  }
}

/* load a single plugin library, and run csoundModuleCreate() if present */
/* returns zero on success */
static CS_NOINLINE int32_t load_external(CSOUND *csound,
                                          const char *libraryPath)
{
  csoundModule_t  m;
  API_CALL        call;
  csoundModule_t  *mp;
  failedModule_t  *spare = NULL;
  char            *fname;
  void            *h, *p;
  int32_t             (*infoFunc)(void);
  int32_t             err;

  /* check for a valid name */
  if (UNLIKELY(libraryPath == NULL || libraryPath[0] == '\0'))
    return CSOUND_ERROR;
  /* remove leading directory components from name (Windows takes '/' too) */
  fname = (char*) libraryPath + (int32_t) strlen(libraryPath);
  for ( ; !IS_DIRSEP(fname[0]) && fname != (char*) libraryPath; fname--)
    ;
  if (IS_DIRSEP(fname[0]))
    fname++;
  if (UNLIKELY(fname[0] == '\0'))
    return CSOUND_ERROR;
  /* load library */
  err = open_library(&h, libraryPath);
  if (UNLIKELY(err)) {
    char ERRSTR[256];
#if (defined(LINUX) || defined(__HAIKU__))
    snprintf(ERRSTR, 256, Str("could not open library '%s' (%s)"),
             libraryPath, dlerror());
#else
    snprintf(ERRSTR, 256, Str("could not open library '%s' (%d)"),
             libraryPath, err);
#endif
    if (csound->delayederrormessages == NULL) {
      csound->delayederrormessages = csound->Malloc(csound, strlen(ERRSTR)+1);
      strcpy(csound->delayederrormessages, ERRSTR);
    }
    else {
      char *new =
        csound->ReAlloc(csound, csound->delayederrormessages,
                        strlen(csound->delayederrormessages)+strlen(ERRSTR)+11);
      if (UNLIKELY(new==NULL)) {
        csound->Free(csound, csound->delayederrormessages);
        return CSOUND_ERROR;
      }
      csound->delayederrormessages = new;
      strcat(csound->delayederrormessages, "\nWARNING: ");
      strcat(csound->delayederrormessages, ERRSTR);
    }
    return CSOUND_ERROR;
  }
  /* was this plugin already loaded ? */
  for (mp = (csoundModule_t*) csound->csmodule_db; mp != NULL; mp = mp->nxt) {
    if (UNLIKELY(mp->h == h)) {
      close_library(h);
      return CSOUND_SUCCESS;
    }
  }
  /* find out if it is a Csound plugin */
  memset(&m, 0, sizeof(csoundModule_t));
  m.h = h;
  m.PreInitFunc =
    (int32_t (*)(CSOUND *)) get_library_symbol(h, PreInitFunc_Name);
  if (m.PreInitFunc != NULL) {
    /* generic plugin library */
    m.fn.p.InitFunc =
      (int32_t (*)(CSOUND *)) get_library_symbol(h, InitFunc_Name);
    m.fn.p.DestFunc =
      (int32_t (*)(CSOUND *)) get_library_symbol(h, DestFunc_Name);
    m.fn.p.ErrCodeToStr =
      (const char *(*)(int32_t)) get_library_symbol(h, ErrCodeToStr_Name);
    /* fgens function in generic module */
    m.fgen_init =
      (NGFENS *(*)(CSOUND *)) get_library_symbol(h, fgen_init_Name);
  }
  else {
    /* opcode library */
    m.fn.o.opcode_init =
      (int64_t (*)(CSOUND *, OENTRY **))
      get_library_symbol(h, opcode_init_Name);
    m.fn.o.fgen_init =
      (NGFENS *(*)(CSOUND *)) get_library_symbol(h, fgen_init_Name);
    if (UNLIKELY(m.fn.o.opcode_init == NULL && m.fn.o.fgen_init == NULL)) {
      /* must have csound_opcode_init() or csound_fgen_init() */
      close_library(h);
      if (UNLIKELY(csound->oparms->msglevel & 0x400))
        csound->Warning(csound, Str("'%s' is not a Csound plugin library\n"),
                        libraryPath);
      return CSOUND_ERROR;
    }
  }
  /* check if the library is compatible with this version of Csound */
  infoFunc = (int32_t (*)(void)) get_library_symbol(h, InfoFunc_Name);
  if (UNLIKELY(check_plugin_compatibility(csound, fname,
                                          infoFunc != NULL ? infoFunc() : 0) != 0)) {
    close_library(h);
    return CSOUND_ERROR;
  }
  /* set up module info structure */
  /* (note: space for NUL character is already included in size of struct) */
  p = (void*) csound->Malloc(csound,
                             sizeof(csoundModule_t) + (size_t) strlen(fname));
  if (UNLIKELY(p == NULL)) {
    close_library(h);
    csound->ErrorMsg(csound,
                     "%s", Str("csoundLoadExternal(): memory allocation failure"));
    return CSOUND_MEMORY;
  }
  mp = (csoundModule_t*) p;
  memcpy(mp, &m, sizeof(csoundModule_t));
  strcpy(&(mp->name[0]), fname);
  /* call csoundModuleCreate() if available; a module whose
     csoundModuleCreate() fails is not linked, so it is neither started nor
     destroyed, and keep_failed_module() keeps its library open. */
  if (m.PreInitFunc != NULL) {
    /* the node that keeps the library if csoundModuleCreate() fails, taken
       before it runs, while closing the library is still safe */
    spare = (failedModule_t*) malloc(sizeof(failedModule_t));
    if (UNLIKELY(spare == NULL)) {
      csound->Free(csound, p);
      close_library(h);
      csound->ErrorMsg(csound, "%s",
                       Str("csoundLoadExternal(): memory allocation failure"));
      return CSOUND_MEMORY;
    }
    api_call_catch(csound, &call);
    if ((err = setjmp(call.target)) != 0) {
      api_call_end(&call);
      print_module_error(csound, Str("Error in pre-initialisation function "
                                     "of module '%s'"), fname, NULL, 0);
      csound->Free(csound, p);
      keep_failed_module(csound, h, spare);
      return (err == (CSOUND_EXITJMP_SUCCESS + CSOUND_MEMORY) ?
              CSOUND_MEMORY : CSOUND_INITIALIZATION);
    }
    err = m.PreInitFunc(csound);
    api_call_end(&call);
    if (UNLIKELY(err != 0)) {
      print_module_error(csound, Str("Error in pre-initialisation function "
                                     "of module '%s'"), fname, &m, err);
      csound->Free(csound, p);
      keep_failed_module(csound, h, spare);
      return CSOUND_INITIALIZATION;
    }
    free(spare);
  }
  /* link into database */
  mp->nxt = (csoundModule_t*) csound->csmodule_db;
  csound->csmodule_db = (void*) mp;
  /* plugin was loaded successfully */
  return CSOUND_SUCCESS;
}

static int32_t check_opcode_deny(CSOUND * csound, const char *fname)
{
  /* Check to see if the fname is on the do-not-load list */
  char buff[256];
  char *th;
  char *p, *deny;
  char *list = getenv("CS_OMIT_LIBS");
  if (list==NULL) return 0;
  strNcpy(buff, fname, 255);
  strrchr(buff, '.')[0] = '\0'; /* Remove .so etc */
  p = csoundStrdup(csound, list);
  deny = cs_strtok_r(p, ",", &th);
  while (deny) {
    if (strcmp(deny, buff)==0) {
      csound->Free(csound, p);
      return 1;
    }
    deny = cs_strtok_r(NULL, ",", &th);
  }
  csound->Free(csound, p);
  return 0;
}


static int32_t _dir_exists(char *path) {
  // returns 1 if path is a directory and it exists
  struct stat s;
  int32_t err = stat(path, &s);
  return (err == 0 && S_ISDIR(s.st_mode)) ? 1 : 0;
}


#if (defined(HAVE_DIRENT_H) && (TARGET_OS_IPHONE == 0))
/* Whether a directory entry names a native plugin library: not starting
   with '_', with the platform's library suffix in any case. */
static int32_t is_plugin_library_name(const char *fname)
{
#if defined(WIN32)
  static const char suffix[] = ".dll";
#elif defined(__MACH__)
  static const char suffix[] = ".dylib";
#else
  static const char suffix[] = ".so";
#endif
  size_t len = strlen(fname), n = sizeof(suffix) - 1, i;
  if (fname[0] == '_' || len <= n)
    return 0;
  fname += len - n;
  if (fname[0] != '.')
    return 0;
  for (i = 1; i < n; i++)
    if ((fname[i] | (char) 0x20) != suffix[i])
      return 0;
  return 1;
}

/* The default plugin search path, as csoundLoadModules() scans it */
typedef struct {
  char        *path;          /* directories, separated by PLUGIN_PATH_SEP */
  const char  *userStart;     /* in path: CS_USER_PLUGINDIR, if it was set */
  int32_t     configured;     /* the first part was set explicitly */
  int32_t     fromEnv;        /* it came from OPCODE7DIR or OPCODE7DIR64 */
  int32_t     dfltdir;        /* Haiku: it is the built-in default */
} pluginSearchPath_t;

/* OPCODE7DIR(64), or the built-in default, or the --opcodedir override,
   followed by the user plugin directory. The caller frees sp->path. */
static void plugin_search_path(CSOUND *csound, pluginSearchPath_t *sp)
{
  const char  *dname;
  char        *userplugindir;
  size_t      pos, size;
  const char  sep = PLUGIN_PATH_SEP;

  memset(sp, 0, sizeof(*sp));
  dname = csoundGetEnv(csound, (sizeof(cs_float) == sizeof(float) ?
                                plugindir_envvar : plugindir64_envvar));
  sp->fromEnv = sp->configured = (dname != NULL);
  if (dname == NULL) {
#ifdef CS_DEFAULT_PLUGINDIR
    dname = CS_DEFAULT_PLUGINDIR;
#  ifdef __HAIKU__
    sp->dfltdir = 1;
#  endif
#else
    dname = "";
#endif
  }
  /* opcodedir GLOBAL override **experimental** */
  if (csound->opcodedir != NULL) {
    dname = csound->opcodedir;
    sp->configured = 1;
    if (csound->oparms->odebug)
      csound->Message(csound, Str("OPCODEDIR overridden to %s \n"), dname);
  }
  pos = strlen(dname);
  userplugindir = getenv("CS_USER_PLUGINDIR");
  // The user set a search path for plugins via an env variable. Paths here
  // should be absolute and should not need variable expansion
  if (userplugindir != NULL) {
    size = pos + strlen(userplugindir) + 2;
    sp->path = csound->Malloc(csound, size);
    snprintf(sp->path, size, "%s%c%s", dname, sep, userplugindir);
    sp->userStart = sp->path + pos + 1;
    return;
  }
#ifdef CS_DEFAULT_USER_PLUGINDIR
  {
    enum { buflen = 1024 };
    char buf[buflen];
    // Use default user path
    // In this case, userplugindir is a relative path to a prefix wich needs
    // to be expanded
    userplugindir = CS_DEFAULT_USER_PLUGINDIR;
#if defined(LINUX) || defined(__MACH__)
    char *prefix = getenv("HOME");
#elif defined(WIN32)
    char *prefix = getenv("LOCALAPPDATA");
#endif
    // VL: need to check so we don't get a segfault with NULL strings
    size_t prefixlen = prefix ? strlen(prefix) : 0;
    size_t userplugindirlen = userplugindir ? strlen(userplugindir) : 0;

    if (userplugindirlen + prefixlen + 1 >= buflen) {
      csound->ErrorMsg(csound, "%s", Str("User plugin dir too long\n"));
    } else {
      snprintf(buf, buflen, "%s/%s", prefix, userplugindir);
      if (_dir_exists(buf)) {
        size = pos + strlen(buf) + 2;
        sp->path = csound->Malloc(csound, size);
        snprintf(sp->path, size, "%s%c%s", dname, sep, buf);
        return;
      }
    }
  }
#endif
  sp->path = csoundStrdup(csound, dname);
}
#endif  /* HAVE_DIRENT_H */

/**
 * Load plugin libraries for Csound instance 'csound', and call
 * pre-initialisation functions.
 * Return value is CSOUND_SUCCESS if there was no error, CSOUND_ERROR if
 * some modules could not be loaded or initialised, and CSOUND_MEMORY
 * if a memory allocation failure has occured.
 */
int32_t csoundLoadModules(CSOUND *csound)
{
#if (defined(HAVE_DIRENT_H) && (TARGET_OS_IPHONE == 0))
  DIR *dir;
  struct dirent *f;
  const char *dname, *fname, *end;
  enum { buflen = 1024 };
  char buf[buflen];
  int32_t n, len, err = CSOUND_SUCCESS;
  char *dname1;
  int32_t read_directory = 1;
  int32_t warn_bad_directory;
  pluginSearchPath_t sp;
  const char sep = PLUGIN_PATH_SEP;

  if (UNLIKELY(csound->csmodule_db != NULL))
    return CSOUND_ERROR;

  /* open plugin directory */
  plugin_search_path(csound, &sp);
#if ENABLE_OPCODEDIR_WARNINGS
  if (!sp.fromEnv)
    csound->opcodedirWasOK = 0;
#endif
  warn_bad_directory = sp.configured;
  dname = sp.path;

  if(UNLIKELY(csound->oparms->odebug))
    csound->Message(csound, Str("Plugins search path: %s\n"), dname);

  /* We now loop through the directory list */
  while(read_directory) {
    /* find separator */
    if((end = strchr(dname, sep)) != NULL) {
      /* Copy the directory without changing the source search path. */
      size_t dirlen = (size_t) (end - dname);
      dname1 = csound->Malloc(csound, dirlen + 1);
      memcpy(dname1, dname, dirlen);
      dname1[dirlen] = '\0';
      /* move to next directory name */
      dname = end + 1;
      if (dname == sp.userStart)
        warn_bad_directory = 1;

    } else {
      /* copy last directory name) */
      dname1 = csoundStrdup(csound, dname);
      read_directory = 0;
    }

    /* protect for the case where there is an
       extra separator at the end */
    if(*dname1 == '\0') {
      csound->Free(csound, dname1);
      continue;
    }
    dir = opendir(dname1);
    if (UNLIKELY(dir == (DIR*) NULL)) {
      if (!sp.dfltdir)
        /* Always report explicitly configured paths; fallback directories
           remain quiet unless verbose mode is enabled. */
        if (warn_bad_directory || csound->oparms->odebug)
          csound->Warning(csound, Str("Error opening plugin directory '%s': %s\n"),
                          dname1, strerror(errno));
      csound->Free(csound, dname1);
      continue;
    }
    if(UNLIKELY(csound->oparms->odebug))
      csound->Message(csound, Str("Opening plugin directory: %s\n"), dname1);
    /* scan all files in directory */
    while ((f = readdir(dir)) != NULL) {
      fname = &(f->d_name[0]);
      if (!is_plugin_library_name(fname))
        continue;
      len = (int32_t) strlen(fname);
      /* found a dynamic library, attempt to open it */
      if (UNLIKELY(((int32_t) strlen(dname1) + len + 2) > 1024)) {
        csound->Warning(csound, Str("path name too long, skipping '%s'\n"),
                        fname);
        continue;
      }
      if (UNLIKELY(check_opcode_deny(csound, fname))) {
        csoundWarning(csound, Str("Library %s omitted\n"), fname);
        continue;
      }

      snprintf(buf, 1024, "%s%c%s", dname1, DIRSEP, fname);

      if (UNLIKELY(csound->oparms->odebug)) {
        csoundMessage(csound, Str("Loading '%s'\n"), buf);
      }
      n = load_external(csound, buf);
      if (UNLIKELY(UNLIKELY(n == CSOUND_ERROR)))
        continue;               /* ignore non-plugin files */
      if (UNLIKELY(n < err))
        err = n;                /* record serious errors */
    }
    closedir(dir);
    csound->Free(csound, dname1);
  }
  csound->Free(csound, sp.path);
  return (err == CSOUND_INITIALIZATION ? CSOUND_ERROR : err);
#else
  return CSOUND_SUCCESS;
#endif  /* HAVE_DIRENT_H */
}

/**
 * Scan and initialise the default plugin directories once per instance.
 *
 * csoundLoadModules() used to run from csoundReset(), before command-line
 * options were decoded, which meant option-only runs such as
 * `csound --version` still inspected installed modules and diagnostics
 * could not observe the requested verbosity. Instead the scan is triggered
 * lazily, before the first operation that actually needs loaded modules.
 *
 * The default_modules_loaded flag is set while the scan/init runs (so that
 * re-entrant enumeration during csoundModuleInit() does not start a second
 * scan) and stays set on success, even if the directories were empty. It is
 * cleared if loading fails, a module's csoundModuleInit() jumping included:
 * a later call then scans again only if no module is left loaded, and
 * otherwise fails too, as csoundLoadModules() refuses a module list that is
 * not empty, until a reset. Callers ensure the default scan runs before any
 * explicit plugin load. Safe to call repeatedly.
 */
int32_t csoundLoadDefaultModules(CSOUND *csound) {
  int32_t err;
  if (LIKELY(csound->default_modules_loaded))
    return CSOUND_SUCCESS;
  /* Preserve host-installed MIDI callbacks while initializing modules. */
  if (csound->enableHostImplementedMIDIIO) {
    char *rtmidi = (char *)csoundQueryGlobalVariable(csound, "_RTMIDI");
    if (rtmidi != NULL)
      strcpy(rtmidi, "hostbased");
    csoundSetConfigurationVariable(csound, "rtmidi", "hostbased");
  }
  /* Mark as in progress before loading. A plugin's csoundModuleInit() may
     call csoundGetModule(), which re-enters here; that call must read the
     module list instead of starting a second scan (csoundLoadModules()
     rejects the non-empty module database and would call Die). The flag is
     cleared on failure, as the comment above says. */
  csound->default_modules_loaded = 1;
  err = csoundLoadModules(csound);
  if (csound->delayederrormessages &&
      csound->printerrormessagesflag == NULL) {
    csound->Warning(csound, "%s\n", csound->delayederrormessages);
    csound->Free(csound, csound->delayederrormessages);
    csound->delayederrormessages = NULL;
  }
  if (UNLIKELY(err != CSOUND_SUCCESS)) {
    csound->default_modules_loaded = 0;
    csound->Die(csound, Str("Failed during csoundLoadModules\n"));
  }
  if (init_default_modules(csound) != 0) {
    csound->default_modules_loaded = 0;
    csound->LongJmp(csound, 1);
  }
  return CSOUND_SUCCESS;
}


static int cmp_func(const void *p1, const void *p2)
{
  return (strcmp(*((const char**) p1), *((const char**) p2)));
}

static int32_t is_wasm_library_name(const char *path)
{
  static const char suffix[] = ".wasm";
  size_t path_length, suffix_length = sizeof(suffix) - 1;
  uint32_t i;
  if (path == NULL || (path_length = strlen(path)) < suffix_length)
    return 0;
  path += path_length - suffix_length;
  for (i = 0; i < suffix_length; i++) {
    char value = path[i];
    if (value >= 'A' && value <= 'Z')
      value = (char) (value - 'A' + 'a');
    if (value != suffix[i])
      return 0;
  }
  return 1;
}

static int32_t load_explicit_opcode_library(CSOUND *csound, const char *path)
{
#ifdef HAVE_WASMTIME
  int32_t result;
#endif
  if (!is_wasm_library_name(path))
    return load_external(csound, path);
#ifdef HAVE_WASMTIME
  result = csoundLoadWasmOpcodeLibrary(csound, path);
  if (result == CSOUND_SUCCESS || result == CSOUND_MEMORY)
    return result;
  return CSOUND_INITIALIZATION;
#else
  csound->ErrorMsg(csound,
                   "Cannot load Wasm opcode library '%s': this Csound build "
                   "has no Wasmtime support\n",
                   path);
  return CSOUND_INITIALIZATION;
#endif
}

int32_t csoundLoadExternals(CSOUND *csound)
{
  char    *s, **lst;
  int32_t     i, cnt, err;

  /* Make sure the default directories are scanned before any explicit
     --opcode-lib libraries are added. */
  if (UNLIKELY(csoundLoadDefaultModules(csound) != CSOUND_SUCCESS))
    return CSOUND_ERROR;

  s = csound->dl_opcodes_oplibs;
  if (UNLIKELY(s == NULL || s[0] == '\0'))
    return 0;

  csound->dl_opcodes_oplibs = NULL;
  csoundMessage(csound, "%s", Str("Loading command-line libraries:\n"));
  cnt = 1;
  i = 0;
  do {
    if (s[i] == ',')
      cnt++;
  } while (s[++i] != '\0');
  lst = (char**) csound->Malloc(csound, sizeof(char*) * cnt);
  i = cnt = 0;
  lst[cnt++] = s;
  do {
    if (s[i] == ',') {
      lst[cnt++] = &(s[i + 1]);
      s[i] = '\0';
    }
  } while (s[++i] != '\0');
  qsort((void*) lst, (size_t) cnt, sizeof(char*), cmp_func);
  i = 0;
  do {
    char  *fname = lst[i];
    if (fname[0] != '\0' && !(i && strcmp(fname, lst[i - 1]) == 0)) {
      err = load_explicit_opcode_library(csound, fname);
      if (UNLIKELY(err == CSOUND_INITIALIZATION || err == CSOUND_MEMORY))
        csoundDie(csound, Str(" *** error loading '%s'\n"), fname);
      else if (!err)
        csoundMessage(csound, Str("loaded %s\n"), fname);
      else
        csoundMessage(csound, Str("failed to load %s\n"), fname);
    }
  } while (++i < cnt);
  /* file list is no longer needed */
  csound->Free(csound, lst);
  csound->Free(csound, s);
  return 0;
}

/**
 * Call initialisation functions of all loaded modules that have a
 * csoundModuleInit symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and CSOUND_ERROR if
 * some modules could not be initialised.
 */
int32_t csoundInitModules(CSOUND *csound)
{
  csoundModule_t  *m;
  int32_t             i, retval = CSOUND_SUCCESS;
  /* call init functions */
  for (m = (csoundModule_t*) csound->csmodule_db; m != NULL; m = m->nxt) {
    i = init_module(csound, m);
    if (UNLIKELY(i != CSOUND_SUCCESS && i < retval))
      retval = i;
  }
#ifdef HAVE_WASMTIME
  i = csoundInitWasmOpcodeLibraries(csound);
  if (UNLIKELY(i != CSOUND_SUCCESS && i < retval))
    retval = i;
#endif
  /* return with error code */
  return retval;
}

/* load a plugin library and also initialise it */
/* called on deferred loading of opcode plugins */
static int32_t load_init_module(CSOUND *csound, const char *fname){
  API_CALL          call;
  volatile int32_t      err;
  const void        *loaded = csound->csmodule_db;

  err = load_external(csound, fname);
  if (UNLIKELY(err != 0))
    return err;
  /* a plugin already loaded links nothing: there is no new module to start */
  if (csound->csmodule_db == loaded)
    return CSOUND_SUCCESS;
  api_call_catch(csound, &call);
  if (UNLIKELY((err = setjmp(call.target)) != 0)) {
    api_call_end(&call);
    return (err == (CSOUND_EXITJMP_SUCCESS + CSOUND_MEMORY) ?
            CSOUND_MEMORY : CSOUND_INITIALIZATION);
  }
  /* csound->csmodule_db is the module load_external() has just linked */
  err = init_module(csound, (csoundModule_t*) csound->csmodule_db);
  api_call_end(&call);

  /* not CSOUND_ERROR, which the scan takes for a file that is no plugin */
  return (err == CSOUND_SUCCESS ? CSOUND_SUCCESS : CSOUND_INITIALIZATION);
}

int32_t csoundLoadAndInitModules(CSOUND *csound, const char *opdir){
#if (defined(HAVE_DIRENT_H) && (TARGET_OS_IPHONE == 0))
  DIR             *dir;
  struct dirent   *f;
  const char      *dname, *fname, *end;
  char            buf[1024];
  int32_t         n, len, err = CSOUND_SUCCESS;
  char            *dname1;
  int32_t         read_directory = 1;
  char sep =
#ifdef WIN32
    ';';
#else
  ':';
#endif
#ifdef __HAIKU__
  int32_t dfltdir = 0;
#endif
  /* Scan the default directories first so that loading an explicit
     --opcode-dir adds to, rather than replaces, the default set. The scan
     stops Csound with a jump when a plugin fails to load or start, and a
     host may call csoundLoadPlugins() first, with no frame for that jump:
     take it here as an error. */
  if (UNLIKELY(csoundCatchJump(csound, csoundLoadDefaultModules)
               != CSOUND_SUCCESS))
    return CSOUND_ERROR;
  /* open plugin directory */
  // EM'2021: This seems to be dead code since opdir will never be NULL and
  // the value of dname will be discarded, see "dname = opdir" later
  dname = csoundGetEnv(csound, (sizeof(cs_float) == sizeof(float) ?
                                plugindir_envvar : plugindir64_envvar));
  if (dname == NULL) {
#if ENABLE_OPCODEDIR_WARNINGS
    csound->opcodedirWasOK = 0;
#  ifdef USE_DOUBLE
    dname = csoundGetEnv(csound, plugindir_envvar);
    if (dname == NULL)
#  endif
#endif
#ifdef  CS_DEFAULT_PLUGINDIR
      dname = CS_DEFAULT_PLUGINDIR;
#ifdef __HAIKU__
    dfltdir = 1;
#endif
#else
    dname = "";
#endif
  }

  if (opdir != NULL) {
    dname = opdir;
  }

  /* We now loop through the directory list */
  while(read_directory) {
    /* find separator */
    if((end = strchr(dname, sep)) != NULL) {
      /* Copy the directory without changing the source search path. */
      size_t dirlen = (size_t) (end - dname);
      dname1 = csound->Malloc(csound, dirlen + 1);
      memcpy(dname1, dname, dirlen);
      dname1[dirlen] = '\0';
      /* move to next directory name */
      dname = end + 1;
    } else {
      /* copy last directory name) */
      dname1 = csoundStrdup(csound, dname);
      read_directory = 0;
    }

    /* protect for the case where there is an
       extra separator at the end */
    if(*dname1 == '\0') {
      csound->Free(csound, dname1);
      continue;
    }

    dir = opendir(dname1);
    if (UNLIKELY(dir == (DIR*) NULL)) {
#if defined(__HAIKU__)
      if(!dfltdir)
#endif
        csound->Warning(csound, Str("Error opening plugin directory '%s': %s\n"),
                        dname1, strerror(errno));
      csound->Free(csound, dname1);
      continue;
    }

    if(UNLIKELY(csound->oparms->odebug))
      csound->Message(csound, Str("Opening plugin directory: %s\n"), dname1);
    /* scan all files in directory */
    while ((f = readdir(dir)) != NULL) {
      fname = &(f->d_name[0]);
      if (!is_plugin_library_name(fname))
        continue;
      len = (int32_t) strlen(fname);
      /* found a dynamic library, attempt to open it */
      if (UNLIKELY(((int32_t) strlen(dname1) + len + 2) > 1024)) {
        csound->Warning(csound, Str("path name too long, skipping '%s'\n"),
                        fname);
        continue;
      }
      if (UNLIKELY(check_opcode_deny(csound, fname))) {
        csoundWarning(csound, Str("Library %s omitted\n"), fname);
        continue;
      }
      snprintf(buf, 1024, "%s%c%s", dname1, DIRSEP, fname);
      if (UNLIKELY(csound->oparms->odebug)) {
        csoundMessage(csound, Str("Loading '%s'\n"), buf);
      }
      n = load_init_module(csound, buf);
      if (UNLIKELY(UNLIKELY(n == CSOUND_ERROR)))
        continue;               /* ignore non-plugin files */
      if (UNLIKELY(n < err))
        err = n;                /* record serious errors */
    }
    closedir(dir);
    csound->Free(csound, dname1);
  }
  return (err == CSOUND_INITIALIZATION ? CSOUND_ERROR : err);
#else
  return CSOUND_SUCCESS;
#endif  /* HAVE_DIRENT_H */
}

/**
 * Call destructor functions of all loaded modules that have a
 * csoundModuleDestroy symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and
 * CSOUND_ERROR if some modules could not be de-initialised.
 */
extern int32_t sfont_ModuleDestroy(CSOUND *csound);
int32_t csoundDestroyModules(CSOUND *csound)
{
  csoundModule_t  *m;
  int32_t             i, retval;

  retval = CSOUND_SUCCESS;
  while (csound->csmodule_db != NULL) {

    m = (csoundModule_t*) csound->csmodule_db;
    /* call destructor functions */
    if (m->PreInitFunc != NULL && m->fn.p.DestFunc != NULL) {
      i = m->fn.p.DestFunc(csound);
      if (UNLIKELY(i != 0)) {
        print_module_error(csound, Str("Error de-initialising module '%s'"),
                           &(m->name[0]), m, i);
        retval = CSOUND_ERROR;
      }
    }
    /* unload library */
    close_library(m->h);
    csound->csmodule_db = (void*) m->nxt;
    /* free memory used by database */
    csound->Free(csound, (void*) m);

  }
#ifndef BUILD_PLUGINS
  sfont_ModuleDestroy(csound);
#endif
#ifdef HAVE_WASMTIME
  i = csoundDestroyWasmOpcodeLibraries(csound);
  if (UNLIKELY(i != CSOUND_SUCCESS && i < retval))
    retval = i;
#endif
  /* return with error code */
  return retval;
}

#endif /* __wasi__ */

/* --list-plugins: list the plugin libraries Csound would load, with the
   metadata each file gives, without loading any of them. */

#if !defined(__wasi__) && defined(HAVE_DIRENT_H) && (TARGET_OS_IPHONE == 0)
#  define CS_LIST_PLUGIN_DIRS 1
#endif

typedef struct {
  char        *path;
  const char  *name;      /* file name, in path */
  int32_t     status;     /* a CSOUND_PLUGIN_METADATA_STATUS */
  char        version[CSOUND_MODULE_VERSION_MAX];
} pluginListEntry_t;

typedef struct {
  pluginListEntry_t *entries;
  int32_t           cnt, size;
} pluginList_t;

/* Add the file at 'path', once, with the metadata read from it. */
static void plugin_list_add(CSOUND *csound, pluginList_t *lst,
                            const char *path)
{
  pluginListEntry_t *e;
  PLUGIN_METADATA   rec;
  const char        *name;
  int32_t           i;
  for (i = 0; i < lst->cnt; i++)
    if (strcmp(lst->entries[i].path, path) == 0)
      return;
  if (lst->cnt == lst->size) {
    lst->size = lst->size ? lst->size * 2 : 16;
    lst->entries = (pluginListEntry_t *)
        csound->ReAlloc(csound, lst->entries,
                        sizeof(pluginListEntry_t) * (size_t) lst->size);
  }
  e = &lst->entries[lst->cnt++];
  e->path = csoundStrdup(csound, path);
  for (name = e->path + strlen(e->path);
       name != e->path && !IS_DIRSEP(name[-1]); name--)
    ;
  e->name = name;
  e->version[0] = '\0';
  e->status = pluginMetadataRead(path, &rec);
  if (e->status == CSOUND_PLUGIN_METADATA_OK) {
    memcpy(e->version, rec.payload, rec.versionLength + 1);
    pluginMetadataFree(&rec);
  }
}

#ifdef CS_LIST_PLUGIN_DIRS
/* Add the plugin libraries in each directory of 'dirs', as the loader
   picks them. */
static void plugin_list_add_dirs(CSOUND *csound, pluginList_t *lst,
                                 const char *dirs)
{
  const char  *end;
  char        *dname;
  DIR         *dir;
  struct dirent *f;
  for (;;) {
    size_t len;
    end = strchr(dirs, PLUGIN_PATH_SEP);
    len = end != NULL ? (size_t) (end - dirs) : strlen(dirs);
    if (len > 0) {
      dname = csound->Malloc(csound, len + 1);
      memcpy(dname, dirs, len);
      dname[len] = '\0';
      if ((dir = opendir(dname)) != NULL) {
        while ((f = readdir(dir)) != NULL) {
          const char *fname = &(f->d_name[0]);
          size_t size;
          char *path;
          if (!is_plugin_library_name(fname) ||
              check_opcode_deny(csound, fname))
            continue;
          size = len + strlen(fname) + 2;
          path = csound->Malloc(csound, size);
          snprintf(path, size, "%s%c%s", dname, DIRSEP, fname);
          plugin_list_add(csound, lst, path);
          csound->Free(csound, path);
        }
        closedir(dir);
      }
      csound->Free(csound, dname);
    }
    if (end == NULL)
      break;
    dirs = end + 1;
  }
}
#endif

static int cmp_plugin_entry(const void *a, const void *b)
{
  const pluginListEntry_t *x = (const pluginListEntry_t *) a;
  const pluginListEntry_t *y = (const pluginListEntry_t *) b;
  int r = strcmp(x->name, y->name);
  return r != 0 ? r : strcmp(x->path, y->path);
}

static const char *plugin_status_text(CSOUND *csound,
                                      const pluginListEntry_t *e)
{
  switch (e->status) {
  case CSOUND_PLUGIN_METADATA_OK:         return e->version;
  case CSOUND_PLUGIN_METADATA_NONE:       return Str("(no-meta)");
  case CSOUND_PLUGIN_METADATA_INVALID:    return Str("(invalid)");
  case CSOUND_PLUGIN_METADATA_AMBIGUOUS:  return Str("(ambiguous)");
  default:                                return Str("(unreadable)");
  }
}

/* Print the plugin libraries in the default plugin directories, in the
   --opcode-dir directories and the --opcode-lib files, sorted by file name,
   with the version each file gives and its path. No library is loaded:
   the listing also shows plugins that would fail to load. */
void list_plugins(CSOUND *csound)
{
  pluginList_t  lst = { NULL, 0, 0 };
  int           nameWidth = 0, versionWidth = 0, len;
  int32_t       i;
#ifdef CS_LIST_PLUGIN_DIRS
  pluginSearchPath_t sp;
  plugin_search_path(csound, &sp);
  plugin_list_add_dirs(csound, &lst, sp.path);
  csound->Free(csound, sp.path);
  if (csound->list_plugin_dirs != NULL)
    plugin_list_add_dirs(csound, &lst, csound->list_plugin_dirs);
#endif
  if (csound->dl_opcodes_oplibs != NULL) {
    char *libs = csoundStrdup(csound, csound->dl_opcodes_oplibs), *p, *th;
    for (p = cs_strtok_r(libs, ",", &th); p != NULL;
         p = cs_strtok_r(NULL, ",", &th))
      if (p[0] != '\0')
        plugin_list_add(csound, &lst, p);
    csound->Free(csound, libs);
  }
  if (lst.cnt > 1)
    qsort((void*) lst.entries, (size_t) lst.cnt, sizeof(pluginListEntry_t),
          cmp_plugin_entry);
  for (i = 0; i < lst.cnt; i++) {
    if ((len = (int) strlen(lst.entries[i].name)) > nameWidth)
      nameWidth = len;
    len = (int) strlen(plugin_status_text(csound, &lst.entries[i]));
    if (len > versionWidth)
      versionWidth = len;
  }
  for (i = 0; i < lst.cnt; i++) {
    pluginListEntry_t *e = &lst.entries[i];
    csound->Message(csound, "%-*s  %-*s  %s\n",
                    nameWidth, e->name,
                    versionWidth, plugin_status_text(csound, e), e->path);
    csound->Free(csound, e->path);
  }
  csound->Message(csound, "\n");
  csound->Message(csound, Str("%d plugins\n\n"), lst.cnt);
  csound->Free(csound, lst.entries);
}

/* --opcode-dir with --list-plugins: keep the directory, to list it or to
   load it later, instead of loading it now. */
void csoundRecordPluginDir(CSOUND *csound, const char *dir)
{
  char *dirs = csound->list_plugin_dirs;
  size_t size = strlen(dir) + 1;
  if (dir[0] == '\0')
    return;
  if (dirs == NULL) {
    csound->list_plugin_dirs = csoundStrdup(csound, dir);
    return;
  }
  size += strlen(dirs) + 1;
  dirs = csound->ReAlloc(csound, dirs, size);
  snprintf(dirs + strlen(dirs), size - strlen(dirs), "%c%s",
           PLUGIN_PATH_SEP, dir);
  csound->list_plugin_dirs = dirs;
}

/* Load the directories csoundRecordPluginDir() kept, as --opcode-dir would
   have done. */
int32_t csoundLoadRecordedPluginDirs(CSOUND *csound)
{
  char *dirs = csound->list_plugin_dirs;
  int32_t err;
  if (dirs == NULL)
    return CSOUND_SUCCESS;
  csound->list_plugin_dirs = NULL;
  err = csoundLoadPlugins(csound, dirs);
  csound->Free(csound, dirs);
  return err;
}

int32_t csoundLoadRequestedPlugins(CSOUND *csound) {
  csoundModule_t *oldHead;
  csoundModule_t *module;
  int32_t err;
  int32_t retval;

  if (csound == NULL)
    return CSOUND_SUCCESS;

  /* Ensure default plugin directories are available even when no explicit
     --opcode-lib was requested (e.g. csoundCompileOrc() before csoundStart()). */
  if (UNLIKELY(csoundLoadDefaultModules(csound) != CSOUND_SUCCESS))
    return CSOUND_ERROR;

  if (csound->dl_opcodes_oplibs == NULL || csound->dl_opcodes_oplibs[0] == '\0')
    return CSOUND_SUCCESS;

  oldHead = (csoundModule_t *)csound->csmodule_db;
  retval = csoundLoadExternals(csound);

  if (csound->modules_loaded == 0) {
    err = csoundInitModules(csound);
    if (UNLIKELY(err != CSOUND_SUCCESS &&
                 (retval == CSOUND_SUCCESS || err < retval)))
      retval = err;
    if (LIKELY(err == CSOUND_SUCCESS))
      csound->modules_loaded = 1;
    return retval;
  }

  for (module = (csoundModule_t *)csound->csmodule_db;
       module != oldHead;
       module = module->nxt) {
    if (UNLIKELY(module == NULL)) {
      if (retval == CSOUND_SUCCESS || CSOUND_ERROR < retval)
        retval = CSOUND_ERROR;
      return retval;
    }
    err = init_module(csound, module);
    if (UNLIKELY(err != CSOUND_SUCCESS &&
                 (retval == CSOUND_SUCCESS || err < retval)))
      retval = err;
  }
#ifdef HAVE_WASMTIME
  err = csoundInitWasmOpcodeLibraries(csound);
  if (UNLIKELY(err != CSOUND_SUCCESS &&
               (retval == CSOUND_SUCCESS || err < retval)))
    retval = err;
#endif
  return retval;
}

int32_t csoundCatchJump(CSOUND *csound, int32_t (*func)(CSOUND *))
{
  API_CALL          call;
  int32_t           err;

#ifdef CSOUND_WASI_REALTIME
  /* A compiler worker's jump goes to its compile context, or ends the
     process: exitjmp is the render thread's, and is left alone. */
  if (csoundWasiIsWorkerThread())
    return func(csound);
#endif
  /* A note's init pass must keep its current opcode if this call fails. */
  if (note_init_runs(csound))
    api_call_begin(csound, &call);
  else
    api_call_catch(csound, &call);
  if (setjmp(call.target) != 0)
    err = CSOUND_ERROR;
  else
    err = func(csound);
  api_call_end(&call);
  return err;
}

/* ------------------------------------------------------------------------ */
#if defined(WIN32)
int32_t open_library(void **library, const char *libraryPath)
{
  *library = (void*) LoadLibrary(libraryPath);
  return (*library != NULL ? 0 : -1);
}

int32_t close_library(void *library)
{
  return (int32_t) (FreeLibrary((HMODULE) library) == FALSE ? -1 : 0);
}

void *get_library_symbol(void *library, const char *procedureName)
{
  return (void*) GetProcAddress((HMODULE) library, procedureName);
}

/* dyld code */
#elif !(defined(__wasi__)) && (defined(LINUX) || defined(BSD) \
                               || defined(NEW_MACH_CODE) || defined(__HAIKU__))
int32_t open_library(void **library, const char *libraryPath){
  int32_t flg = RTLD_NOW;
  if (libraryPath != NULL) {
    int32_t len = (int32_t) strlen(libraryPath);
    /* ugly hack to fix importing modules in Python opcodes */
    if (len >= 9 && strcmp(&(libraryPath[len - 9]), "/libpy.so") == 0)
      flg |= RTLD_GLOBAL;
    if (len >= 12 && strcmp(&(libraryPath[len - 12]), "/libpy.dylib") == 0)
      flg |= RTLD_GLOBAL;
  }
  *library = (void*) dlopen(libraryPath, flg);
  return (*library != NULL ? 0 : -1);
}

int32_t close_library(void *library){
  return (int32_t) dlclose(library);
}

void *get_library_symbol(void *library, const char *procedureName){
  return (void*) dlsym(library, procedureName);
}

#else /* case for platforms without shared libraries -- added 062404, akozar */
int32_t open_library(void **library, const char *libraryPath){
  *library = NULL;
  return -1;
}

int32_t close_library(void *library){
  return 0;
}

void *get_library_symbol(void *library, const char *procedureName){
  return NULL;
}
#endif

#if ENABLE_OPCODEDIR_WARNINGS
static const char *opcodedirWarnMsg[] = {
  "################################################################",
#ifndef USE_DOUBLE
  "#               WARNING: OPCODE7DIR IS NOT SET !               #",
#else
  "#              WARNING: OPCODE7DIR64 IS NOT SET !              #",
#endif
  "# Csound requires this environment variable to be set to find  #",
  "# its plugin libraries. If it is not set, you may experience   #",
  "# missing opcodes, audio/MIDI drivers, or utilities.           #",
  "################################################################",
  NULL
};
#endif

void print_opcodedir_warning(CSOUND *p)
{
#if ENABLE_OPCODEDIR_WARNINGS
  if (!p->opcodedirWasOK) {
    const char  **sp;
    for (sp = &(opcodedirWarnMsg[0]); *sp != NULL; sp++)
      p->MessageS(p, CSOUNDMSG_WARNING, "        %s\n", Str(*sp));
  }
#else
  (void) p;
#endif
}

#ifndef BUILD_PLUGINS
/**
   Builtin linkage for C opcodes - Instructions:
   - use csoundCore.h instead of csdl.h.
   - name the OENTRY array <name>, where <name> is any arbitrary name.
   - add the macro LINKAGE_BUILTIN(<name>) to the end of the file.
   - append the init function prototype below
   extern int64_t <name>_init(CSOUND *, void *);
   - append the init function name <name>_init to the
   staticmodules[] array initialisation.
   - insert source code to libcsound_SRCS in../CMakeLists.txt
*/
typedef int32_t (*INITFN)(CSOUND *, void *);
 int32_t babo_localops_init(CSOUND *, void *);
 int32_t bilbar_localops_init(CSOUND *, void *);
 int32_t compress_localops_init(CSOUND *, void *);
 int32_t pvsbuffer_localops_init(CSOUND *, void *);
 int32_t vosim_localops_init(CSOUND *, void *);
 int32_t eqfil_localops_init(CSOUND *, void *);
 int32_t modal4_localops_init(CSOUND *, void *);
 int32_t scoreline_localops_init(CSOUND *, void *);
 int32_t physmod_localops_init(CSOUND *, void *);
 int32_t modmatrix_localops_init(CSOUND *, void *);
 int32_t spectra_localops_init(CSOUND *, void *);
 int32_t ambicode1_localops_init(CSOUND *, void *);
 int32_t grain4_localops_init(CSOUND *, void *);
 int32_t hrtferX_localops_init(CSOUND *, void *);
 int32_t loscilx_localops_init(CSOUND *, void *);
 int32_t pan2_localops_init(CSOUND *, void *);
 int32_t arrayvars_localops_init(CSOUND *, void *);
 int32_t phisem_localops_init(CSOUND *, void *);
 int32_t pvoc_localops_init(CSOUND *, void *);
 int32_t hrtfopcodes_localops_init(CSOUND *, void *);
 int32_t hrtfreverb_localops_init(CSOUND *, void *);
 int32_t hrtfearly_localops_init(CSOUND *, void *);
 int32_t minmax_localops_init(CSOUND *, void *);
 int32_t gendy_localops_init(CSOUND *, void *);
 int32_t vbap_localops_init(CSOUND *, void *);
 int32_t vaops_localops_init(CSOUND *, void*);
 int32_t ugakbari_localops_init(CSOUND *, void *);
 int32_t harmon_localops_init(CSOUND *, void *);
 int32_t pitchtrack_localops_init(CSOUND *, void *);
 int32_t squinewave_localops_init(CSOUND *, void *);
 int32_t partikkel_localops_init(CSOUND *, void *);
 int32_t shape_localops_init(CSOUND *, void *);
 int32_t tabaudio_localops_init(CSOUND *, void *);
 int32_t tabsum_localops_init(CSOUND *, void *);
 int32_t crossfm_localops_init(CSOUND *, void *);
 int32_t pvlock_localops_init(CSOUND *, void *);
 int32_t fareyseq_localops_init(CSOUND *, void *);
 int32_t cpumeter_localops_init(CSOUND *, void *);
 int32_t scnoise_localops_init(CSOUND *, void *);
 int32_t mp3in_localops_init(CSOUND *, void *);
 int32_t bformdec2_localops_init(CSOUND *, void *);
 int32_t afilts_localops_init(CSOUND *, void *);
 int32_t pinker_localops_init(CSOUND *, void *);
 int32_t paulstretch_localops_init(CSOUND *, void *);
 int32_t wpfilters_localops_init(CSOUND *, void *);
 int32_t zak_localops_init(CSOUND *, void *);
 int32_t lufs_localops_init(CSOUND *, void *);
 int32_t sterrain_localops_init(CSOUND *, void *);
 int32_t date_localops_init(CSOUND *, void *);
 int32_t system_localops_init(CSOUND *, void *);
 int32_t liveconv_localops_init(CSOUND *, void *);
 int32_t gamma_localops_init(CSOUND *, void *);
 int32_t framebuffer_localops_init(CSOUND *, void *);
 int32_t cell_localops_init(CSOUND *, void *);
 int32_t exciter_localops_init(CSOUND *, void *);
 int32_t buchla_localops_init(CSOUND *, void *);
 int32_t select_localops_init(CSOUND *, void *);
 int32_t counter_localops_init(CSOUND *, void *);
 int32_t platerev_localops_init(CSOUND *, void *);
 int32_t sequencer_localops_init(CSOUND *, void *);
 int32_t pvsgendy_localops_init(CSOUND *, void *);
 int32_t scugens_localops_init(CSOUND *, void *);
 int32_t emugens_localops_init(CSOUND *, void *);
 int32_t control_localops_init(CSOUND *, void *);
 int32_t urandom_localops_init(CSOUND *, void *);

#ifdef HAVE_SOCKETS
 int32_t socksend_localops_init(CSOUND *, void *);
 int32_t sockrecv_localops_init(CSOUND *, void *);
#endif

#ifndef NO_SERIAL_OPCODES
 int32_t serial_localops_init(CSOUND *, void *);
#endif

typedef int32_t (*INITFN2)(CSOUND *);

 int32_t stdopc_ModuleInit(CSOUND *csound);
 int32_t pvsopc_ModuleInit(CSOUND *csound);
 int32_t sfont_ModuleInit(CSOUND *csound);
 int32_t sfont_ModuleCreate(CSOUND *csound);
 int32_t newgabopc_ModuleInit(CSOUND *csound);
 int32_t csoundModuleInit_signalflowgraph(CSOUND *csound);
 int32_t arrayops_init_modules(CSOUND *csound);
 int32_t pvsops_init_modules(CSOUND *csound);
 int32_t trigEnv_init_modules(CSOUND *csound);
 #ifndef __wasi__
 int32_t csoundModuleInit_ampmidid(CSOUND *csound);
 int32_t csoundModuleCreate_mixer(CSOUND *csound);
 int32_t csoundModuleInit_mixer(CSOUND *csound);
 int32_t csoundModuleInit_doppler(CSOUND *csound);
 int32_t lfsr_init_modules(CSOUND *csound);
 #endif
 #ifndef BARE_METAL
 extern int32_t csoundModuleInit_ftsamplebank(CSOUND *csound);
 #endif
 int32_t csoundModuleInit_fractalnoise(CSOUND *csound);
 int32_t scansyn_init_(CSOUND *csound);
 int32_t scansynx_init_(CSOUND *csound);
#ifdef CSOUND_STATIC_UTILITIES
 int32_t csoundInitUtilities(CSOUND *csound);
#elif defined(CSOUND_WASI_CLI)
 int32_t mkir_init_(CSOUND *csound);
#endif

/**
   Builtin linkage for C fgens - Instructions:
   - use csoundCore.h instead of csdl.h.
   - name the NGFENS array <name>, where <name> is any arbitrary name.
   - add the macro FLINKAGE_BUILTIN(<name>) to the end of the file.
   - append the init function prototype below
   NGFENS* <name>_init(CSOUND *);
   - append the init function name <name>_init to the
   ftgenab[] array initialisation.
   - insert source code to libcsound_SRCS in../CMakeLists.txt
*/
typedef NGFENS* (*FGINITFN)(CSOUND *);

NGFENS *quadbezier_fgens_init(CSOUND *);
NGFENS *ftest_fgens_init(CSOUND *);
NGFENS *farey_fgens_init(CSOUND *);
NGFENS *padsyn_fgen_init(CSOUND *);
NGFENS *mp3in_fgen_init(CSOUND *);
#ifdef CSOUND_WASI_CLI
NGFENS *mkir_fgen_init(CSOUND *);
#endif

CS_NOINLINE int32_t csoundInitStaticModules(CSOUND *csound)
{
  int32_t     i;
  int32_t length;
  OENTRY  *opcodlst_n;
  const INITFN staticmodules[] = {
#if defined(LINUX) || defined(__MACH__)
    cpumeter_localops_init,
#endif
    counter_localops_init,
#if !(defined(__wasi__))
    system_localops_init,
#ifndef NO_SERIAL_OPCODES
    serial_localops_init,
#endif
#ifdef HAVE_SOCKETS
    sockrecv_localops_init,
    socksend_localops_init,
#endif
#endif  // !wasi
#if defined(LINUX) || defined(__MACH__)
    control_localops_init, urandom_localops_init,
#endif
    scnoise_localops_init, afilts_localops_init,
    mp3in_localops_init, hrtferX_localops_init,
    hrtfearly_localops_init, hrtfreverb_localops_init,
    bformdec2_localops_init, babo_localops_init,
    bilbar_localops_init, vosim_localops_init,
    compress_localops_init,  pinker_localops_init,
    squinewave_localops_init,  eqfil_localops_init,
    hrtfopcodes_localops_init, lufs_localops_init,
    sterrain_localops_init,date_localops_init,
    liveconv_localops_init, gamma_localops_init,
    wpfilters_localops_init, gendy_localops_init,
    phisem_localops_init, physmod_localops_init,
    framebuffer_localops_init, cell_localops_init,
    exciter_localops_init, buchla_localops_init,
    select_localops_init, platerev_localops_init,
    sequencer_localops_init,grain4_localops_init,
    loscilx_localops_init, pan2_localops_init,
    minmax_localops_init, vaops_localops_init,
    ugakbari_localops_init, harmon_localops_init,
    pitchtrack_localops_init, partikkel_localops_init,
    shape_localops_init, tabsum_localops_init,
    crossfm_localops_init, pvlock_localops_init,
    fareyseq_localops_init,  paulstretch_localops_init,
    tabaudio_localops_init,  scoreline_localops_init,
    modmatrix_localops_init, ambicode1_localops_init,
    arrayvars_localops_init, zak_localops_init,
    scugens_localops_init, emugens_localops_init,
    pvoc_localops_init, spectra_localops_init,
    vbap_localops_init,
    NULL };

  const INITFN2 staticmodules2[] = {
    stdopc_ModuleInit,
    newgabopc_ModuleInit,
    pvsopc_ModuleInit,
    sfont_ModuleCreate,
    sfont_ModuleInit,
    csoundModuleInit_signalflowgraph,
    arrayops_init_modules,
    pvsops_init_modules,
    trigEnv_init_modules,
#if !defined(__wasi__)
    csoundModuleInit_ampmidid,
    csoundModuleCreate_mixer,
    csoundModuleInit_mixer,
    csoundModuleInit_doppler,
    lfsr_init_modules,
#endif
    csoundModuleInit_fractalnoise,
#if !defined(BARE_METAL)
    csoundModuleInit_ftsamplebank,
#endif
    scansyn_init_,
    scansynx_init_,
#ifdef CSOUND_STATIC_UTILITIES
    csoundInitUtilities,
#elif defined(CSOUND_WASI_CLI)
    mkir_init_,
#endif
    NULL
  };

  const FGINITFN fgentab[] = {
    ftest_fgens_init, farey_fgens_init, quadbezier_fgens_init,
    padsyn_fgen_init, mp3in_fgen_init,
#ifdef CSOUND_WASI_CLI
    mkir_fgen_init,
#endif
    NULL };


  for (i=0; staticmodules[i]!=NULL; i++) {
    length = (staticmodules[i])(csound, &opcodlst_n);

    if (UNLIKELY(length <= 0L)) return CSOUND_ERROR;
    length /= (int32_t) sizeof(OENTRY);
    if (length) {
      if (UNLIKELY(csoundAppendOpcodes(csound, opcodlst_n, length) != 0))
        return CSOUND_ERROR;
    }
  }

  for (i=0; staticmodules2[i]!=NULL; i++) {
    if(UNLIKELY(staticmodules2[i](csound))) return CSOUND_ERROR;
  }

  /* modules were initialised successfully */
  /* Now fgens */
  for (i = 0; fgentab[i]!=NULL; i++) {
    int32_t j;
    NGFENS  *names = (fgentab[i])(csound);
    for (j = 0; names[j].name != NULL; j++)
      allocgen(csound, names[j].name, names[j].fn);
  }
  return CSOUND_SUCCESS;
}
#endif
