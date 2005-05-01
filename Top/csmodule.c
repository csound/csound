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
 * csoundCreate() after the creation of a Csound instance, and are unloaded   *
 * at the end of performance.                                                 *
 * A library may export any of the following four interface functions,        *
 * however, the presence of csoundModuleCreate() is required for identifying  *
 * the file as a Csound plugin module.                                        *
 *                                                                            *
 * int csoundModuleCreate(void *csound)         (required)                    *
 * ------------------------------------                                       *
 *                                                                            *
 * Pre-initialisation function that is called by csoundCreate() after the     *
 * creation of Csound instance 'csound'.                                      *
 *                                                                            *
 * int csoundModuleInit(void *csound)           (optional)                    *
 * ----------------------------------                                         *
 *                                                                            *
 * Called by Csound instances before orchestra translation. One possible use  *
 * of csoundModuleInit() is adding new opcodes with csoundAppendOpcode().     *
 *                                                                            *
 * int csoundModuleDestroy(void *csound)        (optional)                    *
 * -------------------------------------                                      *
 *                                                                            *
 * Destructor function for Csound instance 'csound', called at the end of     *
 * performance, after closing audio output.                                   *
 *                                                                            *
 * char *csoundModuleErrorCodeToString(int errcode)     (optional)            *
 * ------------------------------------------------                           *
 *                                                                            *
 * Converts error codes returned by any of the initialisation or destructor   *
 * functions to a string message.                                             *
 *                                                                            *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "csoundCore.h"
#include "csound.h"
#include "csmodule.h"

#if defined(HAVE_DIRENT_H)
#include <dirent.h>
#ifdef __MACH__
#ifdef DIRENT_FIX
typedef void* DIR;
DIR opendir(const char *);
struct dirent *readdir(DIR*);
int closedir(DIR*);
#endif
#endif
#endif

/* module interface function names */
static  const   char    *PreInitFunc_Name =   "csoundModuleCreate";
static  const   char    *InitFunc_Name =      "csoundModuleInit";
static  const   char    *DestFunc_Name =      "csoundModuleDestroy";
static  const   char    *ErrCodeToStr_Name =  "csoundModuleErrorCodeToString";

/* environment variable storing path to plugin libraries */
static  const   char    *plugindir_envvar =     "OPCODEDIR";
#ifdef USE_DOUBLE
static  const   char    *plugindir64_envvar =   "OPCODEDIR64";
#endif
/* default directory to load plugins from if environment variable is not set */
static  const   char    *default_plugin_dir =   ".";
/* name of dynamic variable storing plugin database */
static  const   char    *plugindb_name =        "::modules.DB";

typedef struct csoundModule_s {
    struct csoundModule_s *nxt;             /* pointer to next link in chain */
    void    *h;                             /* library handle                */
    char    *name;                          /* name of the module            */
    int     (*PreInitFunc)(void *csound);   /* pre-initialisation routine    */
    int     (*InitFunc)(void *csound);      /* initialisation routine        */
    int     (*DestFunc)(void *csound);      /* destructor routine            */
    char    *(*ErrCodeToStr)(int errcode);  /* convert error code to string  */
} csoundModule_t;

/* load all modules from plugin directory, and build module database */

