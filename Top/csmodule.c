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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/******************************************************************************
 * NEW PLUGIN INTERFACE                                                       *
 * ====================                                                       *
 *                                                                            *
 * Plugin libraries are loaded from the directory defined by the environment  *
 * variable OPCODE6DIR (or the current directory if OPCODE6DIR is unset) by   *
 * csoundPreCompile() while initialising a Csound instance, and are unloaded  *
 * at the end of performance by csoundReset().                                *
 * A library may export any of the following five interface functions,        *
 * however, the presence of csoundModuleCreate() is required for identifying  *
 * the file as a Csound plugin module.                                        *
 *                                                                            *
 * int csoundModuleCreate(CSOUND *csound)       (required)                    *
 * --------------------------------------                                     *
 *                                                                            *
 * Pre-initialisation function, called by csoundPreCompile().                 *
 *                                                                            *
 * int csoundModuleInit(CSOUND *csound)         (optional)                    *
 * ------------------------------------                                       *
 *                                                                            *
 * Called by Csound instances before orchestra translation. One possible use  *
 * of csoundModuleInit() is adding new opcodes with csoundAppendOpcode().     *
 *                                                                            *
 * int csoundModuleDestroy(CSOUND *csound)      (optional)                    *
 * ---------------------------------------                                    *
 *                                                                            *
 * Destructor function for Csound instance 'csound', called at the end of     *
 * performance, after closing audio output.                                   *
 *                                                                            *
 * const char *csoundModuleErrorCodeToString(int errcode)   (optional)        *
 * ------------------------------------------------------                     *
 *                                                                            *
 * Converts error codes returned by any of the initialisation or destructor   *
 * functions to a string message.                                             *
 *                                                                            *
 * int csoundModuleInfo(void)                   (optional)                    *
 * --------------------------                                                 *
 *                                                                            *
 * Returns information that can be used to determine if the plugin was built  *
 * for a compatible version of libcsound. The return value may be the sum of  *
 * any of the following two values:                                           *
 *                                                                            *
 *   ((CS_APIVERSION << 16) + (CS_APISUBVER << 8))      API version           *
 *   (int) sizeof(MYFLT)                                MYFLT type            *
 *                                                                            *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>

#include "csoundCore.h"
#include "csmodule.h"

#if defined(__MACH__)
#include <TargetConditionals.h>
#if (TARGET_OS_IPHONE == 0) && (TARGET_IPHONE_SIMULATOR == 0)
#if defined(MAC_OS_X_VERSION_10_6) && \
    (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_6)
#define NEW_MACH_CODE
#endif
#endif
#endif

#if !(defined (NACL))
#if defined(LINUX) || defined(NEW_MACH_CODE)
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
int             closedir(DIR*);
#  endif
#endif

#if defined(WIN32)
#  include <io.h>
#  include <direct.h>
#endif

extern  int     allocgen(CSOUND *, char *, int (*)(FGDATA *, FUNC *));

/* module interface function names */

static  const   char    *opcode_init_Name =   "csound_opcode_init";
static  const   char    *fgen_init_Name =     "csound_fgen_init";

static  const   char    *PreInitFunc_Name =   "csoundModuleCreate";
static  const   char    *InitFunc_Name =      "csoundModuleInit";
static  const   char    *DestFunc_Name =      "csoundModuleDestroy";
static  const   char    *ErrCodeToStr_Name =  "csoundModuleErrorCodeToString";

static  const   char    *InfoFunc_Name =      "csoundModuleInfo";

/* environment variable storing path to plugin libraries */
static  const   char    *plugindir_envvar =   "OPCODE6DIR";
static  const   char    *plugindir64_envvar = "OPCODE6DIR64";

/* default directory to load plugins from if environment variable is not set */
#if !(defined (NACL))
#if !(defined(_CSOUND_RELEASE_) && (defined(LINUX) || defined(__MACH__)))
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
    long    (*opcode_init)(CSOUND *, OENTRY **);  /* list of opcode entries  */
    NGFENS  *(*fgen_init)(CSOUND *);        /* list of named GEN routines    */
    void    (*dummy)(void);                 /* unused                        */
} opcodeLibFunc_t;

