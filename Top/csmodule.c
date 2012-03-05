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
 * variable OPCODEDIR (or the current directory if OPCODEDIR is unset) by     *
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
#if defined(MAC_OS_X_VERSION_10_6) && (MAC_OS_X_VERSION_MIN_REQUIRED>=MAC_OS_X_VERSION_10_6)
#define NEW_MACH_CODE
#else
#define OLD_MACH_CODE
#endif
#endif
#endif

#if defined(LINUX) || defined(NEW_MACH_CODE)
#include <dlfcn.h>
#elif defined(WIN32)
#include <windows.h>
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
#elif defined (OLD_MACH_CODE)
#  define ERR_STR_LEN 255
#  include <mach-o/dyld.h>
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
static  const   char    *plugindir_envvar =   "OPCODEDIR";
static  const   char    *plugindir64_envvar = "OPCODEDIR64";

/* default directory to load plugins from if environment variable is not set */
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
    csound->MessageS(csound, CSOUNDMSG_ERROR, Str(fmt), fname);
    if (m != NULL && m->fn.p.ErrCodeToStr != NULL)
      csound->MessageS(csound, CSOUNDMSG_ERROR,
                       ": %s\n", Str(m->fn.p.ErrCodeToStr(err)));
    else
      csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
}

static int check_plugin_compatibility(CSOUND *csound, const char *fname, int n)
{
    int     myfltSize, minorVersion, majorVersion;

    myfltSize = n & 0xFF;
    if (UNLIKELY(myfltSize != 0 && myfltSize != (int) sizeof(MYFLT))) {
      csound->Warning(csound, Str("not loading '%s' (uses incompatible "
                                  "floating point type)"), fname);
      return -1;
    }
    if (UNLIKELY(n & (~0xFF))) {
      minorVersion = (n & 0xFF00) >> 8;
      majorVersion = (n & (~0xFFFF)) >> 16;
      if (majorVersion != (int) CS_APIVERSION ||
          (minorVersion > (int) CS_APISUBVER) ||
          (minorVersion <= 5)) { /* NOTE **** REFACTOR *** */
        csound->Warning(csound, Str("not loading '%s' (incompatible "
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
/*       printf("About to open library '%s'\n", libraryPath); */
/* #endif */
    err = csound->OpenLibrary(&h, libraryPath);
    if (UNLIKELY(err)) {
      char ERRSTR[256];
 #if defined(LINUX)
      sprintf(ERRSTR, Str("could not open library '%s' (%s)"),
                      libraryPath, dlerror());
 #else
      sprintf(ERRSTR, Str("could not open library '%s' (%d)"),
                      libraryPath, err);
 #endif
      if (csound->delayederrormessages == NULL) {
        csound->delayederrormessages = malloc(strlen(ERRSTR)+1);
        strcpy(csound->delayederrormessages, ERRSTR);
      }
      else {
        csound->delayederrormessages =
          realloc(csound->delayederrormessages,
                  strlen(csound->delayederrormessages)+strlen(ERRSTR)+11);
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
    char *p, *deny;
    char *list = getenv("CS_OMIT_LIBS");
    /* printf("DEBUG %s(%d): check fname=%s\n", __FILE__, __LINE__, fname); */
    /* printf("DEBUG %s(%d): list %s\n", __FILE__, __LINE__, list); */
    if (list==NULL) return 0;
    strcpy(buff, fname);
    strrchr(buff, '.')[0] = '\0'; /* Remove .so etc */
    p = strdup(list);
    deny = strtok(p, ",");
    /* printf("DEBUG %s(%d): check buff=%s\n", __FILE__, __LINE__, deny); */
    while (deny) {
      /* printf("DEBUG %s(%d): deny=%s\n", __FILE__, __LINE__, deny); */
      if (strcmp(deny, buff)==0) {
        free(p);
        /* printf("DEBUG %s(%d): found\n", __FILE__, __LINE__); */
        return 1;
      }
      deny = strtok(NULL, ",");
    }
    free(p);
    /* printf("DEBUG %s(%d): not found\n", __FILE__, __LINE__); */
    return 0;
}
#ifdef mac_classic
/* The following code implements scanning of "OPCODEDIR"
   and auto-loading of plugins for MacOS 9 */


/* These structures describe the 'cfrg' resource used by system sw */
typedef struct {
    OSType  codeType;
    long    updateLevel;
    long    curVersion;
    long    oldVersion;
    long    stackSize;
    int16   libraryDir;
    char    fragType;
    char    fragLocation;
    long    fragOffset;
    long    fragLength;
    long    reservedA;
    long    reservedB;
    int16   descLength;
    Str63   fragName;
} cfrg_desc;

typedef struct {
    long    reserved1;
    long    reserved2;
    long    version;
    long    reserved3;
    long    reserved4;
    long    reserved5;
    long    reserved6;
    long    numFragDesc;
    cfrg_desc firstDesc;
} cfrg_rsrc;


static void CopyPascalString(StringPtr to, StringPtr from)
{
    BlockMoveData(from, to, *from+1);
    return;
}

/*  Copies a pascal string to a C string in space you provide.
    Returns false if it runs out of space. */
static Boolean CopyPascalToCString(char* target, const StringPtr source,
                                   size_t availspace)
{
    Boolean enough_space = ((size_t)source[0] < availspace);
    size_t  bytes2copy = (enough_space ? (size_t)source[0] : (availspace-1));
    unsigned char* pos = source + 1;

    while (bytes2copy--) *target++ = *pos++;
    *target = '\0';
    return enough_space;
}

/* Returns in "me" the location of the currently running host application. */
static OSErr GetHostLocation( FSSpecPtr me )
{
    ProcessSerialNumber psn;
    ProcessInfoRec      pinfo;
    OSErr               err;

    /* get info for current process */
    err = GetCurrentProcess(&psn);
    if  (UNLIKELY(err != noErr))  return err;
    pinfo.processInfoLength = sizeof(ProcessInfoRec);
    pinfo.processName = NULL;
    pinfo.processAppSpec = me;
    err = GetProcessInformation(&psn, &pinfo);
    if  (UNLIKELY(err != noErr))  return err;
    return noErr;
}

#define kCsoundExtFolder "\pCsound"

/* Looks for a folder or alias to one named "Csound" in the extensions folder */
static OSErr FindCsoundExtensionsFolder(FSSpecPtr location)
{
    OSErr   err;
    int16   systemVRefNum;
    long    extDirID;
    long    csoundDirID;

    /* find the system extensions folder */
    err = FindFolder(kOnSystemDisk, kExtensionFolderType, kCreateFolder,
            &systemVRefNum, &extDirID);
    if (UNLIKELY(err != noErr)) return err;

    /* look for subfolder named "Csound" */
    err = FSMakeFSSpec(systemVRefNum, extDirID, kCsoundExtFolder, location);
    if (err != noErr) return err;  /* does not exist */
    else {
        Boolean targetIsFolder;
        Boolean wasAliased;

        err = ResolveAliasFile(location, true, &targetIsFolder, &wasAliased);
        if (UNLIKELY(err != noErr)) return err;
        if (UNLIKELY(!targetIsFolder))
          return paramErr; /* "Csound" is a file, not a folder */
    }

    return noErr;
}

/*  Gets the folder ID for a directory specified via a full FSSpec. */
static OSErr GetFolderID(const FSSpecPtr loc, long* dirID)
{
    OSErr       err;
    CInfoPBRec  catinfo;
    Str255      name;

    CopyPascalString(name, loc->name);
    catinfo.dirInfo.ioCompletion = NULL;
    catinfo.dirInfo.ioNamePtr = name;
    catinfo.dirInfo.ioVRefNum = loc->vRefNum;
    catinfo.dirInfo.ioFDirIndex = 0;    /* we want info about the named folder */
    catinfo.dirInfo.ioDrDirID = loc->parID;

    err = PBGetCatInfo(&catinfo, false);
    *dirID = catinfo.dirInfo.ioDrDirID;
    return err;
}

/* Gets the name for a library that the Code Fragment Manager needs to load it */
static OSErr GetFragmentName(CSOUND* csound, FSSpecPtr libr, char* name)
{
    OSErr       err;
    Handle      cfrgh;
    cfrg_rsrc*  cfrg;
    int16       refnum;

    const char kIsLib = 0;  /* fragType for import libraries */

    /* open the plugin's resource fork */
    refnum = FSpOpenResFile(libr, fsRdPerm);
    err = ResError();
    if (UNLIKELY(err != noErr)) return err;

    /* load the plugin's 'cfrg' resource */
    cfrgh = GetResource('cfrg', 0);
    err = ResError();
    if  (UNLIKELY(err != noErr)) {
        CloseResFile(refnum);
        return err;
    }
    cfrg = (cfrg_rsrc*)*cfrgh;
    if  (UNLIKELY(cfrg->version != 0x00000001 || cfrg->numFragDesc < 1)) {
        ReleaseResource(cfrgh);
        CloseResFile(refnum);
        return -1;
    }
    /* we assume the library we want is the first fragment descriptor */
    if  (LIKELY(cfrg->firstDesc.fragType == kIsLib)) {
        CopyPascalToCString(name, cfrg->firstDesc.fragName, 255);
        err = noErr;
    }
    else err = -1;

    ReleaseResource(cfrgh);
    CloseResFile(refnum);
    return err;
}

/* Examine each file in theFolder and load it if it is a Csound plugin */
static OSErr SearchFolderAndLoadPlugins(CSOUND *csound, FSSpecPtr theFolder, int* cserr)
{
    OSErr      err, err2;
    int        result;
    Str63      name;
    char       fragname[255];
    CInfoPBRec catinfo;
    FSSpec     spec;
    long       folderID;
    int16      idx = 1;

    const char kFolderBit = (1<<4);

    err = GetFolderID(theFolder, &folderID);
    if (UNLIKELY(err != noErr)) return err;

    *cserr = CSOUND_SUCCESS;
    catinfo.hFileInfo.ioCompletion = NULL;
    catinfo.hFileInfo.ioVRefNum = theFolder->vRefNum;
    catinfo.hFileInfo.ioNamePtr = name;
    do {
        catinfo.hFileInfo.ioFDirIndex = idx;
        catinfo.hFileInfo.ioDirID = folderID;
        catinfo.hFileInfo.ioACUser = 0;
        err = PBGetCatInfo(&catinfo, false);
        /* ignore folders */
        if (err == noErr && !(catinfo.hFileInfo.ioFlAttrib & kFolderBit)) {
            if (catinfo.hFileInfo.ioFlFndrInfo.fdType == 'shlb' &&
                catinfo.hFileInfo.ioFlFndrInfo.fdCreator == 'Csnd') {
                /* this is a Csound plugin library */
                err2 = FSMakeFSSpec(catinfo.hFileInfo.ioVRefNum,
                      catinfo.hFileInfo.ioFlParID, catinfo.hFileInfo.ioNamePtr, &spec);
                if (err2 != noErr) continue; /* this really should not happen */
                err2 = GetFragmentName(csound, &spec, fragname);
                result = CSOUND_SUCCESS;
                if (err2 == noErr) result = csoundLoadExternal(csound, fragname);
                /* record serious errors */
                if (result != CSOUND_SUCCESS && result != CSOUND_ERROR) *cserr = result;
                /* continue to search folder when one file fails to load */
            }
        }
        ++idx;
    } while (err == noErr);
    return noErr;
}

int csoundLoadModules(CSOUND *csound)
{
    OSErr       err;
    int         cserr;
    Handle      cfrgh;
    AliasHandle alias;
    cfrg_rsrc*  cfrg;
    int16       alisID;
    Boolean     wasChanged;
    FSSpec      pluginDir, fromFile;

    /* find the "Plugins" folder */
    /* first load the host application's 'cfrg' resource */
    cfrgh = GetResource('cfrg', 0);
    err = ResError();
    if  (UNLIKELY(err != noErr || cfrgh == NULL)) {
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }
    cfrg = (cfrg_rsrc*)*cfrgh;
    if  (UNLIKELY(cfrg->version != 0x00000001 || cfrg->numFragDesc < 1)) {
        ReleaseResource(cfrgh);
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }
    alisID = cfrg->firstDesc.libraryDir;
    ReleaseResource(cfrgh);
    cfrgh = NULL; cfrg = NULL;

    /* now load the 'alis' resource that points to "Plugins" */
    alias = (AliasHandle)GetResource('alis', alisID);
    err = ResError();
    if  (UNLIKELY(err != noErr || alias == NULL)) {
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }
    /* resolve alias relative to host application */
    err = GetHostLocation(&fromFile);
    if  (UNLIKELY(err != noErr)) {
        ReleaseResource((Handle)alias);
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }
    err = ResolveAlias(&fromFile, alias, &pluginDir, &wasChanged);
    ReleaseResource((Handle)alias);
    if  (UNLIKELY(err != noErr)) {
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }

    cserr = CSOUND_SUCCESS;
    /* search the "Plugins" folder for Csound plugin libraries */
    err = SearchFolderAndLoadPlugins(csound, &pluginDir, &cserr);
    if  (UNLIKELY(err != noErr)) {
        csound->ErrorMsg(csound, Str("Error opening plugin directory\n"));
        return CSOUND_ERROR;
    }
    if  (UNLIKELY(cserr != CSOUND_SUCCESS)) return cserr;

    /* finally, locate and search our Extensions subfolder for plugins */
    err = FindCsoundExtensionsFolder(&pluginDir);
    if  (err == noErr)
        SearchFolderAndLoadPlugins(csound, &pluginDir, &cserr);
    /* ignore errors from search & we don't care if unable to locate */

    return cserr;
}
#endif /* mac_classic library searching */

#ifndef mac_classic
/**
 * Load plugin libraries for Csound instance 'csound', and call
 * pre-initialisation functions.
 * Return value is CSOUND_SUCCESS if there was no error, CSOUND_ERROR if
 * some modules could not be loaded or initialised, and CSOUND_MEMORY
 * if a memory allocation failure has occured.
 */
int csoundLoadModules(CSOUND *csound)
{
#ifdef HAVE_DIRENT_H
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
        dname = CS_DEFAULT_PLUGINDIR;
    }
    dir = opendir(dname);
    if (UNLIKELY(dir == (DIR*) NULL)) {
      csound->ErrorMsg(csound, Str("Error opening plugin directory '%s': %s"),
                               dname, strerror(errno));
      return CSOUND_ERROR;
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
        csound->Warning(csound, Str("Library %s omitted\n"), fname);
        continue;
      }
      if (csoundCheckOpcodePluginFile(csound, fname) != 0)
        continue;               /* skip file if marked for deferred loading */
      sprintf(buf, "%s%c%s", dname, DIRSEP, fname);
/*       printf("Loading: %s\n", buf); */
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
#endif /* not mac_classic */

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
    csound->Message(csound, Str("Loading command-line libraries:\n"));
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
          csound->Die(csound, Str(" *** error loading '%s'"), fname);
        else if (!err)
          csound->Message(csound, "  %s\n", fname);
      }
    } while (++i < cnt);
    /* file list is no longer needed */
    free(lst);
    mfree(csound, s);
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
            if (UNLIKELY(csound->AppendOpcodes(csound, opcodlst_n,
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

#elif defined(LINUX) || defined (NEW_MACH_CODE)

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

#elif defined (OLD_MACH_CODE)

/* Set and get the error string for use by dlerror */
static const char *error(int setget, const char *str, ...)
{
    static char errstr[ERR_STR_LEN];
    static int  err_filled = 0;
    const char  *retval;
    NSLinkEditErrors ler;
    int         lerno;
    const char  *dylderrstr;
    const char  *file;
    va_list     arg;
    if (setget <= 0) {
      va_start(arg, str);
      strncpy(errstr, "dlsimple: ", ERR_STR_LEN);
      vsnprintf(errstr + 10, ERR_STR_LEN - 10, str, arg);
      va_end(arg);
      /* We prefer to use the dyld error string if getset is 1*/
      if (setget == 0) {
        NSLinkEditError(&ler, &lerno, &file, &dylderrstr);
#if 0
        fprintf(stderr, "dyld: %s\n", dylderrstr);
#endif
        if (dylderrstr && strlen(dylderrstr))
          strncpy(errstr, dylderrstr, ERR_STR_LEN);
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
static void *dlsymIntern(void *handle, const char *symbol)
{
    NSSymbol  nssym = (NSSymbol) 0;
    /* If the handle is -1, if is the app global context */
    if (handle == (void*) -1L) {
      /* Global context, use NSLookupAndBindSymbol */
      if (NSIsSymbolNameDefined(symbol)) {
        nssym = NSLookupAndBindSymbol(symbol);
      }
    }
    /* Now see if the handle is a struct mach_header* or not,
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
                            (struct mach_header *)handle, symbol,
                            NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                            NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
        }
      }
      else {
        nssym = NSLookupSymbolInModule(handle, symbol);
      }
    }
    if (UNLIKELY(!nssym)) {
      error(0, "Symbol \"%s\" Not found", symbol);
      return NULL;
    }
    return NSAddressOfSymbol(nssym);
}

int csoundOpenLibrary(void **library, const char *libraryPath)
{
    NSObjectFileImage ofi = 0;
    NSObjectFileImageReturnCode ofirc;
    static int (*make_private_module_public) (NSModule module) = NULL;
    unsigned int flags =  NSLINKMODULE_OPTION_RETURN_ON_ERROR |
                          NSLINKMODULE_OPTION_PRIVATE;

    /* If we got no path, the app wants the global namespace,
       use -1 as the marker in this case */
    if (!libraryPath) {
      *library = (void*) -1L;
      return 0;
    }
    *library = NULL;

    /* Create the object file image, works for things linked
       with the -bundle arg to ld */
    ofirc = NSCreateObjectFileImageFromFile(libraryPath, &ofi);
    switch (ofirc) {
    case NSObjectFileImageSuccess:
      /* It was okay, so use NSLinkModule to link in the image */
      *library = NSLinkModule(ofi, libraryPath, flags);
      /* Don't forget to destroy the object file
         image, unless you like leaks */
      NSDestroyObjectFileImage(ofi);
      /* If the mode was global, then change the module, this avoids
         multiply defined symbol errors to first load private then
         make global. Silly, isn't it. */
      if (!make_private_module_public) {
        _dyld_func_lookup("__dyld_NSMakePrivateModulePublic",
                          (void **) &make_private_module_public);
      }
      make_private_module_public(*library);
      break;
    case NSObjectFileImageInappropriateFile:
      /* It may have been a dynamic library rather
         than a bundle, try to load it */
      *library = (void*) NSAddImage(libraryPath,
                                    NSADDIMAGE_OPTION_RETURN_ON_ERROR);
      break;
    case NSObjectFileImageFailure:
      error(0, "Object file setup failure :  \"%s\"", libraryPath);
      return -1;
    case NSObjectFileImageArch:
      error(0, "No object for this architecture :  \"%s\"", libraryPath);
      return -1;
    case NSObjectFileImageFormat:
      error(0, "Bad object file format :  \"%s\"", libraryPath);
      return -1;
    case NSObjectFileImageAccess:
      error(0, "Can't read object file :  \"%s\"", libraryPath);
      return -1;
    }
    if (UNLIKELY(*library == NULL)) {
      error(0, "Can not open \"%s\"", libraryPath);
      return -1;
    }
    return 0;
}

int csoundCloseLibrary(void *library)
{
    if (UNLIKELY((((struct mach_header *)library)->magic == MH_MAGIC) ||
                 (((struct mach_header *)library)->magic == MH_CIGAM))) {
      error(-1, "Can't remove dynamic libraries on darwin");
      return -1;
    }
    if (UNLIKELY(!NSUnLinkModule(library, 0))) {
      error(0, "unable to unlink module %s", NSNameOfModule(library));
      return -1;
    }
    return 0;
}

void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    char  undersym[257];

    if (UNLIKELY((int) strlen(procedureName) > 255)) {
      error(-1, "Symbol name is too long");
      return NULL;
    }
    sprintf(undersym, "_%s", procedureName);
    return (void*) dlsymIntern(library, undersym);
}

#elif defined(mac_classic)

PUBLIC int csoundOpenLibrary(void **library, const char *libraryName)
{
    CFragConnectionID connID;
    Ptr         mainAddr;
    OSErr       err;
    Str63       macLibName;
    Str255      errName;

    *library = NULL;
    if (LIKELY(strlen(libraryName) < 64)) {
      strcpy((char*) macLibName, libraryName);
      c2pstr((char*) macLibName);
    }
    else {
 /*   csoundMessage("%s is not a valid library name because it is too long.\n",
                    libraryName); */
      return -1;
    }
    /* first, test to see if the library is already loaded */
    err = GetSharedLibrary(macLibName, kPowerPCCFragArch, kFindCFrag,
                           &connID, &mainAddr, errName);
    if (UNLIKELY(err == noErr))
      return -1;        /* already loaded */
    else if (UNLIKELY(err != cfragLibConnErr)) {  /* some other error occurred */
 /*   csoundMessage("Failed to load plugin library %s with Mac error %d.\n",
                    libraryName, err); */
      return -1;
    }
    else {  /* attempt to load the library */
      err = GetSharedLibrary(macLibName, kPowerPCCFragArch, kLoadCFrag,
                             &connID, &mainAddr, errName);
      if (UNLIKELY(err != noErr)) {
 /*     csoundMessage("Failed to load plugin library %s with Mac error %d.\n",
                      libraryName, err); */
        return -1;
      }
    }
    *library = (void*) connID;
    return 0;
}

PUBLIC int csoundCloseLibrary(void *library)
{
    CFragConnectionID connID;
    OSErr       err;

    connID = (CFragConnectionID) library;
    err = CloseConnection(&connID);
    return 0 /* (err != noErr) */;  /* ignore errors */
}

PUBLIC void *csoundGetLibrarySymbol(void *library, const char *procedureName)
{
    OSErr       err;
    Ptr         symAddr;
    CFragSymbolClass  symClass;
    CFragConnectionID connID;
    Str255      macProcName;

    connID = (CFragConnectionID) library;
    if (LIKELY(strlen(procedureName) < 256)) {
      strcpy((char*) macProcName, procedureName);
      c2pstr((char*) macProcName);
    }
    else {
 /*   csoundMessage("%s is not a valid library procedure name "
                    "because it is too long.\n", procedureName); */
      return NULL;
    }
    err = FindSymbol(connID, macProcName, &symAddr, &symClass);
    if (UNLIKELY(err != noErr)) {
 /*   csoundMessage("Failed to find library procedure %s with Mac error %d.\n",
                    procedureName, err); */
      return NULL;
    }
    else if (UNLIKELY(symClass == kDataCFragSymbol)) {
 /*   csoundMessage("Failed to load procedure %s "
                    "because it is not a code symbol.\n", procedureName); */
      return NULL;
    }

    return (void*) symAddr;
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
    "#               WARNING: OPCODEDIR IS NOT SET !                #",
#else
    "#              WARNING: OPCODEDIR64 IS NOT SET !               #",
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
extern long tabvars_localops_init(CSOUND *, void *);
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
extern long mp3in_localops_init(CSOUND *, void *);
extern long gendy_localops_init(CSOUND *, void *);
extern long scnoise_localops_init(CSOUND *, void *);

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
                                 pan2_localops_init, tabvars_localops_init,
                                 phisem_localops_init, pvoc_localops_init,
                                 stackops_localops_init, vbap_localops_init,
                                 ugakbari_localops_init, harmon_localops_init,
                                 pitchtrack_localops_init, partikkel_localops_init,
                                 shape_localops_init, tabsum_localops_init,
                                 crossfm_localops_init, pvlock_localops_init,
                                 fareyseq_localops_init, hrtfearly_localops_init,
                                 hrtfreverb_localops_init, minmax_localops_init,
                                 vaops_localops_init,
#ifndef WIN32
                                 cpumeter_localops_init,
#endif
                                 mp3in_localops_init, gendy_localops_init,
                                 scnoise_localops_init, NULL };

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
        if (UNLIKELY(csound->AppendOpcodes(csound, opcodlst_n,
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