static int build_module_database(void *csound_)
{
#ifdef HAVE_DIRENT_H
    ENVIRON         *csound = (ENVIRON*) csound_;
    DIR             *dir;
    struct dirent   *f;
    csoundModule_t  *m, **plugin_db;
    char            *dname = NULL, *fname, *buf1;
    void            *h, *sym;
    int             is_library, len;
    /* open plugin directory */
#ifdef USE_DOUBLE
    if (csoundGetEnv(csound, plugindir64_envvar) != NULL)
      dname = (char*) csoundGetEnv(csound, plugindir64_envvar);
    else {
#endif
      if (csoundGetEnv(csound, plugindir_envvar) != NULL)
        dname = (char*) csoundGetEnv(csound, plugindir_envvar);
      else
        dname = (char*) default_plugin_dir;
#ifdef USE_DOUBLE
    }
#endif
    dir = opendir(dname);
    if (dir == (DIR*) NULL) {
      csound->Message(csound, Str("Error opening plugin directory '%s': %s\n"),
                 dname, strerror(errno));
      return CSOUND_ERROR;
    }
    /* get pointer to database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db == NULL)
      return CSOUND_ERROR;
    /* scan all files in directory */
    while ((f = readdir(dir)) != NULL) {
      is_library = 0;
      fname = (char*) &(f->d_name[0]);
      len = (int) strlen(fname);
#if defined(__unix) || defined(__unix__) || defined(LINUX)
      if (len >= 4) {
        if ((fname[len - 1] == 'o' || fname[len - 1] == 'O') &&
            (fname[len - 2] == 's' || fname[len - 2] == 'S') &&
            (fname[len - 3] == '.' || fname[len - 3] == '.'))
          is_library = 1;
      }
#elif defined(WIN32)
      if (len >= 5) {
        if ((fname[len - 1] == 'l' || fname[len - 1] == 'L') &&
            (fname[len - 2] == 'l' || fname[len - 2] == 'L') &&
            (fname[len - 3] == 'd' || fname[len - 3] == 'D') &&
            (fname[len - 4] == '.' || fname[len - 4] == '.'))
          is_library = 1;
      }
#elif defined(__MACH__)
      if (len >= 7) {
        if ((fname[len - 1] == 'b' || fname[len - 1] == 'B') &&
            (fname[len - 2] == 'i' || fname[len - 2] == 'I') &&
            (fname[len - 3] == 'l' || fname[len - 3] == 'L') &&
            (fname[len - 4] == 'y' || fname[len - 4] == 'Y') &&
            (fname[len - 5] == 'd' || fname[len - 5] == 'D') &&
            (fname[len - 6] == '.' || fname[len - 6] == '.'))
          is_library = 1;
      }
#endif
      if (!is_library)
        continue;
      /* found a dynamic library, attempt to open it */
      buf1 = (char*) malloc(sizeof(char) * (size_t) ((int) strlen(dname)
                                                     + len + 2));
      if (buf1 == NULL)
        return CSOUND_MEMORY;
      sprintf(buf1, "%s%c%s", dname, DIRSEP, fname);
      h = (void*) csoundOpenLibrary(buf1);
      free((void*) buf1);
      if (h == NULL) {
        csound->Message(csound, Str("WARNING: could not open library '%s'\n"),
                                fname);
        continue;
      }
      /* check if it is actually a Csound plugin file */
      sym = (void*) csoundGetLibrarySymbol(h, PreInitFunc_Name);
      if (sym == NULL) {
        /* not a plugin file */
        sym = (void*) csoundGetLibrarySymbol(h, "opcode_size");
        if (sym == NULL) {
          /* if it is not even an opcode library, print warning */
          csound->Message(csound,
                          Str("WARNING: '%s' is not a Csound plugin library\n"),
                          fname);
        }
        csoundCloseLibrary(h);
        continue;
      }
      /* create module structure */
      m = (csoundModule_t*)
            ((void*) malloc((size_t)
                            ((((int) sizeof(csoundModule_t) + 15) & (~15))
                             + (((len + 1) + 15) & (~15)))));
      if (m == NULL) {
        csoundCloseLibrary(h);
        return CSOUND_MEMORY;
      }
      m->nxt = (*plugin_db);
      m->h = h;
      m->name = (char*) m + (((int) sizeof(csoundModule_t) + 15) & (~15));
      strcpy(m->name, fname);
      m->PreInitFunc =
        (int (*)(void*)) csoundGetLibrarySymbol(h, PreInitFunc_Name);
      m->InitFunc = (int (*)(void*)) csoundGetLibrarySymbol(h, InitFunc_Name);
      m->DestFunc = (int (*)(void*)) csoundGetLibrarySymbol(h, DestFunc_Name);
      m->ErrCodeToStr =
        (char *(*)(int)) csoundGetLibrarySymbol(h, ErrCodeToStr_Name);
      /* link into chain */
      (*plugin_db) = m;
    }
    closedir(dir);
#endif  /* HAVE_DIRENT_H */
    return CSOUND_SUCCESS;
}

/* unload all external modules, and free memory used by database */

