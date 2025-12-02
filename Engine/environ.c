/*
    files.c: environment functions

    Copyright (C) 2005-25 Istvan Varga, Victor Lazzarini

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

#include "csoundCore.h"
#include "environ.h"
#include "namedins.h"
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* list of environment variables used by Csound */
static const char *envVar_list[] = {
    "CSNOSTOP",
    "CSOUND7RC",
    "CSSTRNGS",
    "CS_LANG",
    "HOME",
    "INCDIR",
    "OPCODE7DIR",
    "OPCODE7DIR64",
    "RAWWAVE_PATH",
    "SADIR",
    "SFDIR",
    "SFOUTYP",
    "SNAPDIR",
    "SSDIR",
    "MFDIR",
    NULL
};


typedef struct searchPathCacheEntry_s {
    char    *name;
    struct searchPathCacheEntry_s   *nxt;
    char    *lst[1];
} searchPathCacheEntry_t;

typedef struct nameChain_s {
    struct nameChain_s  *nxt;
    char    s[1];
} nameChain_t;

/* Space for 16 global environment variables, */
/* 32 bytes for name and 480 bytes for value. */
/* Only written by csoundSetGlobalEnv().      */

static char globalEnvVars[8192] = { (char) 0 };

#define globalEnvVarName(x)   ((char*) &(globalEnvVars[(int32_t) (x) << 9]))
#define globalEnvVarValue(x)  ((char*) &(globalEnvVars[((int32_t) (x) << 9) + 32]))

static int32_t is_valid_envvar_name(const char *name)
{
    char *s;

    if (UNLIKELY(name == NULL || name[0] == '\0'))
      return 0;
    s = (char*) &(name[0]);
    if (UNLIKELY(!(isalpha(*s) || *s == '_')))
      return 0;
    while (*(++s) != '\0') {
      if (UNLIKELY(!(isalpha(*s) || isdigit(*s) || *s == '_')))
        return 0;
    }
    return 1;
}

/**
 * Get pointer to value of environment variable 'name'.
 * Return value is NULL if the variable is not set.
 */

const char *csoundGetEnv(CSOUND *csound, const char *name)
{
    if (csound == NULL) {
      int32_t i;
      if (name == NULL || name[0] == '\0')
        return (const char*) NULL;
      for (i = 0; i < 16; i++) {
        if (strcmp(globalEnvVarName(i), name) == 0)
          return (const char*) globalEnvVarValue(i);
      }
      return (const char*) getenv(name);
    }

    if (csound->envVarDB == NULL) return NULL;

    return (const char*) cs_hash_table_get(csound, csound->envVarDB, (char*)name);
}

/**
 * Set the global value of environment variable 'name' to 'value',
 * or delete variable if 'value' is NULL.
 * It is not safe to call this function while any Csound instances
 * are active.
 * Returns zero on success.
 */

int32_t csoundSetGlobalEnv(const char *name, const char *value)
{
    int32_t   i;

    if (UNLIKELY(name == NULL || name[0] == '\0' || (int32_t) strlen(name) >= 32))
      return -1;                        /* invalid name             */
    for (i = 0; i < 16; i++) {
      if ((value != NULL && globalEnvVarName(i)[0] == '\0') ||
          strcmp(name, globalEnvVarName(i)) == 0)
        break;
    }
    if (UNLIKELY(i >= 16))              /* not found / no free slot */
      return -1;
    if (value == NULL) {
      globalEnvVarName(i)[0] = '\0';    /* delete existing variable */
      return 0;
    }
    if (UNLIKELY(strlen(value) >= 480))
      return -1;                        /* string value is too long */
    strcpy(globalEnvVarName(i), name);
    strcpy(globalEnvVarValue(i), value);
    return 0;
}

/**
 * Set environment variable 'name' to 'value'.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
 * if the environment variable could not be set for some reason.
 */

int32_t csoundSetEnv(CSOUND *csound, const char *name, const char *value)
{
    searchPathCacheEntry_t  *ep, *nxt;
    char                    *oldValue;

    /* check for valid parameters */
    if (UNLIKELY(csound == NULL || !is_valid_envvar_name(name)))
      return CSOUND_ERROR;

    /* invalidate search path cache */
    ep = (searchPathCacheEntry_t*) csound->searchPathCache;
    while (ep != NULL) {
      nxt = ep->nxt;
      csound->Free(csound, ep);
      ep = nxt;
    }
    csound->searchPathCache = NULL;


    oldValue = cs_hash_table_get(csound, csound->envVarDB, (char*)name);
    if (oldValue != NULL) {
      csound->Free(csound, oldValue);
    }

    cs_hash_table_put(csound, csound->envVarDB,
                      (char*)name, cs_strdup(csound, (char*)value));

    /* print debugging info if requested */
    if (UNLIKELY(csoundGetDebug(csound) > 99)) {
      csoundMessage(csound, Str("Environment variable '%s' has been set to "),
                              name);
      if (value == NULL)
        csoundMessage(csound, "NULL\n");
      else
        csoundMessage(csound, "'%s'\n", value);
    }
    /* report success */
    return CSOUND_SUCCESS;
}

