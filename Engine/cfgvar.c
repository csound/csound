/*
    cfgvar.c:

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
#include "cfgvar.h"
#include "namedins.h"

/* faster version that assumes a non-empty string */

static inline unsigned char name_hash_(const char *s)
{
    const unsigned char *c = (const unsigned char*) &(s[0]);
    unsigned int  h = 0U;
    do {
      h = strhash_tabl_8[h ^ *c];
    } while (*(++c) != (unsigned char) '\0');
    return (unsigned char) h;
}

#define local_cfg_db    (((CSOUND*) csound)->cfgVariableDB)

/* the global database */

/* list of error messages */

static const char *errmsg_list[] = {
    Str_noop("(no error)"),
    Str_noop("invalid variable name"),
    Str_noop("invalid variable type"),
    Str_noop("invalid flags specified"),
    Str_noop("NULL pointer argument"),
    Str_noop("the specified value is too high"),
    Str_noop("the specified value is too low"),
    Str_noop("the specified value is not an integer power of two"),
    Str_noop("invalid boolean value; must be 0 or 1"),
    Str_noop("memory allocation failed"),
    Str_noop("string exceeds maximum allowed length"),
    Str_noop("(unknown error)")
};

/* check if the specified name, and type and flags values are valid */

static int check_name(const char *name)
{
    int i, c;

    if (UNLIKELY(name == NULL))
      return CSOUNDCFG_INVALID_NAME;
    if (UNLIKELY(name[0] == '\0'))
      return CSOUNDCFG_INVALID_NAME;
    i = -1;
    while (name[++i] != '\0') {
      c = (int) ((unsigned char) name[i]);
      if (UNLIKELY((c & 0x80) || !(c == '_' || isalpha(c) || isdigit(c))))
        return CSOUNDCFG_INVALID_NAME;
    }
    return CSOUNDCFG_SUCCESS;
}

static int check_type(int type)
{
    switch (type) {
      case CSOUNDCFG_INTEGER:
      case CSOUNDCFG_BOOLEAN:
      case CSOUNDCFG_FLOAT:
      case CSOUNDCFG_DOUBLE:
      case CSOUNDCFG_MYFLT:
      case CSOUNDCFG_STRING:
        break;
      default:
        return CSOUNDCFG_INVALID_TYPE;
    }
    return CSOUNDCFG_SUCCESS;
}

static int check_flags(int flags)
{
    if (UNLIKELY((flags & (~(CSOUNDCFG_POWOFTWO))) != 0))
      return CSOUNDCFG_INVALID_FLAG;
    return CSOUNDCFG_SUCCESS;
}

/**
 * Allocate configuration variable structure with the specified parameters:
 *   name:    name of the variable (may contain letters, digits, and _)
 *   p:       pointer to variable
 *   type:    type of variable, determines how 'p' is interpreted
 *              CSOUNDCFG_INTEGER:      int*
 *              CSOUNDCFG_BOOLEAN:      int* (value may be 0 or 1)
 *              CSOUNDCFG_FLOAT:        float*
 *              CSOUNDCFG_DOUBLE:       double*
 *              CSOUNDCFG_MYFLT:        MYFLT*
 *              CSOUNDCFG_STRING:       char* (should have enough space)
 *   flags:   bitwise OR of flag values, currently only CSOUNDCFG_POWOFTWO
 *            is available, which requests CSOUNDCFG_INTEGER values to be
 *            power of two
 *   min:     for CSOUNDCFG_INTEGER, CSOUNDCFG_FLOAT, CSOUNDCFG_DOUBLE, and
 *            CSOUNDCFG_MYFLT, a pointer to a variable of the type selected
 *            by 'type' that specifies the minimum allowed value.
 *            If 'min' is NULL, there is no minimum value.
 *   max:     similar to 'min', except it sets the maximum allowed value.
 *            For CSOUNDCFG_STRING, it is a pointer to an int variable
 *            that defines the maximum length of the string (including the
 *            null character at the end) in bytes. This value is limited
 *            to the range 8 to 16384, and if max is NULL, it defaults to 256.
 *   shortDesc: a short description of the variable (may be NULL or an empty
 *            string if a description is not available)
 *   longDesc: a long description of the variable (may be NULL or an empty
 *            string if a description is not available)
 * A pointer to the newly allocated structure is stored in (*ptr).
 * Return value is CSOUNDCFG_SUCCESS, or one of the error codes.
 */