static void destroy_module_database(void *csound)
{
    csoundModule_t  *m, *prv, **plugin_db;

    /* get pointer to database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db == NULL)
      return;
    prv = (csoundModule_t*) NULL;
    m = (*plugin_db);
    while (m != NULL) {
      prv = m;
      csoundCloseLibrary(m->h);
      m = m->nxt;
      free((void*) prv);
    }
    (*plugin_db) = (csoundModule_t*) NULL;
    csoundDestroyGlobalVariable(csound, plugindb_name);
}

/**
 * Load plugin libraries for Csound instance 'csound', and call
 * pre-initialisation functions.
 * Return value is CSOUND_SUCCESS if there was no error, CSOUND_ERROR if
 * some modules could not be loaded or initialised, and CSOUND_MEMORY
 * if a memory allocation failure has occured.
 */
int csoundLoadModules(void *csound_)
{
    ENVIRON         *csound = (ENVIRON*) csound_;
    csoundModule_t  *m, **plugin_db;
    int             i, retval;

    /* create database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db != NULL)
      return CSOUND_ERROR;      /* already have one */
    retval = csoundCreateGlobalVariable(csound, plugindb_name,
                                        sizeof(csoundModule_t*));
    if (retval != CSOUND_SUCCESS)
      return retval;
    retval = build_module_database(csound);
    if (retval != CSOUND_SUCCESS)
      return retval;
    /* get pointer to database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db == NULL)
      return CSOUND_ERROR;
    /* call init functions */
    m = (*plugin_db);
    retval = CSOUND_SUCCESS;
    while (m != NULL) {
      if (m->PreInitFunc != NULL) {
        i = m->PreInitFunc(csound);
        if (i != 0) {
          retval = CSOUND_ERROR;
          csound->Message(csound, Str("Error in pre-initialisation function "
                                      "of module '%s'"), m->name);
          if (m->ErrCodeToStr != NULL)
            csound->Message(csound, ": %s", Str(m->ErrCodeToStr(i)));
          csound->Message(csound, "\n");
        }
      }
      m = m->nxt;
    }
    /* return with error code */
    return retval;
}

/**
 * Call initialisation functions of all loaded modules that have a
 * csoundModuleInit symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and CSOUND_ERROR if
 * some modules could not be initialised.
 */
int csoundInitModules(void *csound_)
{
    ENVIRON         *csound = (ENVIRON*) csound_;
    csoundModule_t  *m, **plugin_db;
    int             i, retval;

    /* get pointer to database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db == NULL)
      return CSOUND_ERROR;
    /* call init functions */
    m = (*plugin_db);
    retval = CSOUND_SUCCESS;
    while (m != NULL) {
      if (m->InitFunc != NULL) {
        i = m->InitFunc(csound);
        if (i != 0) {
          retval = CSOUND_ERROR;
          csound->Message(csound, Str("Error starting module '%s'"), m->name);
          if (m->ErrCodeToStr != NULL)
            csound->Message(csound, ": %s", Str(m->ErrCodeToStr(i)));
          csound->Message(csound, "\n");
        }
      }
      m = m->nxt;
    }
    /* return with error code */
    return retval;
}

/**
 * Call destructor functions of all loaded modules that have a
 * csoundModuleDestroy symbol, for Csound instance 'csound'.
 * Return value is CSOUND_SUCCESS if there was no error, and
 * CSOUND_ERROR if some modules could not be de-initialised.
 */
int csoundDestroyModules(void *csound_)
{
    ENVIRON         *csound = (ENVIRON*) csound_;
    csoundModule_t  *m, **plugin_db;
    int             i, retval;

    /* get pointer to database */
    plugin_db =
      (csoundModule_t**) (csoundQueryGlobalVariable(csound, plugindb_name));
    if (plugin_db == NULL)
      return CSOUND_ERROR;
    /* call destructor functions */
    m = (*plugin_db);
    retval = CSOUND_SUCCESS;
    while (m != NULL) {
      if (m->DestFunc != NULL) {
        i = m->DestFunc(csound);
        if (i != 0) {
          retval = CSOUND_ERROR;
          csound->Message(csound, Str("Error de-initialising module '%s'"),
                                  m->name);
          if (m->ErrCodeToStr != NULL)
            csound->Message(csound, ": %s", Str(m->ErrCodeToStr(i)));
          csound->Message(csound, "\n");
        }
      }
      m = m->nxt;
    }
    /* unload modules */
    destroy_module_database(csound);
    /* return with error code */
    return retval;
}