/**
 * Append 'value' to environment variable 'name', using ENVSEP as
 * separator character.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
 * if the environment variable could not be set for some reason.
 */

int32_t csoundAppendEnv(CSOUND *csound, const char *name, const char *value)
{
    const char  *oldval;
    char        *newval;
    int32_t         retval;

    /* check for valid parameters */
    if (UNLIKELY(csound == NULL || !is_valid_envvar_name(name)))
      return CSOUND_ERROR;
    /* get original value of variable */
    oldval = csoundGetEnv(csound, name);
    if (oldval == NULL)
      return csoundSetEnv(csound, name, value);
    if (value == NULL || value[0] == '\0')
      return CSOUND_SUCCESS;
    /* allocate new value (+ 2 bytes for ENVSEP and null character) */
    newval = (char*) csound->Malloc(csound, (size_t) strlen(oldval)
                             + (size_t) strlen(value) + (size_t) 2);
    /* append to old value */
    strcpy(newval, oldval);     /* These are safe as space calculated above */
    //    printf("%d: newval = %s\n", __LINE__, newval);
    // should be a better way
    newval[strlen(oldval)]= ENVSEP;
    newval[strlen(oldval)+1]= '\0';
    //    printf("%d: newval = %s\n", __LINE__, newval);
    strcat(newval, value);
    //    printf("%d: newval = %s\n", __LINE__, newval);
    /* set variable */
    retval = csoundSetEnv(csound, name, newval);
    csound->Free(csound, newval);
    /* return with error code */
    return retval;
}

/**
 * Prepend 'value' to environment variable 'name', using ENVSEP as
 * separator character.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or CSOUND_MEMORY
 * if the environment variable could not be set for some reason.
 */

int32_t csoundPrependEnv(CSOUND *csound, const char *name, const char *value)
{
    const char  *oldval;
    char        *newval;
    int32_t         retval;

    /* check for valid parameters */
    if (UNLIKELY(csound == NULL || !is_valid_envvar_name(name)))
      return CSOUND_ERROR;
    /* get original value of variable */
    oldval = csoundGetEnv(csound, name);
    if (oldval == NULL)
      return csoundSetEnv(csound, name, value);
    if (value == NULL || value[0] == '\0')
      return CSOUND_SUCCESS;
    /* allocate new value (+ 2 bytes for ';' and null character) */
    newval = (char*) csound->Malloc(csound, (size_t) strlen(oldval)
                                     + (size_t) strlen(value) + (size_t) 2);
    /* prepend to old value */
    strcpy(newval, value);
    //    printf("%d: newval = %s\n", __LINE__,  newval);
    newval[strlen(value)]= ENVSEP;
    newval[strlen(value)+1]= '\0';
    //    printf("%d: newval = %s\n", __LINE__,  newval);
    strcat(newval, oldval);
    //    printf("%d: newval = %s\n", __LINE__,  newval);
    /* set variable */
    retval = csoundSetEnv(csound, name, newval);
    csound->Free(csound, newval);
    /* return with error code */
    return retval;
}

/**
 * Initialise environment variable database, and copy system
 * environment variables.
 * Returns CSOUND_SUCCESS on success, and CSOUND_ERROR or
 * CSOUND_MEMORY in case of an error.
 */

