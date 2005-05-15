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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "csoundCore.h"
#include "csound.h"
#include "envvar.h"

/* from namedins.c */
extern const unsigned char strhash_tabl_8[256];

#define name_hash(x,y)  (strhash_tabl_8[(unsigned char) x ^ (unsigned char) y])
#define ENV_DB          (((ENVIRON*) csound)->envVarDB)

typedef struct envVarEntry_s {
    struct envVarEntry_s
            *nxt;               /* pointer to next link in chain        */
    char    *name;              /* name of environment variable         */
    char    *value;             /* value of environment variable        */
} envVarEntry_t;

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

static envVarEntry_t **getEnvVarChain(void *csound, const char *name)
{
    unsigned char *c, h;
    /* check for trivial cases */
    if (ENV_DB == NULL || name == NULL || name[0] == '\0')
      return NULL;
    /* calculate hash value */
    c = (unsigned char*) name - 1;
    h = (unsigned char) 0;
    while (*++c) h = name_hash(h, *c);
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

/* Is 's' also found later in 'base' (s >= base) ? Non-zero: yes. */

static int check_for_duplicate(const char *base, const char *s)
{
    int i, j, d;

    if (*s == '\0' || *s == ';')
      return 1;
    i = (int) (s - base);
    while (base[i] != ';' && base[i] != '\0')
      i++;
    while (base[i] != '\0') {
      j = 0;
      d = 1;
      i++;
      while (base[i] != ';' && base[i] != '\0') {
        if (s[j] != base[i])
          d = 0;
        i++;
        if (s[j] != ';' && s[j] != '\0')
          j++;
      }
      if (d && (s[j] == ';' || s[j] == '\0'))
        return 1;
    }
    return 0;
}

/* Make a copy of ';' separated list, remove unneeded ';' characters */
/* and duplicate list entries. */

static char *fix_env_value(void *csound, const char *oldval)
{
    char  *s;
    int   i, new_len, cnt, n;

    if (oldval == NULL)
      return NULL;      /* trivial case */
    /* calculate new length */
    new_len = cnt = n = 0;
    i = 0;
    while (1) {
      if (oldval[i] == ';' || oldval[i] == '\0') {
        if (cnt && !check_for_duplicate(oldval, &(oldval[n]))) {
          new_len += (cnt + 1); /* put token and ';' character */
        }
        if (oldval[i] == '\0')  /* end of list */
          break;
        n = i + 1;              /* start position of next token */
        cnt = 0;
      }
      else
        cnt++;                  /* count token length */
      i++;
    }
    if (new_len < 1)            /* if empty, */
      new_len = 1;              /* still need null character */
    /* allocate memory */
    s = (char*) mmalloc(csound, (size_t) new_len);
    if (s == NULL)
      return NULL;
    /* create new list */
    new_len = cnt = n = 0;
    i = 0;
    while (1) {
      if (oldval[i] == ';' || oldval[i] == '\0') {
        if (cnt && !check_for_duplicate(oldval, &(oldval[n]))) {
          do {                  /* copy token */
            s[new_len++] = oldval[n++];
          } while (--cnt);
          s[new_len++] = ';';   /* put ';' character at end of token */
        }
        if (oldval[i] == '\0')  /* end of list */
          break;
        n = i + 1;              /* start position of next token */
        cnt = 0;
      }
      else
        cnt++;                  /* count length */
      i++;
    }
    /* terminate list with null character */
    s[(new_len > 0 ? (new_len - 1) : 0)] = '\0';
    /* return pointer to new list */
    return s;
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
    while (p != NULL && strcmp(p->name, name) != 0)
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
    envVarEntry_t **pp, *p;
    char          *s1, *s2;

    /* check for valid parameters */
    if (csound == NULL || !is_valid_envvar_name(name))
      return CSOUND_ERROR;
    pp = getEnvVarChain(csound, name);
    if (pp == NULL)
      return CSOUND_ERROR;
    p = *pp;
    s1 = (char*) name;
    s2 = NULL;
    /* copy value */
    if (value != NULL) {
      s2 = fix_env_value(csound, value);
      if (s2 == NULL)
        return CSOUND_MEMORY;
    }
    /* is this variable already defined ? */
    while (p != NULL && strcmp(p->name, name) != 0)
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
      if (p == NULL)
        return CSOUND_MEMORY;
      s1 = (char*) mmalloc(csound, (size_t) strlen(name) + (size_t) 1);
      if (s1 == NULL)
        return CSOUND_MEMORY;
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
    if (newval == NULL)
      return CSOUND_MEMORY;
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
    if (ENV_DB == NULL)
      return CSOUND_MEMORY;
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
    if (name == NULL) {
      sprintf(msg, " *** memory allocation failure\n");
      retval = CSOUND_MEMORY;
      goto err_return;
    }
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

/* Does 'filename' exist and is it a readable file ? Non-zero: yes. */
/* Also attempts to detect directories. */

static int try_file_open(const char *filename)
{
    FILE *tmp;
    char *buf;
    int  n;

    if (filename == NULL || filename[0] == '\0')    /* trivial case */
      return 0;
    n = (int) strlen(filename) - 1;
    if (filename[n] == DIRSEP)                      /* directory */
      return 0;
#ifdef WIN32
    if (filename[n] == ':')                         /* drive letter ? */
      return 0;
#endif
    tmp = fopen(filename, "rb");
    if (tmp == NULL)
      return 0;                                     /* not found */
    /* found file, but may still be a directory */
    fclose(tmp);
#ifndef WIN32
    buf = (char*) malloc((size_t) n + (size_t) 3);
    if (buf == NULL)
      return 0;     /* not really the best solution */
    strcpy(buf, filename);
    buf[n + 1] = DIRSEP;
    buf[n + 2] = '\0';
    /* if open still does not fail then it is a directory */
    tmp = fopen(buf, "rb");
    free(buf);
    if (tmp == NULL)
      return 1;     /* OK, really a file */
    fclose(tmp);
    /* found something, but it is apparently a directory, not a file */
    return 0;
#else
    return 1;
#endif
}

/* Does file name have full path ? 0: no, 1: yes and file is found, */
/* -1: name is invalid or it has full path but file cannot be found */

static int is_name_full_path(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
      return -1;
#ifdef WIN32
    if (!(filename[0] == '.' || filename[0] == DIRSEP ||
          ((int) strlen(filename) >= 2 && isalpha(filename[0]) &&
           filename[1] == ':')))
      return 0;
#elif defined(mac_classic)
    /* MacOS relative paths begin with DIRSEP */
    /* Full paths contain DIRSEP but do not start with it */
    if (filename[0] == DIRSEP)  return 0;
    if (! strchr(filename, DIRSEP))  return 0;
#else
    if (!(filename[0] == '.' || filename[0] == DIRSEP))
      return 0;
#endif
    if (!try_file_open(filename))
      return -1;    /* full path but does not exist */
    return 1;
}

static char *csoundFindInputFile_(void *csound,
                                  const char *filename, const char *envList)
{
    char  *s, *s2, *buf, *name;
    int   retval, i, len, len2;

    /* copy and convert name */
    name = (char*) mmalloc(csound, (size_t) strlen(filename) + (size_t) 1);
    strcpy(name, filename);
    i = -1;
    while (name[++i] != '\0') {
      if (name[i] == '/' || name[i] == '\\')
        name[i] = DIRSEP;
    }
    /* check for name with path */
    retval = is_name_full_path(name);
    if (retval < 0) {
      mfree(csound, name);
      return NULL;              /* invalid name, or full path and not found */
    }
    if (retval > 0)
      return name;              /* full path, found file */
    /* not full path, need to search; try current directory first */
    s = (char*) mmalloc(csound, (size_t) strlen(name) + (size_t) 3);
#ifdef mac_classic
    strcpy(s, name);
#else
    s[0] = '.';
    s[1] = DIRSEP;
    s[2] = '\0';
    strcat(s, name);
#endif
    if (try_file_open(name)) {
      mfree(csound, name);
      return s;                 /* found file in current directory */
    }
    mfree(csound, s);
    /* not in current directory, create list of directories to search */
    if (envList == NULL || envList[0] == '\0') {
      mfree(csound, name);
      return NULL;              /* no environment variables specified */
    }
    s = (char*) mmalloc(csound, (size_t) strlen(envList) + (size_t) 1);
    strcpy(s, envList);
    /* split environment variable list to tokens */
    i = (int) strlen(envList);
    while (--i >= 0) {
      if (s[i] == ';')
        s[i] = '\0';
    }
    /* calculate total length of pathname list */
    i = len = 0;
    while (i < (int) strlen(envList)) {
      if (s[i] == '\0') {
        i++; continue;
      }
      s2 = csoundGetEnv(csound, (char*) s + (int) i);
      i += ((int) strlen((char*) s + (int) i));
      if (s2 != NULL && s2[0] != '\0') {
        len += (int) strlen(s2) + 1;
      }
    }
    if (len <= 0) {
      /* empty list */
      mfree(csound, s);
      mfree(csound, name);
      return NULL;
    }
    /* create pathname list */
    buf = (char*) mmalloc(csound, (size_t) len + (size_t) 1);
    buf[0] = '\0';
    i = len = 0;
    while (i < (int) strlen(envList)) {
      if (s[i] == '\0') {
        i++; continue;
      }
      s2 = csoundGetEnv(csound, (char*) s + (int) i);
      i += ((int) strlen((char*) s + (int) i));
      if (s2 != NULL && s2[0] != '\0') {
        strcat(buf, s2);
        len += (int) strlen(s2);
        buf[len++] = ';';
        buf[len] = '\0';
      }
    }
    mfree(csound, s);   /* environment variable list is no longer needed */
    /* convert pathname delimiters */
    i = -1;
    while (++i < len) {
      if (buf[i] == '/' || buf[i] == '\\')
        buf[i] = DIRSEP;
    }
    /* split pathname list to tokens */
    i = len;
    while (--i >= 0) {
      if (buf[i] == ';') {
        buf[i] = '\0';
        while (i > 0 && buf[i - 1] == DIRSEP)   /* strip any trailing  */
          buf[--i] = '\0';                      /* pathname delimiters */
      }
    }
    /* search file in pathname list */
    i = len;
    while (--i >= 0) {
      if (buf[i] == '\0')
        continue;
      while (i > 0 && buf[i - 1] != '\0')
        i--;
      /* construct file name */
      len2 = (int) strlen((char*) buf + (int) i);
      s2 = (char*) mmalloc(csound, (size_t) len2 + (size_t) strlen(name)
                                   + (size_t) 2);
      strcpy(s2, (char*) buf + (int) i);
      s2[len2] = DIRSEP;
      s2[len2 + 1] = '\0';
      strcat(s2, name);
      if (((ENVIRON*) csound)->oparms->odebug)
        csoundMessage(csound, Str("  Trying file '%s'...\n"), s2);
      if (try_file_open(s2)) {
        /* found file, clean up and return with full name */
        mfree(csound, buf);
        mfree(csound, name);
        return s2;
      }
      mfree(csound, s2);    /* not found in this directory */
    }
    /* could not find file, clean up and report error */
    mfree(csound, buf);
    mfree(csound, name);
    return NULL;
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
    char *name_found;

    if (csound == NULL || filename == NULL || filename[0] == '\0')
      return NULL;
    if (((ENVIRON*) csound)->oparms->odebug) {
      csoundMessage(csound, Str("Searching for input file '%s'"), filename);
      if (envList != NULL && envList[0] != '\0')
        csoundMessage(csound, Str(" in %s"), envList);
      csoundMessage(csound, "\n");
    }
    name_found = csoundFindInputFile_(csound, filename, envList);
    if (((ENVIRON*) csound)->oparms->odebug) {
      if (name_found != NULL)
        csoundMessage(csound, Str("Found '%s'\n"), name_found);
      else
        csoundMessage(csound, Str("Could not find '%s' in any of the "
                                  "search paths\n"), filename);
    }
    return name_found;
}

/* Is it possible to write to 'filename' ? Non-zero: yes. */
/* Also attempts to detect directories. */

static int try_outfile_open(const char *filename)
{
    FILE *tmp;
    int  n;

    if (filename == NULL || filename[0] == '\0')    /* trivial case */
      return 0;
    n = (int) strlen(filename) - 1;
    if (filename[n] == DIRSEP)                      /* directory */
      return 0;
#ifdef WIN32
    if (filename[n] == ':')                         /* drive letter ? */
      return 0;
#endif
    /* try opening read/write */
    tmp = fopen(filename, "r+b");
    if (tmp == NULL) {
      /* not found, try creating new file */
      tmp = fopen(filename, "wb");
      if (tmp == NULL)
        return 0;           /* failed */
      fclose(tmp);
      remove(filename);     /* OK, remove temporarily created file */
      return 1;
    }
    /* found file; it is expected that opening with "r+b" would fail */
    /* on a directory (FIXME: is this true on all platforms ?) */
    fclose(tmp);
    return 1;
}

/* Does file name have full path ? 0: no, 1: yes and file can be written to, */
/* -1: name is invalid or it has full path but cannot write to file */

static int is_outname_full_path(const char *filename)
{
    if (filename == NULL || filename[0] == '\0')
      return -1;
#ifdef WIN32
    if (!(filename[0] == '.' || filename[0] == DIRSEP ||
          ((int) strlen(filename) >= 2 && isalpha(filename[0]) &&
           filename[1] == ':')))
      return 0;
#elif defined(mac_classic)
    /* MacOS relative paths begin with DIRSEP */
    /* Full paths contain DIRSEP but do not start with it */
    if (filename[0] == DIRSEP)  return 0;
    if (! strchr(filename, DIRSEP))  return 0;
#else
    if (!(filename[0] == '.' || filename[0] == DIRSEP))
      return 0;
#endif
    if (!try_outfile_open(filename))
      return -1;    /* full path but cannot write */
    return 1;
}

static char *csoundFindOutputFile_(void *csound,
                                   const char *filename, const char *envList)
{
    char  *s, *s2, *buf, *name;
    int   retval, i, len, len2;

    /* copy and convert name */
    name = (char*) mmalloc(csound, (size_t) strlen(filename) + (size_t) 1);
    strcpy(name, filename);
    i = -1;
    while (name[++i] != '\0') {
      if (name[i] == '/' || name[i] == '\\')
        name[i] = DIRSEP;
    }
    /* check for name with path */
    retval = is_outname_full_path(filename);
    if (retval < 0) {
      mfree(csound, name);
      return NULL;          /* invalid name, or full path but cannot write */
    }
    if (retval > 0)
      return name;          /* full path, found file */
    /* not full path, need to search; create list of directories */
    if (envList == NULL || envList[0] == '\0')
      goto try_current_dir;     /* no environment variables specified */
    s = (char*) mmalloc(csound, (size_t) strlen(envList) + (size_t) 1);
    strcpy(s, envList);
    /* split environment variable list to tokens */
    i = (int) strlen(envList);
    while (--i >= 0) {
      if (s[i] == ';')
        s[i] = '\0';
    }
    /* calculate total length of pathname list */
    i = len = 0;
    while (i < (int) strlen(envList)) {
      if (s[i] == '\0') {
        i++; continue;
      }
      s2 = csoundGetEnv(csound, (char*) s + (int) i);
      i += ((int) strlen((char*) s + (int) i));
      if (s2 != NULL && s2[0] != '\0') {
        len += (int) strlen(s2) + 1;
      }
    }
    if (len <= 0) {
      /* empty list */
      mfree(csound, s);
      goto try_current_dir;
    }
    /* create pathname list */
    buf = (char*) mmalloc(csound, (size_t) len + (size_t) 1);
    buf[0] = '\0';
    i = len = 0;
    while (i < (int) strlen(envList)) {
      if (s[i] == '\0') {
        i++; continue;
      }
      s2 = csoundGetEnv(csound, (char*) s + (int) i);
      i += ((int) strlen((char*) s + (int) i));
      if (s2 != NULL && s2[0] != '\0') {
        strcat(buf, s2);
        len += (int) strlen(s2);
        buf[len++] = ';';
        buf[len] = '\0';
      }
    }
    mfree(csound, s);   /* environment variable list is no longer needed */
    /* convert pathname delimiters */
    i = -1;
    while (++i < len) {
      if (buf[i] == '/' || buf[i] == '\\')
        buf[i] = DIRSEP;
    }
    /* split pathname list to tokens */
    i = len;
    while (--i >= 0) {
      if (buf[i] == ';') {
        buf[i] = '\0';
        while (i > 0 && buf[i - 1] == DIRSEP)   /* strip any trailing  */
          buf[--i] = '\0';                      /* pathname delimiters */
      }
    }
    /* search file in pathname list */
    i = len;
    while (--i >= 0) {
      if (buf[i] == '\0')
        continue;
      while (i > 0 && buf[i - 1] != '\0')
        i--;
      /* construct file name */
      len2 = (int) strlen((char*) buf + (int) i);
      s2 = (char*) mmalloc(csound, (size_t) len2 + (size_t) strlen(name)
                                   + (size_t) 2);
      strcpy(s2, (char*) buf + (int) i);
      s2[len2] = DIRSEP;
      s2[len2 + 1] = '\0';
      strcat(s2, name);
      if (((ENVIRON*) csound)->oparms->odebug)
        csoundMessage(csound, Str("  Trying file '%s'...\n"), s2);
      if (try_outfile_open(s2)) {
        /* found file, clean up and return with full name */
        mfree(csound, buf);
        mfree(csound, name);
        return s2;
      }
      mfree(csound, s2);    /* cannot write to this directory */
    }
    /* cannot write to any of the directories in the search paths */
    mfree(csound, buf);

 try_current_dir:
    /* try current directory if cannot write anywhere else */
    s = (char*) mmalloc(csound, (size_t) strlen(name) + (size_t) 3);
#ifdef mac_classic
    strcpy(s, name);
#else
    s[0] = '.';
    s[1] = DIRSEP;
    s[2] = '\0';
    strcat(s, name);
#endif
    mfree(csound, name);
    if (try_outfile_open(s))
      return s;                 /* can write to current directory */
    /* cannot write file to any of the search paths */
    mfree(csound, s);
    return NULL;
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
    char *name_found;

    if (csound == NULL || filename == NULL || filename[0] == '\0')
      return NULL;
    if (((ENVIRON*) csound)->oparms->odebug) {
      csoundMessage(csound, Str("Searching for output file '%s'"), filename);
      if (envList != NULL && envList[0] != '\0')
        csoundMessage(csound, Str(" in %s"), envList);
      csoundMessage(csound, "\n");
    }
    name_found = csoundFindOutputFile_(csound, filename, envList);
    if (((ENVIRON*) csound)->oparms->odebug) {
      if (name_found != NULL)
        csoundMessage(csound, Str("Found '%s'\n"), name_found);
      else
        csoundMessage(csound, Str("Cannot write '%s' to any of the "
                                  "search paths\n"), filename);
    }
    return name_found;
}