typedef struct pluginLibFunc_s {
    int         (*InitFunc)(CSOUND *);      /* initialisation routine        */
    int         (*DestFunc)(CSOUND *);      /* destructor routine            */
    const char  *(*ErrCodeToStr)(int);      /* convert error code to string  */
} pluginLibFunc_t;

typedef struct csoundModule_s {
    struct csoundModule_s *nxt;             /* pointer to next link in chain */
    void        *h;                         /* library handle                */
    int         (*PreInitFunc)(CSOUND *);   /* pre-initialisation routine    */
                                            /*   (always NULL if opcode lib) */
    union {
      pluginLibFunc_t   p;                  /* generic plugin interface      */
      opcodeLibFunc_t   o;                  /* opcode library interface      */
    } fn;
    char        name[1];                    /* name of the module            */
} csoundModule_t;

static CS_NOINLINE void print_module_error(CSOUND *csound,
                                           const char *fmt, const char *fname,
                                           const csoundModule_t *m, int err)
{
    csoundMessageS(csound, CSOUNDMSG_ERROR, Str(fmt), fname);
    if (m != NULL && m->fn.p.ErrCodeToStr != NULL)
      csoundMessageS(csound, CSOUNDMSG_ERROR,
                       ": %s\n", Str(m->fn.p.ErrCodeToStr(err)));
    else
      csoundMessageS(csound, CSOUNDMSG_ERROR, "\n");
}

static int check_plugin_compatibility(CSOUND *csound, const char *fname, int n)
{
    int     myfltSize, minorVersion, majorVersion;

    myfltSize = n & 0xFF;
    if (UNLIKELY(myfltSize != 0 && myfltSize != (int) sizeof(MYFLT))) {
      csoundWarning(csound, Str("not loading '%s' (uses incompatible "
                                  "floating point type)"), fname);
      return -1;
    }
    if (UNLIKELY(n & (~0xFF))) {
      minorVersion = (n & 0xFF00) >> 8;
      majorVersion = (n & (~0xFFFF)) >> 16;
      if (majorVersion != (int) CS_APIVERSION ||
          (minorVersion > (int) CS_APISUBVER)) { /* NOTE **** REFACTOR *** */
        csoundWarning(csound, Str("not loading '%s' (incompatible "
                                    "with this version of Csound (%d.%d/%d.%d)"),
                        fname, majorVersion,minorVersion,
                        CS_APIVERSION,CS_APISUBVER);
        return -1;
      }
    }
    return 0;
}

/* load a single plugin library, and run csoundModuleCreate() if present */
/* returns zero on success */