static int cfg_alloc_structure(csCfgVariable_t **ptr,
                               const char *name,
                               void *p, int type, int flags,
                               void *min, void *max,
                               const char *shortDesc, const char *longDesc)
{
    int     structBytes = 0, nameBytes, sdBytes, ldBytes, totBytes;
    void    *pp;
    unsigned char *sdp = NULL, *ldp = NULL;

    (*ptr) = (csCfgVariable_t*) NULL;
    /* check for valid parameters */
    if (UNLIKELY(p == NULL))
      return CSOUNDCFG_NULL_POINTER;
    if (UNLIKELY(check_name(name) != CSOUNDCFG_SUCCESS))
      return CSOUNDCFG_INVALID_NAME;
    if (UNLIKELY(check_type(type) != CSOUNDCFG_SUCCESS))
      return CSOUNDCFG_INVALID_TYPE;
    if (UNLIKELY(check_flags(flags) != CSOUNDCFG_SUCCESS))
      return CSOUNDCFG_INVALID_FLAG;
    /* calculate the number of bytes to allocate */
    structBytes = ((int) sizeof(csCfgVariable_t) + 15) & (~15);
    nameBytes = (((int) strlen(name) + 1) + 15) & (~15);
    if (shortDesc != NULL)
      sdBytes = (shortDesc[0] == '\0' ? 0 : (int) strlen(shortDesc) + 1);
    else
      sdBytes = 0;
    sdBytes = (sdBytes + 15) & (~15);
    if (longDesc != NULL)
      ldBytes = (longDesc[0] == '\0' ? 0 : (int) strlen(longDesc) + 1);
    else
      ldBytes = 0;
    ldBytes = (ldBytes + 15) & (~15);
    totBytes = structBytes + nameBytes + sdBytes + ldBytes;
    /* allocate memory */
    pp = (void*) malloc((size_t) totBytes);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_MEMORY;
    memset(pp, 0, (size_t) totBytes);
    (*ptr) = (csCfgVariable_t*) pp;
    /* copy name and descriptions */
    strcpy(((char*) pp + (int) structBytes), name);
    if (sdBytes > 0) {
      sdp = (unsigned char*) pp + (int) (structBytes + nameBytes);
      strcpy((char*) sdp, shortDesc);
    }
    else
      sdp = NULL;
    if (ldBytes > 0) {
      ldp = (unsigned char*) pp + (int) (structBytes + nameBytes + sdBytes);
      strcpy((char*) ldp, longDesc);
    }
    else
      ldp = NULL;
    /* set up structure */
    (*ptr)->h.nxt = (csCfgVariable_t*) NULL;
    (*ptr)->h.name = (unsigned char*) pp + (int) structBytes;
    (*ptr)->h.p = p;
    (*ptr)->h.type = type;
    (*ptr)->h.flags = flags;
    (*ptr)->h.shortDesc = sdp;
    (*ptr)->h.longDesc = ldp;
    /* depending on type */
    switch (type) {
      case CSOUNDCFG_INTEGER:                                   /* integer */
        (*ptr)->i.min = (min == NULL ? -0x7FFFFFFF : *((int*) min));
        (*ptr)->i.max = (max == NULL ? 0x7FFFFFFF : *((int*) max));
        break;
      case CSOUNDCFG_BOOLEAN:                                   /* boolean */
        (*ptr)->b.flags &= (~(CSOUNDCFG_POWOFTWO));
        break;
      case CSOUNDCFG_FLOAT:                                     /* float */
        (*ptr)->f.flags &= (~(CSOUNDCFG_POWOFTWO));
        (*ptr)->f.min = (min == NULL ? -1.0e30f : *((float*) min));
        (*ptr)->f.max = (max == NULL ? 1.0e30f : *((float*) max));
        break;
      case CSOUNDCFG_DOUBLE:                                    /* double */
        (*ptr)->d.flags &= (~(CSOUNDCFG_POWOFTWO));
        (*ptr)->d.min = (min == NULL ? -1.0e30 : *((double*) min));
        (*ptr)->d.max = (max == NULL ? 1.0e30 : *((double*) max));
        break;
      case CSOUNDCFG_MYFLT:                                     /* MYFLT */
        (*ptr)->m.flags &= (~(CSOUNDCFG_POWOFTWO));
        (*ptr)->m.min = (min == NULL ? (MYFLT) -1.0e30 : *((MYFLT*) min));
        (*ptr)->m.max = (max == NULL ? (MYFLT) 1.0e30 : *((MYFLT*) max));
        break;
      case CSOUNDCFG_STRING:                                    /* string */
        (*ptr)->s.flags &= (~(CSOUNDCFG_POWOFTWO));
        if (max != NULL) {
          (*ptr)->s.maxlen = *((int*) max);
          if ((*ptr)->s.maxlen < 8) (*ptr)->s.maxlen = 8;
          if ((*ptr)->s.maxlen > 16384) (*ptr)->s.maxlen = 16384;
        }
        else
          (*ptr)->s.maxlen = 256;   /* default maximum string length */
        break;
    }
    /* done */
    return CSOUNDCFG_SUCCESS;
}

