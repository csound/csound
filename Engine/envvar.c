/*
    envvar.c:

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

#include "csoundCore.h"
#include <ctype.h>
#include <math.h>
#include "envvar.h"

#if defined(mac_classic) && defined(__MWERKS__)
#include <unix.h>
#endif

/* list of environment variables used by Csound */

static const char *envVar_list[] = {
    "CSNOSTOP",
    "CSOUNDRC",
    "CSSTRNGS",
    "CS_LANG",
    "HOME",
    "INCDIR",
    "OPCODEDIR",
    "OPCODEDIR64",
    "SADIR",
    "SFDIR",
    "SFOUTYP",
    "SNAPDIR",
    "SSDIR",
    NULL
};

/* from namedins.c */
extern const unsigned char strhash_tabl_8[256];

#define ENV_DB          (((ENVIRON*) csound)->envVarDB)

#if defined MSVC
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY,_S_IWRITE
#elif defined(mac_classic) || defined(SYMANTEC) || defined(WIN32)
#define RD_OPTS  O_RDONLY | O_BINARY
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#elif defined DOSGCC
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#else
#ifndef O_BINARY
# define O_BINARY (0)
#endif
#define RD_OPTS  O_RDONLY | O_BINARY, 0
#define WR_OPTS  O_TRUNC | O_CREAT | O_WRONLY | O_BINARY, 0644
#endif

typedef struct envVarEntry_s {
    struct envVarEntry_s
            *nxt;               /* pointer to next link in chain        */
    char    *name;              /* name of environment variable         */
    char    *value;             /* value of environment variable        */
} envVarEntry_t;

typedef struct searchPathCacheEntry_s {
    char    *name;
    struct searchPathCacheEntry_s   *nxt;
    char    *lst[1];
} searchPathCacheEntry_t;

typedef struct nameChain_s {
    struct nameChain_s  *nxt;
    char    s[1];
} nameChain_t;

typedef struct CSFILE_ {
    struct CSFILE_  *nxt;
    struct CSFILE_  *prv;
    int             type;
    int             fd;
    FILE            *f;
    SNDFILE         *sf;
    char            fullName[1];
} CSFILE;

static inline unsigned char name_hash(const char *s)
{
    unsigned char *c = (unsigned char*) &(s[0]);
    unsigned char h = (unsigned char) 0;
    for ( ; *c != (unsigned char) 0; c++)
      h = strhash_tabl_8[*c ^ h];
    return h;
}

static inline int sCmp(const char *x, const char *y)
{
    int tmp = 0;
    while (x[tmp] == y[tmp] && x[tmp] != (char) 0)
      tmp++;
    return (x[tmp] != y[tmp]);
}

static inline envVarEntry_t **getEnvVarChain(void *csound, const char *name)
{
    unsigned char h;
    /* check for trivial cases */
    if (ENV_DB == NULL || name == NULL || name[0] == '\0')
      return NULL;
    /* calculate hash value */
    h = name_hash(name);
    /* return with pointer from table */
    return &(((envVarEntry_t**) ENV_DB)[(int) h]);
}

static int is_valid_envvar_name(const char *name)
{
    char *s;

    if (name == NULL || name[0] == '\0')
      return 0;
    s = (char*) &(name[0]);
    if (!(isalpha(*s) || *s == '_'))
      return 0;
    while (*(++s) != '\0') {
      if (!(isalpha(*s) || isdigit(*s) || *s == '_'))
        return 0;
    }
    return 1;
}

/**
 * Get pointer to value of environment variable 'name'.
 * Return value is NULL if the variable is not set.
 */

PUBLIC char *csoundGetEnv(void *csound, const char *name)
{
    envVarEntry_t **pp, *p;

    if (csound == NULL)
      return NULL;
    pp = getEnvVarChain(csound, name);
    if (pp == NULL)
      return NULL;
    p = *pp;
    while (p != NULL && sCmp(p->name, name) != 0)
      p = p->nxt;
    if (p == NULL)
      return NULL;
    return p->value;
}