static CS_NOINLINE int csoundLoadExternal(CSOUND *csound,
                                          const char *libraryPath)
{
    csoundModule_t  m;
    volatile jmp_buf tmpExitJmp;
    csoundModule_t  *mp;
    char            *fname;
    void            *h, *p;
    int             (*infoFunc)(void);
    int             err;

    /* check for a valid name */
    if (UNLIKELY(libraryPath == NULL || libraryPath[0] == '\0'))
      return CSOUND_ERROR;
    /* remove leading directory components from name */
    fname = (char*) libraryPath + (int) strlen(libraryPath);
    for ( ; fname[0] != DIRSEP && fname != (char*) libraryPath; fname--)
      ;
    if (fname[0] == DIRSEP)
      fname++;
    if (UNLIKELY(fname[0] == '\0'))
      return CSOUND_ERROR;
    /* load library */
/*  #if defined(LINUX) */
    //printf("About to open library '%s'\n", libraryPath);
/* #endif */
    err = csoundOpenLibrary(&h, libraryPath);
    if (UNLIKELY(err)) {
      char ERRSTR[256];
 #if !(defined(NACL)) && defined(LINUX)
      snprintf(ERRSTR, 256, Str("could not open library '%s' (%s)"),
               libraryPath, dlerror());
 #else
      snprintf(ERRSTR, 256, Str("could not open library '%s' (%d)"),
               libraryPath, err);
 #endif
      if (csound->delayederrormessages == NULL) {
        csound->delayederrormessages = malloc(strlen(ERRSTR)+1);
        strcpy(csound->delayederrormessages, ERRSTR);
      }
      else {
        char *new =
          realloc(csound->delayederrormessages,
                  strlen(csound->delayederrormessages)+strlen(ERRSTR)+11);
        if (new==NULL) {
          free(csound->delayederrormessages);
          return CSOUND_ERROR;
        }
        csound->delayederrormessages = new;
        strcat(csound->delayederrormessages, "\nWARNING: ");
        strcat(csound->delayederrormessages, ERRSTR);
      }
      return CSOUND_ERROR;
    }
    /* check if the library is compatible with this version of Csound */
    infoFunc = (int (*)(void)) csoundGetLibrarySymbol(h, InfoFunc_Name);
    if (infoFunc != NULL) {
      if (UNLIKELY(check_plugin_compatibility(csound, fname, infoFunc()) != 0)) {
        csoundCloseLibrary(h);
        return CSOUND_ERROR;
      }
    }
    /* was this plugin already loaded ? */
    for (mp = (csoundModule_t*) csound->csmodule_db; mp != NULL; mp = mp->nxt) {
      if (UNLIKELY(mp->h == h)) {
        csoundCloseLibrary(h);
        return CSOUND_SUCCESS;
      }
    }
    /* find out if it is a Csound plugin */
    memset(&m, 0, sizeof(csoundModule_t));
    m.h = h;
    m.PreInitFunc =
        (int (*)(CSOUND *)) csoundGetLibrarySymbol(h, PreInitFunc_Name);
    if (m.PreInitFunc != NULL) {
      /* generic plugin library */
      m.fn.p.InitFunc =
          (int (*)(CSOUND *)) csoundGetLibrarySymbol(h, InitFunc_Name);
      m.fn.p.DestFunc =
          (int (*)(CSOUND *)) csoundGetLibrarySymbol(h, DestFunc_Name);
      m.fn.p.ErrCodeToStr =
          (const char *(*)(int)) csoundGetLibrarySymbol(h, ErrCodeToStr_Name);
    }
    else {
      /* opcode library */
      m.fn.o.opcode_init =
          (long (*)(CSOUND *, OENTRY **))
              csoundGetLibrarySymbol(h, opcode_init_Name);
      m.fn.o.fgen_init =
          (NGFENS *(*)(CSOUND *)) csoundGetLibrarySymbol(h, fgen_init_Name);
      if (UNLIKELY(m.fn.o.opcode_init == NULL && m.fn.o.fgen_init == NULL)) {
        /* must have csound_opcode_init() or csound_fgen_init() */
        csoundCloseLibrary(h);
        if (csound->oparms->msglevel & 0x400)
          csound->Warning(csound, Str("'%s' is not a Csound plugin library"),
                          libraryPath);
        return CSOUND_ERROR;
      }
    }
    /* set up module info structure */
    /* (note: space for NUL character is already included in size of struct) */
    p = (void*) malloc(sizeof(csoundModule_t) + (size_t) strlen(fname));
    if (UNLIKELY(p == NULL)) {
      csoundCloseLibrary(h);
      csound->ErrorMsg(csound,
                       Str("csoundLoadExternal(): memory allocation failure"));
      return CSOUND_MEMORY;
    }
    mp = (csoundModule_t*) p;
    memcpy(mp, &m, sizeof(csoundModule_t));
    strcpy(&(mp->name[0]), fname);
    /* link into database */
    mp->nxt = (csoundModule_t*) csound->csmodule_db;
    csound->csmodule_db = (void*) mp;
    /* call csoundModuleCreate() if available */
    if (m.PreInitFunc != NULL) {
      memcpy((void*) &tmpExitJmp, (void*) &csound->exitjmp, sizeof(jmp_buf));
      if ((err = setjmp(csound->exitjmp)) != 0) {
        memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));
        print_module_error(csound, Str("Error in pre-initialisation function "
                                       "of module '%s'"), fname, NULL, 0);
        return (err == (CSOUND_EXITJMP_SUCCESS + CSOUND_MEMORY) ?
                CSOUND_MEMORY : CSOUND_INITIALIZATION);
      }
      err = m.PreInitFunc(csound);
      memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));
      if (UNLIKELY(err != 0)) {
        print_module_error(csound, Str("Error in pre-initialisation function "
                                       "of module '%s'"), fname, &m, err);
        return CSOUND_INITIALIZATION;
      }
    }
    /* plugin was loaded successfully */
    return CSOUND_SUCCESS;
}