/**
 * Create global configuration variable with the specified parameters.
 * This function should be called by the host application only.
 *   name:    name of the variable (may contain letters, digits, and _)
 *   p:       pointer to variable
 *   type:    type of variable, determines how 'p' is interpreted
 *              CSOUNDCFG_INTEGER:      int*
 *              CSOUNDCFG_BOOLEAN:      int* (value may be 0 or 1)
 *              CSOUNDCFG_FLOAT:        float*
 *              CSOUNDCFG_DOUBLE:       double*
 *              CSOUNDCFG_MYFLT:        MYFLT*
 *              CSOUNDCFG_STRING:       char* (should have enough space)
 *   flags:   bitwise OR of flag values, currently only CSOUNDCFG_POWOFTWO
 *            is available, which requests CSOUNDCFG_INTEGER values to be
 *            power of two
 *   min:     for CSOUNDCFG_INTEGER, CSOUNDCFG_FLOAT, CSOUNDCFG_DOUBLE, and
 *            CSOUNDCFG_MYFLT, a pointer to a variable of the type selected
 *            by 'type' that specifies the minimum allowed value.
 *            If 'min' is NULL, there is no minimum value.
 *   max:     similar to 'min', except it sets the maximum allowed value.
 *            For CSOUNDCFG_STRING, it is a pointer to an int variable
 *            that defines the maximum length of the string (including the
 *            null character at the end) in bytes. This value is limited
 *            to the range 8 to 16384, and if max is NULL, it defaults to 256.
 *   shortDesc: a short description of the variable (may be NULL or an empty
 *            string if a description is not available)
 *   longDesc: a long description of the variable (may be NULL or an empty
 *            string if a description is not available)
 * Return value is CSOUNDCFG_SUCCESS, or one of the following error codes:
 *   CSOUNDCFG_INVALID_NAME
 *            the specified name is invalid or is already in use
 *   CSOUNDCFG_MEMORY
 *            a memory allocation failure occured
 *   CSOUNDCFG_NULL_POINTER
 *            the 'p' pointer was NULL
 *   CSOUNDCFG_INVALID_TYPE
 *   CSOUNDCFG_INVALID_FLAG
 *            an invalid variable type was specified, or the flags value
 *            had unknown bits set
 */

#if 0
PUBLIC
int csoundCreateGlobalConfigurationVariable(const char *name,
                                            void *p, int type, int flags,
                                            void *min, void *max,
                                            const char *shortDesc,
                                            const char *longDesc)
{
    csCfgVariable_t *pp;
    int             i, retval;
    unsigned char   h;

    /* check if name is already in use */
    if (UNLIKELY(csoundQueryGlobalConfigurationVariable(name) != NULL))
      return CSOUNDCFG_INVALID_NAME;
    /* if database is not allocated yet, create an empty one */
    if (global_cfg_db == NULL) {
      global_cfg_db = (void**) malloc(sizeof(void*) * 256);
      if (UNLIKELY(global_cfg_db == NULL))
        return CSOUNDCFG_MEMORY;
      for (i = 0; i < 256; i++)
        global_cfg_db[i] = (void*) NULL;
    }
    /* allocate structure */
    retval =  cfg_alloc_structure(&pp, name, p, type, flags, min, max,
                                  shortDesc, longDesc);
    if (retval != CSOUNDCFG_SUCCESS)
      return retval;
    /* link into database */
    h = name_hash_(name);
    pp->h.nxt = (csCfgVariable_t*) (global_cfg_db[(int) h]);
    global_cfg_db[(int) h] = (void*) pp;
    /* report success */
    return CSOUNDCFG_SUCCESS;
}
#endif

/**
 * This function is similar to csoundCreateGlobalConfigurationVariable(),
 * except it creates a configuration variable specific to Csound instance
 * 'csound', and is suitable for calling from the Csound library
 * (in csoundPreCompile()) or plugins (in csoundModuleCreate()).
 * The other parameters and return value are the same as in the case of
 * csoundCreateGlobalConfigurationVariable().
 */

PUBLIC int
  csoundCreateConfigurationVariable(CSOUND *csound, const char *name,
                                    void *p, int type, int flags,
                                    void *min, void *max,
                                    const char *shortDesc,
                                    const char *longDesc)
{
    csCfgVariable_t *pp;
    int             i, retval;
    unsigned char   h;

    /* check if name is already in use */
    if (UNLIKELY(csoundQueryConfigurationVariable(csound, name) != NULL))
      return CSOUNDCFG_INVALID_NAME;
    /* if database is not allocated yet, create an empty one */
    if (local_cfg_db == NULL) {
      local_cfg_db = (void**) malloc(sizeof(void*) * 256);
      if (UNLIKELY(local_cfg_db == NULL))
        return CSOUNDCFG_MEMORY;
      for (i = 0; i < 256; i++)
        local_cfg_db[i] = (void*) NULL;
    }
    /* allocate structure */
    retval =  cfg_alloc_structure(&pp, name, p, type, flags, min, max,
                                  shortDesc, longDesc);
    if (UNLIKELY(retval != CSOUNDCFG_SUCCESS))
      return retval;
    /* link into database */
    h = name_hash_(name);
    pp->h.nxt = (csCfgVariable_t*) (local_cfg_db[(int) h]);
    local_cfg_db[(int) h] = (void*) pp;
    /* report success */
    return CSOUNDCFG_SUCCESS;
}

/* returns non-zero if x and y are equal within maxerr error */

#if 0
static int compare_floats(double x, double y, double maxerr)
{
    /* trivial case */
    if (x == y)
      return 1;
    /* if only one of x and y is zero, then not equal */
    if (x == 0.0 || y == 0.0) {
      if (x == 0.0 && y == 0.0)
        return 1;
      else
        return 0;
    }
    /* different sign: not equal */
    if ((x < 0.0 && y > 0.0) || (x > 0.0 && y < 0.0))
      return 0;
    /* fuzzy compare */
    if (fabs((x / y) - 1.0) < maxerr)
      return 1;
    return 0;
}
#endif