/**
 * Set environment variable 'name' to 'value'.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
 * if the environment variable could not be set for some reason.
 */

int csoundSetEnv(void *csound, const char *name, const char *value)
{
    searchPathCacheEntry_t  *ep, *nxt;
    envVarEntry_t           **pp, *p;
    char                    *s1, *s2;

    /* check for valid parameters */
    if (csound == NULL || !is_valid_envvar_name(name))
      return CSOUND_ERROR;
    pp = getEnvVarChain(csound, name);
    if (pp == NULL)
      return CSOUND_ERROR;
    /* invalidate search path cache */
    ep = (searchPathCacheEntry_t*) ((ENVIRON*) csound)->searchPathCache;
    while (ep != NULL) {
      nxt = ep->nxt;
      mfree(csound, ep);
      ep = nxt;
    }
    ((ENVIRON*) csound)->searchPathCache = NULL;
    p = *pp;
    s1 = (char*) name;
    s2 = NULL;
    /* copy value */
    if (value != NULL) {
      s2 = (char*) mmalloc(csound, strlen(value) + 1);
      strcpy(s2, value);
    }
    /* is this variable already defined ? */
    while (p != NULL && sCmp(p->name, name) != 0)
      p = p->nxt;
    if (p != NULL) {
      /* yes, only need to replace value */
      if (p->value != NULL)
        mfree(csound, p->value);
      p->value = s2;
    }
    else {
      /* no, need to allocate new entry, and copy name too */
      p = (envVarEntry_t*) mmalloc(csound, sizeof(envVarEntry_t));
      s1 = (char*) mmalloc(csound, (size_t) strlen(name) + (size_t) 1);
      strcpy(s1, name);
      /* store pointers to name and value, and link into chain */
      p->nxt = *pp;
      p->name = s1;
      p->value = s2;
      *pp = p;
    }
    /* print debugging info if requested */
    if (((ENVIRON*) csound)->oparms->odebug) {
      ((ENVIRON*) csound)->Message(csound, Str("Environment variable '%s' "
                                               "has been set to "), name);
      if (value == NULL)
        ((ENVIRON*) csound)->Message(csound, "NULL\n");
      else
        ((ENVIRON*) csound)->Message(csound, "'%s'\n", s2);
    }
    /* report success */
    return CSOUND_SUCCESS;
}

/**
 * Append 'value' to environment variable 'name', using ';' as
 * separator character.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
 * if the environment variable could not be set for some reason.
 */

int csoundAppendEnv(void *csound, const char *name, const char *value)
{
    char  *oldval, *newval;
    int   retval;

    /* check for valid parameters */
    if (csound == NULL || !is_valid_envvar_name(name))
      return CSOUND_ERROR;
    /* get original value of variable */
    oldval = csoundGetEnv(csound, name);
    if (oldval == NULL)
      return csoundSetEnv(csound, name, value);
    if (value == NULL || value[0] == '\0')
      return CSOUND_SUCCESS;
    /* allocate new value (+ 2 bytes for ';' and null character) */
    newval = (char*) mmalloc(csound, (size_t) strlen(oldval)
                                     + (size_t) strlen(value) + (size_t) 2);
    /* append to old value */
    strcpy(newval, oldval);
    strcat(newval, ";");
    strcat(newval, value);
    /* set variable */
    retval = csoundSetEnv(csound, name, newval);
    mfree(csound, newval);
    /* return with error code */
    return retval;
}

/**
 * Initialise environment variable database, and copy system
 * environment variables.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
 * CSOUND_MEMORY in case of an error.
 */