static int csoundCheckOpcodeDeny(const char *fname)
{
    /* Check to see if the fname is on the do-not-load list */
    char buff[256];
    char *th;
    char *p, *deny;
    char *list = getenv("CS_OMIT_LIBS");
    /* printf("DEBUG %s(%d): check fname=%s\n", __FILE__, __LINE__, fname); */
    /* printf("DEBUG %s(%d): list %s\n", __FILE__, __LINE__, list); */
    if (list==NULL) return 0;
    strncpy(buff, fname, 255); buff[255]='\0';
    strrchr(buff, '.')[0] = '\0'; /* Remove .so etc */
    p = strdup(list);
    deny = cs_strtok_r(p, ",", &th);
    /* printf("DEBUG %s(%d): check buff=%s\n", __FILE__, __LINE__, deny); */
    while (deny) {
      /* printf("DEBUG %s(%d): deny=%s\n", __FILE__, __LINE__, deny); */
      if (strcmp(deny, buff)==0) {
        free(p);
        /* printf("DEBUG %s(%d): found\n", __FILE__, __LINE__); */
        return 1;
      }
      deny = cs_strtok_r(NULL, ",", &th);
    }
    free(p);
    /* printf("DEBUG %s(%d): not found\n", __FILE__, __LINE__); */
    return 0;
}

/**
 * Load plugin libraries for Csound instance 'csound', and call
 * pre-initialisation functions.
 * Return value is CSOUND_SUCCESS if there was no error, CSOUND_ERROR if
 * some modules could not be loaded or initialised, and CSOUND_MEMORY
 * if a memory allocation failure has occured.
 */
int csoundLoadModules(CSOUND *csound)
{
#if (defined(HAVE_DIRENT_H) && (TARGET_OS_IPHONE == 0))
    DIR             *dir;
    struct dirent   *f;
    const char      *dname, *fname;
    char            buf[1024];
    int             i, n, len, err = CSOUND_SUCCESS;

    if (UNLIKELY(csound->csmodule_db != NULL))
      return CSOUND_ERROR;

    /* open plugin directory */
    dname = csoundGetEnv(csound, (sizeof(MYFLT) == sizeof(float) ?
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
#else
      dname = "";
#endif

    }
    dir = opendir(dname);
    if (UNLIKELY(dir == (DIR*) NULL)) {
      //if (dname != NULL)  /* cannot be other */
      csound->Warning(csound, Str("Error opening plugin directory '%s': %s"),
                               dname, strerror(errno));
      //else
      //csound->Warning(csound, Str("Error opening plugin directory: %s"),
      //                         strerror(errno));
      return CSOUND_SUCCESS;
    }
    /* load database for deferred plugin loading */
/*     n = csoundLoadOpcodeDB(csound, dname); */
/*     if (n != 0) */
/*       return n; */
    /* scan all files in directory */
    while ((f = readdir(dir)) != NULL) {
      fname = &(f->d_name[0]);
      if (UNLIKELY(fname[0]=='_')) continue;
      n = len = (int) strlen(fname);
      if (UNLIKELY(fname[0]=='_')) continue;
#if defined(WIN32)
      strcpy(buf, "dll");
      n -= 4;
#elif defined(__MACH__)
      strcpy(buf, "dylib");
      n -= 6;
#else
      strcpy(buf, "so");
      n -= 3;
#endif
      if (n <= 0 || fname[n] != '.')
        continue;
      i = 0;
      do {
        if ((fname[++n] | (char) 0x20) != buf[i])
          break;
      } while (buf[++i] != '\0');
      if (buf[i] != '\0')
        continue;
      /* found a dynamic library, attempt to open it */
      if (UNLIKELY(((int) strlen(dname) + len + 2) > 1024)) {
        csound->Warning(csound, Str("path name too long, skipping '%s'"),
                                fname);
        continue;
      }
      /* printf("DEBUG %s(%d): possibly deny %s\n", __FILE__, __LINE__,fname); */
      if (csoundCheckOpcodeDeny(fname)) {
        csoundWarning(csound, Str("Library %s omitted\n"), fname);
        continue;
      }
      snprintf(buf, 1024, "%s%c%s", dname, DIRSEP, fname);
      if (csound->oparms->odebug) {
        csoundMessage(csound, Str("Loading '%s'\n"), buf);
      }
      n = csoundLoadExternal(csound, buf);
      if (UNLIKELY(n == CSOUND_ERROR))
        continue;               /* ignore non-plugin files */
      if (UNLIKELY(n < err))
        err = n;                /* record serious errors */
    }
    closedir(dir);
    return (err == CSOUND_INITIALIZATION ? CSOUND_ERROR : err);
#else
    return CSOUND_SUCCESS;
#endif  /* HAVE_DIRENT_H */
}

static int cmp_func(const void *p1, const void *p2)
{
    return (strcmp(*((const char**) p1), *((const char**) p2)));
}

int csoundLoadExternals(CSOUND *csound)
{
    char    *s, **lst;
    int     i, cnt, err;

    s = csound->dl_opcodes_oplibs;
    if (s == NULL || s[0] == '\0')
      return 0;
    /* IV - Feb 19 2005 */
    csound->dl_opcodes_oplibs = NULL;
    csoundMessage(csound, Str("Loading command-line libraries:\n"));
    cnt = 1;
    i = 0;
    do {
      if (s[i] == ',')
        cnt++;
    } while (s[++i] != '\0');
    lst = (char**) malloc(sizeof(char*) * cnt);
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
        err = csoundLoadExternal(csound, fname);
        if (UNLIKELY(err == CSOUND_INITIALIZATION || err == CSOUND_MEMORY))
          csoundDie(csound, Str(" *** error loading '%s'"), fname);
        else if (!err)
          csoundMessage(csound, "  %s\n", fname);
      }
    } while (++i < cnt);
    /* file list is no longer needed */
    free(lst);
    csound->Free(csound, s);
    return 0;
}