/* Test if two existing configuration variables are compatible, that is, */
/* they have the same type, flags, descriptions, and limit values. */
/* Return value is non-zero if the two variables are compatible. */

#if 0
static int are_cfgvars_compatible(csCfgVariable_t *p1, csCfgVariable_t *p2)
{
    if (p1->h.type != p2->h.type || p1->h.flags != p2->h.flags)
      return 0;
    if (p1->h.shortDesc != NULL && p2->h.shortDesc != NULL) {
      if (strcmp((char*) p1->h.shortDesc, (char*) p2->h.shortDesc) != 0)
        return 0;
    }
    else if (!(p1->h.shortDesc == NULL && p2->h.shortDesc == NULL))
      return 0;
    if (p1->h.longDesc != NULL && p2->h.longDesc != NULL) {
      if (strcmp((char*) p1->h.longDesc, (char*) p2->h.longDesc) != 0)
        return 0;
    }
    else if (!(p1->h.longDesc == NULL && p2->h.longDesc == NULL))
      return 0;
    switch (p1->h.type) {
      case CSOUNDCFG_INTEGER:
        if (p1->i.min != p2->i.min || p1->i.max != p2->i.max)
          return 0;
        break;
      case CSOUNDCFG_BOOLEAN:
        break;
      case CSOUNDCFG_FLOAT:
        if (!compare_floats((double) p1->f.min, (double) p2->f.min, 1.0e-6) ||
            !compare_floats((double) p1->f.max, (double) p2->f.max, 1.0e-6))
          return 0;
        break;
      case CSOUNDCFG_DOUBLE:
        if (!compare_floats((double) p1->d.min, (double) p2->d.min, 1.0e-12) ||
            !compare_floats((double) p1->d.max, (double) p2->d.max, 1.0e-12))
          return 0;
        break;
      case CSOUNDCFG_MYFLT:
#ifdef USE_DOUBLE
        if (!compare_floats((double) p1->m.min, (double) p2->m.min, 1.0e-12) ||
            !compare_floats((double) p1->m.max, (double) p2->m.max, 1.0e-12))
          return 0;
#else
        if (!compare_floats((double) p1->m.min, (double) p2->m.min, 1.0e-6) ||
            !compare_floats((double) p1->m.max, (double) p2->m.max, 1.0e-6))
          return 0;
#endif
        break;
      case CSOUNDCFG_STRING:
        if (p1->s.maxlen != p2->s.maxlen)
          return 0;
        break;
      default:
        return 0;
    }
    return 1;
}
#endif

/**
 * Copy a global configuration variable to a Csound instance.
 * This function is experimental and may be subject to changes in
 * future releases of the Csound library.
 */

#if 0
PUBLIC int csoundCopyGlobalConfigurationVariable(CSOUND *csound,
                                                 const char *name,
                                                 void *p)
{
    csCfgVariable_t *pp, *lp;
    char            *sdp, *ldp;
    int             j, type, flags;

    /* look up global configuration variable */
    pp = csoundQueryGlobalConfigurationVariable(name);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_INVALID_NAME;

    /* check if a local configuration variable with this name already exists */
    lp = csoundQueryConfigurationVariable(csound, name);
    if (lp != NULL) {
      /* found one, find out if it is compatible */
      if (UNLIKELY(!are_cfgvars_compatible(lp, pp)))
        return CSOUNDCFG_INVALID_NAME;      /* no, report error */
      /* if a new data pointer was specified, store it */
      if (p != NULL)
        lp->h.p = p;
      else
        p = lp->h.p;    /* otherwise, get it from the existing variable */
    }
    else {
      /* create new local configuration variable */
      if (UNLIKELY(p == NULL))
        return CSOUNDCFG_NULL_POINTER;  /* must have a valid data pointer */
      type = pp->h.type;
      flags = pp->h.flags;
      sdp = (char*) pp->h.shortDesc;
      ldp = (char*) pp->h.longDesc;
      switch (type) {
        case CSOUNDCFG_INTEGER:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                (void*) &(pp->i.min),
                                                (void*) &(pp->i.max), sdp, ldp);
          break;
        case CSOUNDCFG_BOOLEAN:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                NULL, NULL, sdp, ldp);
          break;
        case CSOUNDCFG_FLOAT:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                (void*) &(pp->f.min),
                                                (void*) &(pp->f.max), sdp, ldp);
          break;
        case CSOUNDCFG_DOUBLE:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                (void*) &(pp->d.min),
                                                (void*) &(pp->d.max), sdp, ldp);
          break;
        case CSOUNDCFG_MYFLT:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                (void*) &(pp->m.min),
                                                (void*) &(pp->m.max), sdp, ldp);
          break;
        case CSOUNDCFG_STRING:
          j = csoundCreateConfigurationVariable(csound, name, p, type, flags,
                                                NULL, (void*) &(pp->s.maxlen),
                                                sdp, ldp);
          break;
        default:
          return CSOUNDCFG_INVALID_TYPE;
      }
      if (j != CSOUNDCFG_SUCCESS)
        return j;
    }
    /* copy value */
    switch (pp->h.type) {
      case CSOUNDCFG_INTEGER:   *((int*) p) = *(pp->i.p);               break;
      case CSOUNDCFG_BOOLEAN:   *((int*) p) = *(pp->b.p);               break;
      case CSOUNDCFG_FLOAT:     *((float*) p) = *(pp->f.p);             break;
      case CSOUNDCFG_DOUBLE:    *((double*) p) = *(pp->d.p);            break;
      case CSOUNDCFG_MYFLT:     *((MYFLT*) p) = *(pp->m.p);             break;
      case CSOUNDCFG_STRING:    strcpy((char*) p, (char*) (pp->s.p));   break;
      default: return CSOUNDCFG_INVALID_TYPE;
    }
    /* report success */
    return CSOUNDCFG_SUCCESS;
}
#endif