int csoundInitEnv(void *csound)
{
    int i, retval;
    /* check if already initialised */
    if (ENV_DB != NULL)
      return CSOUND_SUCCESS;
    /* allocate table */
    ENV_DB = (void*) mmalloc(csound, sizeof(envVarEntry_t*) * (size_t) 256);
    for (i = 0; i < 256; i++)
      ((envVarEntry_t**) ENV_DB)[i] = (envVarEntry_t*) NULL;
    /* copy standard Csound environment variables */
    i = -1;
    while (envVar_list[++i] != NULL) {
      retval = csoundSetEnv(csound, envVar_list[i], getenv(envVar_list[i]));
      if (retval != CSOUND_SUCCESS)
        return retval;
    }
    /* done */
    return CSOUND_SUCCESS;
}

/**
 * Parse 's' as an assignment to environment variable, in the format
 * "NAME=VALUE" for replacing the previous value, or "NAME+=VALUE"
 * for appending.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
 * CSOUND_MEMORY in case of an error.
 */

int csoundParseEnv(void *csound, const char *s)
{
    char  *name, *value, msg[256];
    int   append_mode, retval;

    /* copy string constant */
    name = (char*) mmalloc(csound, (size_t) strlen(s) + (size_t) 1);
    strcpy(name, s);
    /* check assignment format */
    value = strchr(name, '=');
    append_mode = 0;
    if (value == NULL || value == name) {
      sprintf(msg, " *** invalid format for --env\n");
      retval = CSOUND_ERROR;
      goto err_return;
    }
    *(value++) = '\0';
    if (*(value - 2) == '+') {
      append_mode = 1;
      *(value - 2) = '\0';
    }
    if (!is_valid_envvar_name(name)) {
      sprintf(msg, " *** invalid environment variable name\n");
      retval = CSOUND_ERROR;
      goto err_return;
    }
    /* set variable */
    if (!append_mode)
      retval = csoundSetEnv(csound, name, value);
    else
      retval = csoundAppendEnv(csound, name, value);
    if (retval == CSOUND_MEMORY)
      sprintf(msg, " *** memory allocation failure\n");
    else
      sprintf(msg, " *** error setting environment variable\n");

 err_return:
    if (retval != CSOUND_SUCCESS)
      ((ENVIRON*) csound)->Message(csound, Str(msg));
    if (name != NULL)
      mfree(csound, name);
    return retval;
}