/**
 * Initialise a single module.
 * Return value is CSOUND_SUCCESS if there was no error.
 */
static CS_NOINLINE int csoundInitModule(CSOUND *csound, csoundModule_t *m)
{
    int     i;

    if (m->PreInitFunc != NULL) {
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
        long    length;
        /* load opcodes */
        if (UNLIKELY((length = m->fn.o.opcode_init(csound, &opcodlst_n)) < 0L))
          return CSOUND_ERROR;
        else {
          length /= (long) sizeof(OENTRY);
          if (length) {
            if (UNLIKELY(csoundAppendOpcodes(csound, opcodlst_n,
                                               (int) length) != 0))
              return CSOUND_ERROR;
          }
        }
      }
    }
    return CSOUND_SUCCESS;
}

/**
 * Call initialisation functions of all loaded modules that have a
 * csoundModuleInit symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and CSOUND_ERROR if
 * some modules could not be initialised.
 */

int csoundInitModules(CSOUND *csound)
{
    csoundModule_t  *m;
    int             i, retval = CSOUND_SUCCESS;

    /* call init functions */
    for (m = (csoundModule_t*) csound->csmodule_db; m != NULL; m = m->nxt) {
      i = csoundInitModule(csound, m);
      if (i != CSOUND_SUCCESS && i < retval)
        retval = i;
    }
    /* return with error code */
    return retval;
}

/* load a plugin library and also initialise it */
/* called on deferred loading of opcode plugins */

int csoundLoadAndInitModule(CSOUND *csound, const char *fname)
{
    volatile jmp_buf  tmpExitJmp;
    volatile int      err;

    err = csoundLoadExternal(csound, fname);
    if (UNLIKELY(err != 0))
      return err;
    memcpy((void*) &tmpExitJmp, (void*) &csound->exitjmp, sizeof(jmp_buf));
    if ((err = setjmp(csound->exitjmp)) != 0) {
      memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));
      return (err == (CSOUND_EXITJMP_SUCCESS + CSOUND_MEMORY) ?
              CSOUND_MEMORY : CSOUND_INITIALIZATION);
    }
    /* NOTE: this depends on csound->csmodule_db being the most recently */
    /* loaded plugin library */
    err = csoundInitModule(csound, (csoundModule_t*) csound->csmodule_db);
    memcpy((void*) &csound->exitjmp, (void*) &tmpExitJmp, sizeof(jmp_buf));

    return err;
}

/**
 * Call destructor functions of all loaded modules that have a
 * csoundModuleDestroy symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and
 * CSOUND_ERROR if some modules could not be de-initialised.
 */