/**
 * Copy all global configuration variables to the specified Csound instance.
 * This function is experimental and may be subject to changes in
 * future releases of the Csound library.
 */

#if 0
PUBLIC int csoundCopyGlobalConfigurationVariables(CSOUND *csound)
{
    csCfgVariable_t *pp, *lp;
    int             i, j, k, retval;
    char            *s;
    void            *ptr = NULL;

    if (UNLIKELY(global_cfg_db == NULL)) {
      /* empty database: nothing to do */
      return CSOUNDCFG_SUCCESS;
    }

    retval = CSOUNDCFG_SUCCESS;
    for (i = 0; i < 256; i++) {
      pp = (csCfgVariable_t*) (global_cfg_db[i]);
      while (pp != NULL) {
        /* does a local configuration variable with this name already exist ? */
        lp = csoundQueryConfigurationVariable(csound, (char*) pp->h.name);
        if (lp == NULL) {
          /* no, create named global variable */
          s = (char*) malloc((size_t) strlen((char*) pp->h.name) + (size_t) 2);
          if (UNLIKELY(s == NULL))
            return CSOUNDCFG_MEMORY;
          strcpy(s, ".");
          strcat(s, (char*) pp->h.name);
          switch (pp->h.type) {
            case CSOUNDCFG_INTEGER:   k = (int) sizeof(int);      break;
            case CSOUNDCFG_BOOLEAN:   k = (int) sizeof(int);      break;
            case CSOUNDCFG_FLOAT:     k = (int) sizeof(float);    break;
            case CSOUNDCFG_DOUBLE:    k = (int) sizeof(double);   break;
            case CSOUNDCFG_MYFLT:     k = (int) sizeof(MYFLT);    break;
            case CSOUNDCFG_STRING:    k = (int) (pp->s.maxlen);   break;
            default:  k = 1;  /* should not happen */
          }
          j = csoundCreateGlobalVariable(csound, s, (size_t) k);
          if (UNLIKELY(j != CSOUND_SUCCESS)) {
            if (j == CSOUND_MEMORY)
              return CSOUNDCFG_MEMORY;
            else {
              retval = CSOUNDCFG_INVALID_NAME;
              pp = (csCfgVariable_t*) (pp->h.nxt);
              continue;
            }
          }
          /* get pointer (should not be NULL) */
          ptr = csoundQueryGlobalVariable(csound, s);
          free((void*) s);
        }
        else
          ptr = NULL;   /* use pointer of existing configuration variable */
        /* copy variable */
        j = csoundCopyGlobalConfigurationVariable(csound, (char*) pp->h.name,
                                                  ptr);
        if (UNLIKELY(j == CSOUNDCFG_MEMORY || j == CSOUNDCFG_NULL_POINTER))
          return j;
        else if (UNLIKELY(j != CSOUNDCFG_SUCCESS))
          retval = CSOUNDCFG_INVALID_NAME;
        /* continue with next link in chain */
        pp = (csCfgVariable_t*) (pp->h.nxt);
      }
    }
    return retval;
}
#endif

/* set configuration variable to value (with error checking) */