char **csoundGetSearchPathFromEnv(ENVIRON *csound, const char *envList)
{
    searchPathCacheEntry_t  *p;
    nameChain_t             *env_lst = NULL, *path_lst = NULL, *tmp, *prv, *nxt;
    char                    *s;
    int                     i, j, k, len, pathCnt = 0, totLen = 0;

    /* check if the specified environment variable list was already parsed */
    p = (searchPathCacheEntry_t*) csound->searchPathCache;
    while (p != NULL) {
      if (sCmp(p->name, envList) == 0)
        return (&(p->lst[0]));
      p = p->nxt;
    }
    /* not found, need to create new entry */
    len = (int) strlen(envList);
    /* split environment variable list to tokens */
    for (i = j = 0; i <= len; i++) {
      if (envList[i] == ';' || envList[i] == '\0') {
        if (i > j) {
          tmp = (nameChain_t*) mmalloc(csound, sizeof(nameChain_t) + (i - j));
          for (k = 0; j < i; j++, k++)
            tmp->s[k] = envList[j];
          tmp->s[k] = '\0';
          tmp->nxt = NULL;
          if (env_lst != NULL) {
            /* search for duplicate entry */
            prv = nxt = env_lst;
            do {
              if (sCmp(env_lst->s, tmp->s) == 0)
                break;
              prv = nxt;
            } while ((nxt = prv->nxt) != NULL);
            if (nxt == NULL)
              prv->nxt = tmp;
            else
              mfree(csound, tmp);       /* and remove if there is any */
          }
          else
            env_lst = tmp;
        }
        j = i + 1;
      }
    }
    /* expand environment variables to path list */
    while (env_lst != NULL) {
      nxt = env_lst->nxt;
      s = (char*) csoundGetEnv(csound, env_lst->s);
      mfree(csound, env_lst);
      env_lst = nxt;
      if (s != NULL && s[0] != '\0')
        len = (int) strlen(s);
      else
        len = -1;
      for (i = j = 0; i <= len; i++) {
        if (s[i] == ';' || s[i] == '\0') {
          if (i > j) {
            tmp = (nameChain_t*) mmalloc(csound, sizeof(nameChain_t)
                                                 + (i - j) + 1);
            /* copy with converting pathname delimiters */
            for (k = 0; j < i; j++, k++)
              tmp->s[k] = (s[j] == '/' || s[j] == '\\' ? DIRSEP : s[j]);
            while (tmp->s[--k] == DIRSEP);
            tmp->s[++k] = DIRSEP;
            tmp->s[++k] = '\0';
            tmp->nxt = path_lst;
            path_lst = tmp;
            /* search for duplicate entry */
            for (prv = tmp; (tmp = tmp->nxt) != NULL; prv = tmp)
              if (sCmp(path_lst->s, tmp->s) == 0)
                break;
            if (tmp != NULL) {
              /* and remove if there is any */
              prv->nxt = tmp->nxt;
              mfree(csound, tmp);
            }
            else {
              /* calculate storage requirement */
              pathCnt++; totLen += (k + 1);
            }
          }
          j = i + 1;
        }
      }
    }
    totLen += ((int) strlen(envList) + 1);
    /* create path cache entry */
    p = (searchPathCacheEntry_t*) mmalloc(csound, sizeof(searchPathCacheEntry_t)
                                                  + sizeof(char*) * pathCnt
                                                  + sizeof(char) * totLen);
    s = (char*) &(p->lst[pathCnt + 1]);
    p->name = s;
    strcpy(p->name, envList);
    s += ((int) strlen(envList) + 1);
    p->nxt = (searchPathCacheEntry_t*) csound->searchPathCache;
    if (csound->oparms->odebug)
      csound->DebugMsg(csound, Str("Creating search path cache for '%s':"),
                               p->name);
    for (i = 0; (i < pathCnt) && (path_lst != NULL); i++) {
      p->lst[i] = s;
      strcpy(s, path_lst->s);
      s += ((int) strlen(path_lst->s) + 1);
      nxt = path_lst->nxt;
      mfree(csound, path_lst);
      path_lst = nxt;
      if (csound->oparms->odebug)
        csound->DebugMsg(csound, "   %2d: \"%s\"", (i + 1), p->lst[i]);
    }
    p->lst[i] = NULL;
    /* link into database */
    csound->searchPathCache = (void*) p;
    /* return with pathname list */
    return (&(p->lst[0]));
}

/* check if file name is valid, and copy with converting pathname delimiters */

static inline char *convert_name(void *csound, const char *filename)
{
    char  *name;
    int   i = 0;

    if (filename == NULL || filename[0] == '\0')
      return NULL;
    name = (char*) mmalloc(csound, (size_t) strlen(filename) + (size_t) 1);
    do {
      if (filename[i] != '/' && filename[i] != '\\')
        name[i] = filename[i];
      else
        name[i] = DIRSEP;
    } while (filename[i++] != '\0');
    if (name[i - 2] == DIRSEP
#ifdef WIN32
        || (isalpha(name[0]) && name[1] == ':' && name[2] == '\0')
#endif
        ) {
      mfree(csound, name);
      return NULL;
    }
    return name;
}

static inline int is_name_fullpath(const char *name)
{
#ifndef mac_classic
    if (name[0] == DIRSEP ||
        (name[0] == '.' && (name[1] == DIRSEP ||
                            (name[1] == '.' && name[2] == DIRSEP))))
      return 1;
    return 0;
#else
    /* MacOS relative paths begin with DIRSEP */
    /* Full paths contain DIRSEP but do not start with it */
    if (name[0] == DIRSEP || strchr(name, DIRSEP) == NULL)
      return 0;
    return 1;
#endif
}

