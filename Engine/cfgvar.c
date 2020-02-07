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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "csoundCore.h"
#include "cfgvar.h"
#include "namedins.h"

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

static int cfg_alloc_structure(CSOUND* csound,
                               csCfgVariable_t **ptr,
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
    pp = (void*) csound->Calloc(csound, (size_t) totBytes);
    if (UNLIKELY(pp == NULL))
      return CSOUNDCFG_MEMORY;
    //memset(pp, 0, (size_t) totBytes);
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
    int             retval;

    /* check if name is already in use */
    if (UNLIKELY(csoundQueryConfigurationVariable(csound, name) != NULL))
      return CSOUNDCFG_INVALID_NAME;
    /* if database is not allocated yet, create an empty one */
    if (csound->cfgVariableDB == NULL) {
      csound->cfgVariableDB = cs_hash_table_create(csound);
      if (UNLIKELY(csound->cfgVariableDB == NULL))
        return CSOUNDCFG_MEMORY;
    }
    /* allocate structure */
    retval =  cfg_alloc_structure(csound, &pp, name, p, type, flags, min, max,
                                  shortDesc, longDesc);
    if (UNLIKELY(retval != CSOUNDCFG_SUCCESS))
      return retval;
    /* link into database */

    cs_hash_table_put(csound, csound->cfgVariableDB, (char*)name, pp);

    /* report success */
    return CSOUNDCFG_SUCCESS;
}

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
        if (strcmp(value, "0") == 0   ||
            strcmp(value, "no") == 0  || strcmp(value, "No") == 0  ||
            strcmp(value, "NO") == 0  || strcmp(value, "off") == 0 ||
            strcmp(value, "Off") == 0 || strcmp(value, "OFF") == 0 ||
            strcmp(value, "false") == 0 || strcmp(value, "False") == 0 ||
            strcmp(value, "FALSE") == 0)
          *(pp->b.p) = 0;
        else if (strcmp(value, "1") == 0   ||
                 strcmp(value, "yes") == 0 || strcmp(value, "Yes") == 0 ||
                 strcmp(value, "YES") == 0 || strcmp(value, "on") == 0  ||
                 strcmp(value, "On") == 0  || strcmp(value, "ON") == 0  ||
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

/**
 * Return pointer to the configuration variable of Csound instace 'csound'
 * with the specified name.
 * The return value may be NULL if the variable is not found in the database.
 */

PUBLIC csCfgVariable_t
    *csoundQueryConfigurationVariable(CSOUND *csound, const char *name)
{
    if (csound->cfgVariableDB == NULL) {
        return NULL;
    }
    return (csCfgVariable_t*) cs_hash_table_get(csound,
                                                csound->cfgVariableDB, (char*)name);
}

/* compare function for qsort() */

static int compare_func(const void *p1, const void *p2)
{
    return (int) strcmp((char*) ((*((csCfgVariable_t**) p1))->h.name),
                        (char*) ((*((csCfgVariable_t**) p2))->h.name));
}

/* create alphabetically sorted list of all entries in 'db' */

static csCfgVariable_t **list_db_entries(CSOUND* csound, CS_HASH_TABLE *db)
{
    csCfgVariable_t **lst;
    size_t          cnt;
    CONS_CELL*      values;

    values = cs_hash_table_values(csound, db);
    cnt = cs_cons_length(values);


    /* allocate memory for list */
    lst = (csCfgVariable_t**) csound->Malloc(csound, sizeof(csCfgVariable_t*)
                                             * (cnt + (size_t) 1));
    if (UNLIKELY(lst == NULL))
      return (csCfgVariable_t**) NULL;  /* not enough memory */
    /* create list */
    if (cnt) {
      cnt = 0;
        while (values != NULL) {
            lst[cnt++] = (csCfgVariable_t*)values->value;
            values = values->next;
        }
        qsort((void*) lst, cnt, sizeof(csCfgVariable_t*), compare_func);
    }

    lst[cnt] = (csCfgVariable_t*) NULL;
    /* return pointer to list */
    return lst;
}

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
    return (list_db_entries(csound, csound->cfgVariableDB));
}

/**
 * Release a configuration variable list previously returned
 * by csoundListGlobalConfigurationVariables() or
 * csoundListConfigurationVariables().
 */

PUBLIC void csoundDeleteCfgVarList(CSOUND* csound, csCfgVariable_t **lst)
{
    if (lst != NULL)
      csound->Free(csound, lst);
}

/* remove a configuration variable from 'db' */

static int remove_entry_from_db(CSOUND* csound, CS_HASH_TABLE *db, const char *name)
{
    csCfgVariable_t *pp = cs_hash_table_get(csound, db, (char*)name);

    if (UNLIKELY(pp == NULL))
        return CSOUNDCFG_INVALID_NAME;

    csound->Free(csound, pp);
    cs_hash_table_remove(csound, db, (char*)name);

    return CSOUNDCFG_SUCCESS;
}

/**
 * Remove the configuration variable of Csound instance 'csound' with the
 * specified name from the database. Plugins need not call this, as all
 * configuration variables are automatically deleted by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success, or
 * CSOUNDCFG_INVALID_NAME if the variable was not found.
 */

PUBLIC int csoundDeleteConfigurationVariable(CSOUND *csound, const char *name)
{
    return remove_entry_from_db(csound, csound->cfgVariableDB, name);
}

static int destroy_entire_db(CSOUND *csound, CS_HASH_TABLE *db)
{
    CONS_CELL *head, *current;
    if (db == NULL)
      return CSOUNDCFG_SUCCESS;

    head = current = cs_hash_table_values(csound, db);

    while (current != NULL) {
        if (current->value != NULL) {
             csound->Free(csound, current->value);
        }
        current = current->next;
    }

    cs_cons_free(csound, head);
    cs_hash_table_free(csound, db);

    return CSOUNDCFG_SUCCESS;
}

/**
 * Remove all configuration variables of Csound instance 'csound'
 * and free database. This function is called by csoundReset().
 * Return value is CSOUNDCFG_SUCCESS in case of success.
 */

int csoundDeleteAllConfigurationVariables(CSOUND *csound)
{
    int retval;
    retval = destroy_entire_db(csound, csound->cfgVariableDB);
    csound->cfgVariableDB = NULL;
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