static int set_cfgvariable_value(csCfgVariable_t *pp, void *value)
{
    double  dVal;
    MYFLT   mVal;
    float   fVal;
    int     iVal;
    /* set value depending on type */
    if (UNLIKELY(value == NULL))
      return CSOUNDCFG_NULL_POINTER;
    switch (pp->h.type) {
      case CSOUNDCFG_INTEGER:
        iVal = *((int*) value);
        if (UNLIKELY(iVal < pp->i.min)) return CSOUNDCFG_TOO_LOW;
        if (UNLIKELY(iVal > pp->i.max)) return CSOUNDCFG_TOO_HIGH;
        if (pp->i.flags & CSOUNDCFG_POWOFTWO)
          if (UNLIKELY(iVal < 1 || (iVal & (iVal - 1)) != 0))
            return CSOUNDCFG_NOT_POWOFTWO;
        *(pp->i.p) = iVal;
        break;
      case CSOUNDCFG_BOOLEAN:
        iVal = *((int*) value);
        if (UNLIKELY(iVal & (~1))) return CSOUNDCFG_INVALID_BOOLEAN;
        *(pp->b.p) = iVal;
        break;
      case CSOUNDCFG_FLOAT:
        fVal = *((float*) value);
        if (UNLIKELY(fVal < pp->f.min)) return CSOUNDCFG_TOO_LOW;
        if (UNLIKELY(fVal > pp->f.max)) return CSOUNDCFG_TOO_HIGH;
        *(pp->f.p) = fVal;
        break;
      case CSOUNDCFG_DOUBLE:
        dVal = *((double*) value);
        if (UNLIKELY(dVal < pp->d.min)) return CSOUNDCFG_TOO_LOW;
        if (UNLIKELY(dVal > pp->d.max)) return CSOUNDCFG_TOO_HIGH;
        *(pp->d.p) = dVal;
        break;
      case CSOUNDCFG_MYFLT:
        mVal = *((MYFLT*) value);
        if (UNLIKELY(mVal < pp->m.min)) return CSOUNDCFG_TOO_LOW;
        if (UNLIKELY(mVal > pp->m.max)) return CSOUNDCFG_TOO_HIGH;
        *(pp->m.p) = mVal;
        break;
      case CSOUNDCFG_STRING:
        if (UNLIKELY((int) strlen((char*) value) >= (pp->s.maxlen - 1)))
          return CSOUNDCFG_STRING_LENGTH;
        strcpy((char*) (pp->s.p), (char*) value);
        break;
      default:
        return CSOUNDCFG_INVALID_TYPE;
    }
    return CSOUNDCFG_SUCCESS;
}

/**
 * Set the value of a global configuration variable; should be called by the
 * host application only.
 * 'value' is a pointer of the same type as the 'p' pointer that was passed
 * to csoundCreateGlobalConfigurationVariable(), depending on the type of
 * the variable (integer, float, etc.).
 * Return value is CSOUNDCFG_SUCCESS in case of success, or one of the
 * following error codes:
 *   CSOUNDCFG_INVALID_NAME
 *            no configuration variable was found with the specified name
 *   CSOUNDCFG_NULL_POINTER
 *            the 'value' pointer was NULL
 *   CSOUNDCFG_TOO_LOW
 *   CSOUNDCFG_TOO_HIGH
 *   CSOUNDCFG_NOT_POWOFTWO
 *   CSOUNDCFG_INVALID_BOOLEAN
 *   CSOUNDCFG_STRING_LENGTH
 *            the specified value was invalid in some way
 */

#if 0
PUBLIC int csoundSetGlobalConfigurationVariable(const char *name, void *value)
{
    csCfgVariable_t *pp;

    /* get pointer to variable */
    pp = csoundQueryGlobalConfigurationVariable(name);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_INVALID_NAME;    /* not found */
    return (set_cfgvariable_value(pp, value));
}
#endif

/**
 * Set the value of a configuration variable of Csound instance 'csound'.
 * The 'name' and 'value' parameters, and return value are the same as
 * in the case of csoundSetGlobalConfigurationVariable().
 */

PUBLIC int
  csoundSetConfigurationVariable(CSOUND *csound, const char *name, void *value)
{
    csCfgVariable_t *pp;

    /* get pointer to variable */
    pp = csoundQueryConfigurationVariable(csound, name);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_INVALID_NAME;    /* not found */
    return (set_cfgvariable_value(pp, value));
}

/* parse string value for configuration variable */

static int parse_cfg_variable(csCfgVariable_t *pp, const char *value)
{
    double  dVal;
    MYFLT   mVal;
    float   fVal;
    int     iVal;

    if (UNLIKELY(value == NULL))
      return CSOUNDCFG_NULL_POINTER;
    switch (pp->h.type) {
      case CSOUNDCFG_INTEGER:
        iVal = (int) atoi(value);
        return set_cfgvariable_value(pp, (void*) (&iVal));
      case CSOUNDCFG_BOOLEAN:
        if (strcmp(value, "0") == 0 ||
            strcmp(value, "no") == 0 || strcmp(value, "No") == 0 ||
            strcmp(value, "NO") == 0 || strcmp(value, "off") == 0 ||
            strcmp(value, "Off") == 0 || strcmp(value, "OFF") == 0 ||
            strcmp(value, "false") == 0 || strcmp(value, "False") == 0 ||
            strcmp(value, "FALSE") == 0)
          *(pp->b.p) = 0;
        else if (strcmp(value, "1") == 0 ||
                 strcmp(value, "yes") == 0 || strcmp(value, "Yes") == 0 ||
                 strcmp(value, "YES") == 0 || strcmp(value, "on") == 0 ||
                 strcmp(value, "On") == 0 || strcmp(value, "ON") == 0 ||
                 strcmp(value, "true") == 0 || strcmp(value, "True") == 0 ||
                 strcmp(value, "TRUE") == 0)
          *(pp->b.p) = 1;
        else
          return CSOUNDCFG_INVALID_BOOLEAN;
        return CSOUNDCFG_SUCCESS;
      case CSOUNDCFG_FLOAT:
        fVal = (float) atof(value);
        return set_cfgvariable_value(pp, (void*) (&fVal));
      case CSOUNDCFG_DOUBLE:
        dVal = (double) atof(value);
        return set_cfgvariable_value(pp, (void*) (&dVal));
      case CSOUNDCFG_MYFLT:
        mVal = (MYFLT) atof(value);
        return set_cfgvariable_value(pp, (void*) (&mVal));
      case CSOUNDCFG_STRING:
        return set_cfgvariable_value(pp, (void*) value);
    }
    return CSOUNDCFG_INVALID_TYPE;
}