static FILE *csoundFindFile_Std(void *csound, char **fullName,
                                const char *filename, const char *mode,
                                const char *envList)
{
    FILE  *f;
    char  *name, *name2, **searchPath;
    int   len;

    *fullName = NULL;
    if ((name = convert_name(csound, filename)) == NULL)
      return (FILE*) NULL;
    if (mode[0] != 'w') {
      /* read: try the specified name first */
      f = fopen(name, mode);
      if (f != NULL) {
        *fullName = name;
        return f;
      }
      /* if full path, and not found: */
      if (is_name_fullpath(name)) {
        mfree(csound, name);
        return (FILE*) NULL;
      }
    }
    else if (is_name_fullpath(name)) {
      /* if write and full path: */
      f = fopen(name, mode);
      if (f != NULL)
        *fullName = name;
      else
        mfree(csound, name);
      return f;
    }
    if (envList == NULL || envList[0] == '\0') {
      mfree(csound, name);
      return (FILE*) NULL;
    }
    /* search paths defined by environment variable list */
    searchPath = csoundGetSearchPathFromEnv((ENVIRON*) csound, envList);
    if (searchPath != NULL) {
      len = (int) strlen(name) + 1;
      while (*searchPath != NULL) {
        name2 = mmalloc(csound, (size_t) strlen(*searchPath) + (size_t) len);
        strcpy(name2, *searchPath);
        strcat(name2, name);
        f = fopen(name2, mode);
        if (f != NULL) {
          mfree(csound, name);
          *fullName = name2;
          return f;
        }
        mfree(csound, name2);
        searchPath++;
      }
    }
    /* if write mode, try current directory last */
    if (mode[0] == 'w') {
      f = fopen(name, mode);
      if (f != NULL) {
        *fullName = name;
        return f;
      }
    }
    /* not found */
    mfree(csound, name);
    return (FILE*) NULL;
}

static int csoundFindFile_Fd(void *csound, char **fullName,
                             const char *filename, int write_mode,
                             const char *envList)
{
    char  *name, *name2, **searchPath;
    int   len, fd;

    *fullName = NULL;
    if ((name = convert_name(csound, filename)) == NULL)
      return -1;
    if (!write_mode) {
      /* read: try the specified name first */
      fd = open(name, RD_OPTS);
      if (fd >= 0) {
        *fullName = name;
        return fd;
      }
      /* if full path, and not found: */
      if (is_name_fullpath(name)) {
        mfree(csound, name);
        return -1;
      }
    }
    else if (is_name_fullpath(name)) {
      /* if write and full path: */
      fd = open(name, WR_OPTS);
      if (fd >= 0)
        *fullName = name;
      else
        mfree(csound, name);
      return fd;
    }
    if (envList == NULL || envList[0] == '\0') {
      mfree(csound, name);
      return -1;
    }
    /* search paths defined by environment variable list */
    searchPath = csoundGetSearchPathFromEnv((ENVIRON*) csound, envList);
    if (searchPath != NULL) {
      len = (int) strlen(name) + 1;
      while (*searchPath != NULL) {
        name2 = mmalloc(csound, (size_t) strlen(*searchPath) + (size_t) len);
        strcpy(name2, *searchPath);
        strcat(name2, name);
        if (!write_mode)
          fd = open(name2, RD_OPTS);
        else
          fd = open(name2, WR_OPTS);
        if (fd >= 0) {
          mfree(csound, name);
          *fullName = name2;
          return fd;
        }
        mfree(csound, name2);
        searchPath++;
      }
    }
    /* if write mode, try current directory last */
    if (write_mode == 'w') {
      fd = open(name, WR_OPTS);
      if (fd >= 0) {
        *fullName = name;
        return fd;
      }
    }
    /* not found */
    mfree(csound, name);
    return -1;
}

