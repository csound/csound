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
    "CSRTAUDIO",
    "CSSTRNGS",
    "CS_LANG",
    "HOME",
    "INCDIR",
    "OPCODEDIR",
    "OPCODEDIR64",
    "SADIR",
    "SFDIR",
    "SFOUTYP",
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

/* Is 's' already found in 'base' (s >= base) ? Non-zero: yes. */

static int check_for_duplicate(const char *base, const char *s)
{
    int i, j, d;

    if (*s == '\0' || *s == ';')
      return 1;
    i = 0;
    while (i < (int) (s - base)) {
      j = 0;
      d = 1;
      while (base[i] != ';' && i < (int) (s - base)) {
        if (s[j] != base[i])
          d = 0;
        i++;
        if (s[j] != '\0' && s[j] != ';')
          j++;
      }
      if (d && (s[j] == '\0' || s[j] == ';'))
        return 1;
      i++;
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
          new_len += cnt;
          s[new_len++] = ';';   /* put ';' character at end of token */
        }
        if (oldval[i] == '\0')  /* end of list */
          break;
        n = i + 1;              /* start position of next token */
        cnt = 0;
      }
      else
        s[new_len + (cnt++)] = oldval[i];   /* copy token and count length */
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
    if (((ENVIRON*) csound)->oparms_->odebug) {
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