/**
 * Set the value of a global configuration variable, by parsing a string;
 * should be called by the host application only.
 * For boolean variables, any of the strings "0", "no", "off", and "false"
 * will set the value to 0, and any of "1", "yes", "on", and "true" means a
 * value of 1.
 * Return value is CSOUNDCFG_SUCCESS in case of success, or one of the
 * following error codes:
 *   CSOUNDCFG_INVALID_NAME
 *            no configuration variable was found with the specified name
 *   CSOUNDCFG_NULL_POINTER
 *            the 'value' pointer was NULL
 *   CSOUNDCFG_TOO_LOW
 *   CSOUNDCFG_TOO_HIGH
 *   CSOUNDCFG_NOT_POWOFTWO
 *   CSOUNDCFG_INVALID_BOOLEAN
 *   CSOUNDCFG_STRING_LENGTH
 *            the specified value was invalid in some way
 */

#if 0
PUBLIC int
  csoundParseGlobalConfigurationVariable(const char *name, const char *value)
{
    csCfgVariable_t *pp;

    /* get pointer to variable */
    pp = csoundQueryGlobalConfigurationVariable(name);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_INVALID_NAME;    /* not found */
    return (parse_cfg_variable(pp, value));
}
#endif

/**
 * Set the value of a configuration variable of Csound instance 'csound',
 * by parsing a string.
 * The 'name' and 'value' parameters, and return value are the same as
 * in the case of csoundParseGlobalConfigurationVariable().
 */

PUBLIC int
  csoundParseConfigurationVariable(CSOUND *csound, const char *name,
                                                    const char *value)
{
    csCfgVariable_t *pp;

    /* get pointer to variable */
    pp = csoundQueryConfigurationVariable(csound, name);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_INVALID_NAME;    /* not found */
    return (parse_cfg_variable(pp, value));
}

static csCfgVariable_t *find_cfg_variable(void **db, const char *name)
{
    csCfgVariable_t *pp;
    unsigned char   h;
    /* check for trivial errors */
    if (UNLIKELY(db == NULL || name == NULL))
      return (csCfgVariable_t*) NULL;
    if (UNLIKELY(name[0] == '\0'))
      return (csCfgVariable_t*) NULL;
    /* calculate hash value */
    h = name_hash_(name);
    /* find entry in database */
    pp = (csCfgVariable_t*) (db[(int) h]);
    while (UNLIKELY(pp!=NULL)) {
      if (sCmp((char*) pp->h.name, name) == 0)
        return pp;                          /* found */
      pp = (csCfgVariable_t*) (pp->h.nxt);
    }
    return (csCfgVariable_t*) NULL;
}

/**
 * Return pointer to the global configuration variable with the specified name.
 * The return value may be NULL if the variable is not found in the database.
 */

#if 0
PUBLIC csCfgVariable_t
  *csoundQueryGlobalConfigurationVariable(const char *name)
{
    return find_cfg_variable(global_cfg_db, name);
}
#endif

/**
 * Return pointer to the configuration variable of Csound instace 'csound'
 * with the specified name.
 * The return value may be NULL if the variable is not found in the database.
 */

PUBLIC csCfgVariable_t
    *csoundQueryConfigurationVariable(CSOUND *csound, const char *name)
{
    return find_cfg_variable(local_cfg_db, name);
}

/* compare function for qsort() */

static int compare_func(const void *p1, const void *p2)
{
    return (int) strcmp((char*) ((*((csCfgVariable_t**) p1))->h.name),
                        (char*) ((*((csCfgVariable_t**) p2))->h.name));
}

/* create alphabetically sorted list of all entries in 'db' */

static csCfgVariable_t **list_db_entries(void **db)
{
    csCfgVariable_t *pp, **lst;
    size_t          cnt = (size_t) 0;
    int             i;

    /* count the number of entries */
    if (db != NULL) {
      for (i = 0; i < 256; i++) {
        pp = (csCfgVariable_t*) (db[i]);
        while (pp != NULL) {
          cnt++;
          pp = (csCfgVariable_t*) (pp->h.nxt);
        }
      }
    }
    /* allocate memory for list */
    lst = (csCfgVariable_t**) malloc(sizeof(csCfgVariable_t*)
                                     * (cnt + (size_t) 1));
    if (UNLIKELY(lst == NULL))
      return (csCfgVariable_t**) NULL;  /* not enough memory */
    /* create list */
    if (cnt) {
      cnt = (size_t) 0;
      for (i = 0; i < 256; i++) {
        pp = (csCfgVariable_t*) (db[i]);
        while (pp != NULL) {
          lst[cnt++] = pp;
          pp = (csCfgVariable_t*) (pp->h.nxt);
        }
      }
      /* sort list */
      qsort((void*) lst, cnt, sizeof(csCfgVariable_t*), compare_func);
    }
    lst[cnt] = (csCfgVariable_t*) NULL;
    /* return pointer to list */
    return lst;
}