/**
 * Search for input file 'filename'.
 * If the file name specifies full path (it begins with '.', the pathname
 * delimiter character, or a drive letter and ':' on Windows), that exact
 * file name is tried without searching.
 * Otherwise, the file is searched relative to the current directory first,
 * and if it is still not found, a pathname list that is created the
 * following way is searched:
 *   1. if envList is NULL or empty, no directories are searched
 *   2. envList is parsed as a ';' separated list of environment variable
 *      names, and all environment variables are expanded and expected to
 *      contain a ';' separated list of directory names
 *   2. all directories in the resulting pathname list are searched, starting
 *      from the last and towards the first one, and the directory where the
 *      file is found first will be used
 * The function returns a pointer to the full name of the file if it is
 * found, and NULL if the file could not be found in any of the search paths,
 * or an error has occured. The caller is responsible for freeing the memory
 * pointed to by the return value, by calling mfree().
 */
PUBLIC char *csoundFindInputFile(void *csound,
                                 const char *filename, const char *envList)
{
    char  *name_found;
    int   fd;

    if (csound == NULL)
      return NULL;
    fd = csoundFindFile_Fd(csound, &name_found, filename, 0, envList);
    if (fd >= 0)
      close(fd);
    return name_found;
}

/**
 * Search for a location to write file 'filename'.
 * If the file name specifies full path (it begins with '.', the pathname
 * delimiter character, or a drive letter and ':' on Windows), that exact
 * file name is tried without searching.
 * Otherwise, a pathname list that is created the following way is searched:
 *   1. if envList is NULL or empty, no directories are searched
 *   2. envList is parsed as a ';' separated list of environment variable
 *      names, and all environment variables are expanded and expected to
 *      contain a ';' separated list of directory names
 *   2. all directories in the resulting pathname list are searched, starting
 *      from the last and towards the first one, and the directory that is
 *      found first where the file can be written to will be used
 * Finally, if the file cannot be written to any of the directories in the
 * search paths, writing relative to the current directory is tried.
 * The function returns a pointer to the full name of the file if a location
 * suitable for writing the file is found, and NULL if the file cannot not be
 * written anywhere in the search paths, or an error has occured.
 * The caller is responsible for freeing the memory pointed to by the return
 * value, by calling mfree().
 */
PUBLIC char *csoundFindOutputFile(void *csound,
                                  const char *filename, const char *envList)
{
    char  *name_found;
    int   fd;

    if (csound == NULL)
      return NULL;
    fd = csoundFindFile_Fd(csound, &name_found, filename, 1, envList);
    if (fd >= 0) {
      close(fd);
      remove(name_found);
    }
    return name_found;
}

/**
 * Open a file and return handle.
 *
 * void *csound:
 *   Csound instance pointer
 * void *fd:
 *   pointer a variable of type int, FILE*, or SNDFILE*, depending on 'type',
 *   for storing handle to be passed to file read/write functions
 * int type:
 *   file type, one of the following:
 *     CSFILE_FD_R:     read file using low level interface (open())
 *     CSFILE_FD_W:     write file using low level interface (open())
 *     CSFILE_STD:      use ANSI C interface (fopen())
 *     CSFILE_SND_R:    read sound file
 *     CSFILE_SND_W:    write sound file
 * const char *name:
 *   file name
 * void *param:
 *   parameters, depending on type:
 *     CSFILE_FD_R:     unused (should be NULL)
 *     CSFILE_FD_W:     unused (should be NULL)
 *     CSFILE_STD:      mode parameter (of type char*) to be passed to fopen()
 *     CSFILE_SND_R:    SF_INFO* parameter for sf_open(), with defaults for
 *                      raw file; the actual format paramaters of the opened
 *                      file will be stored in this structure
 *     CSFILE_SND_W:    SF_INFO* parameter for sf_open(), output file format
 * const char *env:
 *   list of environment variables for search path (see csoundFindInputFile()
 *   for details); if NULL, the specified name is used as it is, without any
 *   conversion or search.
 * return value:
 *   opaque handle to the opened file, for use with csoundGetFileName() or
 *   csoundFileClose(), or storing in FDCH.fd.
 *   On failure, NULL is returned.
 */