extern int sfont_ModuleDestroy(CSOUND *csound);
int csoundDestroyModules(CSOUND *csound)
{
    csoundModule_t  *m;
    int             i, retval;

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
      csoundCloseLibrary(m->h);
      csound->csmodule_db = (void*) m->nxt;
      /* free memory used by database */
      free((void*) m);

    }
    sfont_ModuleDestroy(csound);
    /* return with error code */
    return retval;
}

 /* ------------------------------------------------------------------------ */

#if defined(WIN32)

PUBLIC int csoundOpenLibrary(void **library, const char *libraryPath)
{
    *library = (void*) LoadLibrary(libraryPath);
    return (*library != NULL ? 0 : -1);
}

PUBLIC int csoundCloseLibrary(void *library)
{
    return (int) (FreeLibrary((HMODULE) library) == FALSE ? -1 : 0);
}

PUBLIC void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    return (void*) GetProcAddress((HMODULE) library, procedureName);
}

#elif !(defined(NACL)) && (defined(LINUX) || defined (NEW_MACH_CODE))

PUBLIC int csoundOpenLibrary(void **library, const char *libraryPath)
{
    int flg = RTLD_NOW;
    if (libraryPath != NULL) {
      int len = (int) strlen(libraryPath);
      /* ugly hack to fix importing modules in Python opcodes */
      if (len >= 9 && strcmp(&(libraryPath[len - 9]), "/libpy.so") == 0)
        flg |= RTLD_GLOBAL;
    }
    *library = (void*) dlopen(libraryPath, flg);
    return (*library != NULL ? 0 : -1);
}

PUBLIC int csoundCloseLibrary(void *library)
{
    return (int) dlclose(library);
}

PUBLIC void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    return (void*) dlsym(library, procedureName);
}

#else /* case for platforms without shared libraries -- added 062404, akozar */

int csoundOpenLibrary(void **library, const char *libraryPath)
{
    *library = NULL;
    return -1;
}

int csoundCloseLibrary(void *library)
{
    return 0;
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    return NULL;
}

#endif