/**
 * Create an alphabetically sorted list of all global configuration variables.
 * Returns a pointer to a NULL terminated array of configuration variable
 * pointers, or NULL on error.
 * The caller is responsible for freeing the returned list with
 * csoundDeleteCfgVarList(), however, the variable pointers in the list
 * should not be freed.
 */

#if 0
PUBLIC csCfgVariable_t **csoundListGlobalConfigurationVariables(void)
{
    return (list_db_entries(global_cfg_db));
}
#endif

/**
 * Create an alphabetically sorted list of all configuration variables
 * of Csound instance 'csound'.
 * Returns a pointer to a NULL terminated array of configuration variable
 * pointers, or NULL on error.
 * The caller is responsible for freeing the returned list with
 * csoundDeleteCfgVarList(), however, the variable pointers in the list
 * should not be freed.
 */

PUBLIC csCfgVariable_t **csoundListConfigurationVariables(CSOUND *csound)
{
    return (list_db_entries(local_cfg_db));
}

/**
 * Release a configuration variable list previously returned
 * by csoundListGlobalConfigurationVariables() or
 * csoundListConfigurationVariables().
 */

PUBLIC void csoundDeleteCfgVarList(csCfgVariable_t **lst)
{
    if (lst != NULL)
      free(lst);
}

/* remove a configuration variable from 'db' */

static int remove_entry_from_db(void **db, const char *name)
{
    csCfgVariable_t *pp, *prvp;
    unsigned char   h;
    /* first, check if this key actually exists */
    if (UNLIKELY(find_cfg_variable(db, name) == NULL))
      return CSOUNDCFG_INVALID_NAME;
    /* calculate hash value */
    h = name_hash_(name);
    /* find entry in database */
    prvp = (csCfgVariable_t*) NULL;
    pp = (csCfgVariable_t*) (db[(int) h]);
    while (strcmp((char*) pp->h.name, name) != 0) {
      prvp = pp;
      pp = (csCfgVariable_t*) (pp->h.nxt);
    }
    if (prvp != NULL)
      prvp->h.nxt = pp->h.nxt;      /* unlink */
    else
      db[(int) h] = (void*) (pp->h.nxt);
    /* free allocated memory */
    free ((void*) pp);
    return CSOUNDCFG_SUCCESS;
}

/**
 * Remove the global configuration variable with the specified name
 * from the database. Should be called by the host application only,
 * and never by the Csound library or plugins.
 * Return value is CSOUNDCFG_SUCCESS in case of success, or
 * CSOUNDCFG_INVALID_NAME if the variable was not found.
 */

#if 0
PUBLIC int csoundDeleteGlobalConfigurationVariable(const char *name)
{
    return (remove_entry_from_db(global_cfg_db, name));
}
#endif

/**
 * Remove the configuration variable of Csound instance 'csound' with the
 * specified name from the database. Plugins need not call this, as all
 * configuration variables are automatically deleted by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success, or
 * CSOUNDCFG_INVALID_NAME if the variable was not found.
 */

PUBLIC int csoundDeleteConfigurationVariable(CSOUND *csound, const char *name)
{
    return (remove_entry_from_db(local_cfg_db, name));
}

static int destroy_entire_db(void **db)
{
    csCfgVariable_t *pp, *prvp;
    int             i;

    if (db == NULL)
      return CSOUNDCFG_SUCCESS;
    for (i = 0; i < 256; i++) {
      prvp = (csCfgVariable_t*) NULL;
      pp = (csCfgVariable_t*) (db[i]);
      while (pp != NULL) {
        prvp = pp;
        pp = (csCfgVariable_t*) (pp->h.nxt);
        free((void*) prvp);
      }
    }
    free((void*) db);
    return CSOUNDCFG_SUCCESS;
}

/**
 * Remove all global configuration variables and free database.
 * Should be called by the host application only, and never by the
 * Csound library or plugins.
 * Return value is CSOUNDCFG_SUCCESS in case of success.
 */

#if 0
PUBLIC int csoundDeleteAllGlobalConfigurationVariables(void)
{
    int retval;
    retval = destroy_entire_db(global_cfg_db);
    global_cfg_db = NULL;
    return retval;
}
#endif

/**
 * Remove all configuration variables of Csound instance 'csound'
 * and free database. This function is called by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success.
 */

int csoundDeleteAllConfigurationVariables(CSOUND *csound)
{
    int retval;
    retval = destroy_entire_db(local_cfg_db);
    local_cfg_db = NULL;
    return retval;
}

/**
 * Returns pointer to an error string constant for the specified
 * CSOUNDCFG error code. The string is not translated.
 */

PUBLIC const char *csoundCfgErrorCodeToString(int errcode)
{
    if (errcode > 0 || errcode < CSOUNDCFG_LASTERROR)
      return errmsg_list[1 - CSOUNDCFG_LASTERROR];      /* unknown */
    else
      return errmsg_list[(-errcode)];
}