int32_t csoundInitEnv(CSOUND *csound)
{
    int32_t i, retval;
    /* check if already initialised */
    if (csound->envVarDB != NULL)
      return CSOUND_SUCCESS;
    /* allocate table */
    csound->envVarDB = cs_hash_table_create(csound);
    /* copy standard Csound environment variables */
    for (i = 0; envVar_list[i] != NULL; i++) {
      const char  *name = envVar_list[i];
      const char  *value = getenv(name);
      if (value != NULL) {
        retval = csoundSetEnv(csound, name, value);
        if (retval != CSOUND_SUCCESS)
          return retval;
      }
    }
    /* copy any global defaults set with csoundSetGlobalEnv() */
    for (i = 0; i < 16; i++) {
      if (globalEnvVarName(i)[0] != '\0') {
        retval = csoundSetEnv(csound, globalEnvVarName(i),
                                      globalEnvVarValue(i));
        if (retval != CSOUND_SUCCESS)
          return retval;
      }
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

int32_t csoundParseEnv(CSOUND *csound, const char *s)
{
    char  *name, *value, msg[256];
    int32_t   append_mode, retval;

    /* copy string constant */
    name = (char*) csound->Malloc(csound, (size_t) strlen(s) + (size_t) 1);
    strcpy(name, s);
    /* check assignment format */
    value = strchr(name, '=');
    append_mode = 0;
    if (UNLIKELY(value == NULL || value == name)) {
      strNcpy(msg, Str(" *** invalid format for --env\n"), 256);
      retval = CSOUND_ERROR;
      goto err_return;
    }
    *(value++) = '\0';
    if (*(value - 2) == '+') {
      append_mode = 1;
      *(value - 2) = '\0';
    }
    if (UNLIKELY(!is_valid_envvar_name(name))) {
      strNcpy(msg, Str(" *** invalid environment variable name\n"), 256);
      retval = CSOUND_ERROR;
      goto err_return;
    }
    /* set variable */
    if (!append_mode)
      retval = csoundSetEnv(csound, name, value);
    else
      retval = csoundAppendEnv(csound, name, value);
    if (UNLIKELY(retval == CSOUND_MEMORY))
      strNcpy(msg, Str(" *** memory allocation failure\n"), 256);
    else
      strNcpy(msg, Str(" *** error setting environment variable\n"), 256);

 err_return:
    if (UNLIKELY(retval != CSOUND_SUCCESS))
      csoundMessage(csound, "%s", msg);
    csound->Free(csound, name);
    return retval;
}


char **csoundGetSearchPathFromEnv(CSOUND *csound, const char *envList)
{
    searchPathCacheEntry_t  *p;
    nameChain_t             *env_lst = NULL, *path_lst = NULL, *tmp, *prv, *nxt;
    char                    *s;
    int32_t                     i, j, k, len, pathCnt = 0, totLen = 0;

    /* check if the specified environment variable list was already parsed */
    p = (searchPathCacheEntry_t*) csound->searchPathCache;
    while (p != NULL) {
      if (sCmp(p->name, envList) == 0)
        return (&(p->lst[0]));
      p = p->nxt;
    }
    /* not found, need to create new entry */
    len = (int32_t) strlen(envList);
    /* split environment variable list to tokens */
    for (i = j = 0; i <= len; i++) {
      if (envList[i] == ';' || envList[i] == ':' || envList[i] == '\0') {
        if (i > j) {
          tmp = (nameChain_t*)csound->Malloc(csound, sizeof(nameChain_t) + (i-j));
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
              csound->Free(csound, tmp);       /* and remove if there is any */
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
      csound->Free(csound, env_lst);
      env_lst = nxt;
      if (s != NULL && s[0] != '\0')
        len = (int32_t) strlen(s);
      else
        len = -1;
      // **** THIS CODE DOES NOT CHECK FOR WINDOWS STYLE C:\foo ****
      for (i = j = 0; i <= len; i++) {
        if (i==0 && isalpha(s[i]) && s[i+1]==':') i++;
        else if (s[i] == ';' || s[i] == ':' || s[i] == '\0') {
          if (i > j) {
            tmp = (nameChain_t*) csound->Malloc(csound, sizeof(nameChain_t)
                                                 + (i - j) + 1);
            /* copy with converting pathname delimiters */
            /* FIXME: should call csoundConvertPathname instead */
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
              csound->Free(csound, tmp);
            }
            else {
              /* calculate storage requirement */
              pathCnt++; totLen += (k + 1);
            }
          }
          j = i + 1;
          if (i+2<=len && s[i+2]==':' && isalpha(s[i+1])) i+=2;
        }
      }
    }
    totLen += ((int32_t) strlen(envList) + 1);
    /* create path cache entry */
    p = (searchPathCacheEntry_t*) csound->Malloc(csound,
                                                 sizeof(searchPathCacheEntry_t)
                                                 + sizeof(char*) * pathCnt
                                                 + sizeof(char) * totLen);
    s = (char*) &(p->lst[pathCnt + 1]);
    p->name = s;
    strcpy(p->name, envList);
    s += ((int32_t) strlen(envList) + 1);
    p->nxt = (searchPathCacheEntry_t*) csound->searchPathCache;
    if (UNLIKELY(csoundGetDebug(csound) > 99))
      csound->DebugMsg(csound, Str("Creating search path cache for '%s':"),
                               p->name);
    for (i = 0; (i < pathCnt) && (path_lst != NULL); i++) {
      p->lst[i] = s;
      strcpy(s, path_lst->s);
      s += ((int32_t) strlen(path_lst->s) + 1);
      nxt = path_lst->nxt;
      csound->Free(csound, path_lst);
      path_lst = nxt;
      if (UNLIKELY(csoundGetDebug(csound) > 99))
        csound->DebugMsg(csound, "%5d: \"%s\"", (i + 1), p->lst[i]);
    }
    p->lst[i] = NULL;
    /* link into database */
    csound->searchPathCache = (void*) p;
    /* return with pathname list */
    return (&(p->lst[0]));
}