#if ENABLE_OPCODEDIR_WARNINGS
static const char *opcodedirWarnMsg[] = {
    "################################################################",
#ifndef USE_DOUBLE
    "#               WARNING: OPCODE6DIR IS NOT SET !               #",
#else
    "#              WARNING: OPCODE6DIR64 IS NOT SET !              #",
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

typedef long (*INITFN)(CSOUND *, void *);

extern long babo_localops_init(CSOUND *, void *);
extern long bilbar_localops_init(CSOUND *, void *);
extern long compress_localops_init(CSOUND *, void *);
extern long pvsbuffer_localops_init(CSOUND *, void *);
extern long pvsgendy_localops_init(CSOUND *, void *);
extern long vosim_localops_init(CSOUND *, void *);
extern long eqfil_localops_init(CSOUND *, void *);
extern long modal4_localops_init(CSOUND *, void *);
extern long scoreline_localops_init(CSOUND *, void *);
extern long physmod_localops_init(CSOUND *, void *);
extern long modmatrix_localops_init(CSOUND *, void *);
extern long spectra_localops_init(CSOUND *, void *);
extern long ambicode1_localops_init(CSOUND *, void *);
extern long grain4_localops_init(CSOUND *, void *);
extern long hrtferX_localops_init(CSOUND *, void *);
extern long loscilx_localops_init(CSOUND *, void *);
extern long pan2_localops_init(CSOUND *, void *);
extern long arrayvars_localops_init(CSOUND *, void *);
extern long phisem_localops_init(CSOUND *, void *);
extern long pvoc_localops_init(CSOUND *, void *);
extern long hrtfopcodes_localops_init(CSOUND *, void *);
extern long hrtfreverb_localops_init(CSOUND *, void *);
extern long hrtfearly_localops_init(CSOUND *, void *);
extern long minmax_localops_init(CSOUND *, void *);

extern long stackops_localops_init(CSOUND *, void *);
extern long vbap_localops_init(CSOUND *, void *);
extern long vaops_localops_init(CSOUND *, void*);
extern long ugakbari_localops_init(CSOUND *, void *);
extern long harmon_localops_init(CSOUND *, void *);
extern long pitchtrack_localops_init(CSOUND *, void *);

extern long partikkel_localops_init(CSOUND *, void *);
extern long shape_localops_init(CSOUND *, void *);
extern long tabsum_localops_init(CSOUND *, void *);
extern long crossfm_localops_init(CSOUND *, void *);
extern long pvlock_localops_init(CSOUND *, void *);
extern long fareyseq_localops_init(CSOUND *, void *);
extern long cpumeter_localops_init(CSOUND *, void *);
extern long gendy_localops_init(CSOUND *, void *);
extern long scnoise_localops_init(CSOUND *, void *);
#ifndef NACL
extern long socksend_localops_init(CSOUND *, void *);
extern long mp3in_localops_init(CSOUND *, void *);
extern long sockrecv_localops_init(CSOUND *, void *);
#endif
extern long afilts_localops_init(CSOUND *, void *);
extern long pinker_localops_init(CSOUND *, void *);

extern int stdopc_ModuleInit(CSOUND *csound);
extern int pvsopc_ModuleInit(CSOUND *csound);
extern int sfont_ModuleInit(CSOUND *csound);
extern int sfont_ModuleCreate(CSOUND *csound);
extern int newgabopc_ModuleInit(CSOUND *csound);

const INITFN staticmodules[] = { hrtfopcodes_localops_init, babo_localops_init,
                                 bilbar_localops_init, vosim_localops_init,
                                 compress_localops_init, pvsbuffer_localops_init,
                                 eqfil_localops_init, modal4_localops_init,
                                 scoreline_localops_init, physmod_localops_init,
                                 modmatrix_localops_init, spectra_localops_init,
                                 ambicode1_localops_init, grain4_localops_init,
                                 hrtferX_localops_init, loscilx_localops_init,
                                 pan2_localops_init, arrayvars_localops_init,
                                 phisem_localops_init, pvoc_localops_init,
                                 stackops_localops_init, vbap_localops_init,
                                 ugakbari_localops_init, harmon_localops_init,
                                 pitchtrack_localops_init, partikkel_localops_init,
                                 shape_localops_init, tabsum_localops_init,
                                 crossfm_localops_init, pvlock_localops_init,
                                 fareyseq_localops_init, hrtfearly_localops_init,
                                 hrtfreverb_localops_init, minmax_localops_init,
                                 vaops_localops_init, pvsgendy_localops_init,
#ifdef LINUX
                                 cpumeter_localops_init,
#endif
#ifndef NACL
                                 mp3in_localops_init,
                                 sockrecv_localops_init,
                                 socksend_localops_init,
#endif
                                 gendy_localops_init,
                                 scnoise_localops_init, afilts_localops_init,
                                 pinker_localops_init,
                                 NULL };

typedef NGFENS* (*FGINITFN)(CSOUND *);

NGFENS *ftest_fgens_init(CSOUND *);

const FGINITFN fgentab[] = {  ftest_fgens_init, NULL };

CS_NOINLINE int csoundInitStaticModules(CSOUND *csound)
{
    int     i;
    OENTRY  *opcodlst_n;
    long    length;

    for (i=0; staticmodules[i]!=NULL; i++) {
      length = (staticmodules[i])(csound, &opcodlst_n);

      if (UNLIKELY(length <= 0L)) return CSOUND_ERROR;
      length /= (long) sizeof(OENTRY);
      if (length) {
        if (UNLIKELY(csoundAppendOpcodes(csound, opcodlst_n,
                                           (int) length) != 0))
          return CSOUND_ERROR;
      }
    }
    /* stdopc module */
    if (stdopc_ModuleInit(csound)) return CSOUND_ERROR;

    /* pvs module */
    if (pvsopc_ModuleInit(csound)) return CSOUND_ERROR;

    /* sfont module */
    sfont_ModuleCreate(csound);
    if (sfont_ModuleInit(csound)) return CSOUND_ERROR;

    /* newgabopc */
    if (newgabopc_ModuleInit(csound)) return CSOUND_ERROR;

    /* modules were initialised successfully */
    /* Now fgens */
    for (i = 0; fgentab[i]!=NULL; i++) {
      int j;
      NGFENS  *names = (fgentab[i])(csound);
      for (j = 0; names[j].name != NULL; j++)
        allocgen(csound, names[j].name, names[j].fn);
    }
    return CSOUND_SUCCESS;
}