PUBLIC void *csoundFileOpen(void *csound, void *fd, int type,
                            const char *name, void *param, const char *env)
{
    CSFILE  *p = NULL;
    char    *fullName = NULL;
    SF_INFO sfinfo;
    int     tmp_fd = -1, nbytes = (int) sizeof(CSFILE);

    /* check file type */
    switch (type) {
      case CSFILE_FD_R:
      case CSFILE_FD_W:
        *((int*) fd) = -1;
        break;
      case CSFILE_STD:
        *((FILE**) fd) = (FILE*) NULL;
        break;
      case CSFILE_SND_R:
      case CSFILE_SND_W:
        *((SNDFILE**) fd) = (SNDFILE*) NULL;
        break;
      default:
        csoundMessageS(csound, CSOUNDMSG_ERROR,
                               Str("internal error: csoundFileOpen(): "
                                   "invalid type: %d\n"), type);
        return NULL;
    }
    /* get full name and open file */
    if (env == NULL) {
      fullName = (char*) name;
      if (type == CSFILE_STD) {
        *((FILE**) fd) = fopen(fullName, (char*) param);
        if (*((FILE**) fd) == NULL)
          return NULL;
      }
      else {
        if (type == CSFILE_SND_R || type == CSFILE_FD_R)
          tmp_fd = open(fullName, RD_OPTS);
        else
          tmp_fd = open(fullName, WR_OPTS);
        if (tmp_fd < 0)
          return NULL;
      }
    }
    else {
      if (type == CSFILE_STD) {
        *((FILE**) fd) = csoundFindFile_Std(csound, &fullName, name,
                                                    (char*) param, env);
        if (*((FILE**) fd) == NULL)
          return NULL;
      }
      else {
        if (type == CSFILE_SND_R || type == CSFILE_FD_R)
          tmp_fd = csoundFindFile_Fd(csound, &fullName, name, 0, env);
        else
          tmp_fd = csoundFindFile_Fd(csound, &fullName, name, 1, env);
        if (tmp_fd < 0)
          return NULL;
      }
    }
    nbytes += (int) strlen(fullName);
    /* allocate file structure */
    p = (CSFILE*) malloc((size_t) nbytes);
    if (p == NULL) {
      if (env != NULL)
        mfree(csound, fullName);
      return NULL;
    }
    p->nxt = (CSFILE*) ((ENVIRON*) csound)->open_files;
    p->prv = (CSFILE*) NULL;
    p->type = type;
    p->fd = tmp_fd;
    p->f = (FILE*) NULL;
    p->sf = (SNDFILE*) NULL;
    strcpy(&(p->fullName[0]), fullName);
    if (env != NULL)
      mfree(csound, fullName);
    /* if sound file, re-open file descriptor with libsndfile */
    switch (type) {
      case CSFILE_STD:                          /* stdio */
        p->f = *((FILE**) fd);
        break;
      case CSFILE_SND_R:                        /* sound file read */
        memset(&sfinfo, 0, sizeof(SF_INFO));
        p->sf = sf_open_fd(tmp_fd, SFM_READ, &sfinfo, 0);
        if (p->sf == (SNDFILE*) NULL) {
          /* open failed: maybe raw file ? rewind and try again */
          if (lseek(tmp_fd, (off_t) 0, SEEK_SET) == (off_t) 0)
            p->sf = sf_open_fd(tmp_fd, SFM_READ, (SF_INFO*) param, 0);
          if (p->sf == (SNDFILE*) NULL) {
            close(tmp_fd);
            free(p);
            return NULL;
          }
        }
        else
          memcpy((SF_INFO*) param, &sfinfo, sizeof(SF_INFO));
        *((SNDFILE**) fd) = p->sf;
        break;
      case CSFILE_SND_W:                        /* sound file write */
        p->sf = sf_open_fd(tmp_fd, SFM_WRITE, (SF_INFO*) param, 0);
        if (p->sf == (SNDFILE*) NULL) {
          close(tmp_fd);
          free(p);
          return NULL;
        }
        sf_command(p->sf, SFC_SET_CLIPPING, NULL, SF_TRUE);
        *((SNDFILE**) fd) = p->sf;
        break;
    }
    /* link into chain of open files */
    if (((ENVIRON*) csound)->open_files != NULL)
      ((CSFILE*) ((ENVIRON*) csound)->open_files)->prv = p;
    ((ENVIRON*) csound)->open_files = (void*) p;
    /* return with opaque file handle */
    return (void*) p;
}

/**
 * Allocate a file handle for an existing file already opened with open(),
 * fopen(), or sf_open(), for later use with csoundFileClose() or
 * csoundGetFileName(), or storing in an FDCH structure.
 * Files registered this way (or opened with csoundFileOpen()) are also
 * automatically closed by csoundReset().
 * Parameters and return value are similar to csoundFileOpen(), except
 * fullName is the name that will be returned by a later call to
 * csoundGetFileName().
 */

PUBLIC void *csoundCreateFileHandle(void *csound, void *fd, int type,
                                                  const char *fullName)
{
    CSFILE  *p = NULL;
    int     nbytes = (int) sizeof(CSFILE);

    /* name should not be empty */
    if (fullName == NULL || fullName[0] == '\0')
      return NULL;
    nbytes += (int) strlen(fullName);
    /* allocate file structure */
    p = (CSFILE*) malloc((size_t) nbytes);
    if (p == NULL)
      return NULL;
    p->nxt = (CSFILE*) ((ENVIRON*) csound)->open_files;
    p->prv = (CSFILE*) NULL;
    p->type = type;
    p->fd = -1;
    p->f = (FILE*) NULL;
    p->sf = (SNDFILE*) NULL;
    strcpy(&(p->fullName[0]), fullName);
    /* open file */
    switch (type) {
      case CSFILE_FD_R:
      case CSFILE_FD_W:
        p->fd = *((int*) fd);
        break;
      case CSFILE_STD:
        p->f = *((FILE**) fd);
        break;
      case CSFILE_SND_R:
      case CSFILE_SND_W:
        p->sf = *((SNDFILE**) fd);
        break;
      default:
        csoundMessageS(csound, CSOUNDMSG_ERROR,
                               Str("internal error: csoundCreateFileHandle(): "
                                   "invalid type: %d\n"), type);
        free(p);
        return NULL;
    }
    /* link into chain of open files */
    if (((ENVIRON*) csound)->open_files != NULL)
      ((CSFILE*) ((ENVIRON*) csound)->open_files)->prv = p;
    ((ENVIRON*) csound)->open_files = (void*) p;
    /* return with opaque file handle */
    return (void*) p;
}

/**
 * Get the full name of a file previously opened with csoundFileOpen().
 */

PUBLIC char *csoundGetFileName(void *fd)
{
    return &(((CSFILE*) fd)->fullName[0]);
}

/**
 * Close a file previously opened with csoundFileOpen().
 */

PUBLIC int csoundFileClose(void *csound, void *fd)
{
    CSFILE  *p = (CSFILE*) fd;
    int     retval = -1;

    /* close file */
    switch (p->type) {
      case CSFILE_FD_R:
      case CSFILE_FD_W:
        retval = close(p->fd);
        break;
      case CSFILE_STD:
        retval = fclose(p->f);
        break;
      case CSFILE_SND_R:
      case CSFILE_SND_W:
        retval = sf_close(p->sf);
        if (p->fd >= 0)
          retval |= close(p->fd);
        break;
    }
    /* unlink from chain of open files */
    if (p->prv == NULL)
      ((ENVIRON*) csound)->open_files = (void*) p->nxt;
    else
      p->prv->nxt = p->nxt;
    if (p->nxt != NULL)
      p->nxt->prv = p->prv;
    /* free allocated memory */
    free(fd);
    /* return with error value */
    return retval;
}

/* Close all open files; called by csoundReset(). */

void close_all_files(void *csound)
{
    while (((ENVIRON*) csound)->open_files != NULL)
      csoundFileClose(csound, ((ENVIRON*) csound)->open_files);
}

