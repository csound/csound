/*
    cfgvar.h:

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

#ifndef CSOUND_CFGVAR_H
#define CSOUND_CFGVAR_H

#ifdef __cplusplus
extern "C" {
#endif

/* generic header structure */

typedef struct csCfgVariableHead_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    void            *p;               /* pointer to value                   */
    int             type;             /* type (e.g. CSOUNDCFG_INTEGER)      */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
} csCfgVariableHead_t;

/* int type */

typedef struct csCfgVariableInt_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    int             *p;               /* pointer to value                   */
    int             type;             /* type (CSOUNDCFG_INTEGER)           */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
    int             min;              /* minimum allowed value              */
    int             max;              /* maximum allowed value              */
} csCfgVariableInt_t;

/* boolean type (int with a value of 0 or 1) */

typedef struct csCfgVariableBoolean_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    int             *p;               /* pointer to value                   */
    int             type;             /* type (CSOUNDCFG_BOOLEAN)           */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
} csCfgVariableBoolean_t;

/* float type */

typedef struct csCfgVariableFloat_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    float           *p;               /* pointer to value                   */
    int             type;             /* type (CSOUNDCFG_FLOAT)             */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
    float           min;              /* minimum allowed value              */
    float           max;              /* maximum allowed value              */
} csCfgVariableFloat_t;

/* double type */

typedef struct csCfgVariableDouble_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    double          *p;               /* pointer to value                   */
    int             type;             /* type (CSOUNDCFG_DOUBLE)            */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
    double          min;              /* minimum allowed value              */
    double          max;              /* maximum allowed value              */
} csCfgVariableDouble_t;

/* MYFLT (float or double) type */

typedef struct csCfgVariableMYFLT_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    MYFLT           *p;               /* pointer to value                   */
    int             type;             /* type (CSOUNDCFG_MYFLT)             */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
    MYFLT           min;              /* minimum allowed value              */
    MYFLT           max;              /* maximum allowed value              */
} csCfgVariableMYFLT_t;

/* string type */

typedef struct csCfgVariableString_s {
    union csCfgVariable_u *nxt;       /* pointer to next structure in chain */
    unsigned char   *name;            /* name of the variable               */
    char            *p;               /* value: array of 'maxlen' chars     */
    int             type;             /* type (CSOUNDCFG_STRING)            */
    int             flags;            /* bitwise OR of flags                */
    unsigned char   *shortDesc;       /* short description (NULL if none)   */
    unsigned char   *longDesc;        /* long description (NULL if none)    */
    int             maxlen;           /* maximum length + 1                 */
} csCfgVariableString_t;

/* union of all variable types */

typedef union csCfgVariable_u {
  csCfgVariableHead_t       h;
  csCfgVariableInt_t        i;
  csCfgVariableBoolean_t    b;
  csCfgVariableFloat_t      f;
  csCfgVariableDouble_t     d;
  csCfgVariableMYFLT_t      m;
  csCfgVariableString_t     s;
} csCfgVariable_t;

/* types */

#define CSOUNDCFG_INTEGER   1
#define CSOUNDCFG_BOOLEAN   2
#define CSOUNDCFG_FLOAT     3
#define CSOUNDCFG_DOUBLE    4
#define CSOUNDCFG_MYFLT     5
#define CSOUNDCFG_STRING    6

/* flags */

#define CSOUNDCFG_POWOFTWO  0x00000001

/* error codes */

#define CSOUNDCFG_SUCCESS           0
#define CSOUNDCFG_INVALID_NAME      -1
#define CSOUNDCFG_INVALID_TYPE      -2
#define CSOUNDCFG_INVALID_FLAG      -3
#define CSOUNDCFG_NULL_POINTER      -4
#define CSOUNDCFG_TOO_HIGH          -5
#define CSOUNDCFG_TOO_LOW           -6
#define CSOUNDCFG_NOT_POWOFTWO      -7
#define CSOUNDCFG_INVALID_BOOLEAN   -8
#define CSOUNDCFG_MEMORY            -9
#define CSOUNDCFG_STRING_LENGTH     -10

#define CSOUNDCFG_LASTERROR         -10

/* -------- interface functions -------- */

  PUBLIC int
    csoundCreateGlobalConfigurationVariable(const char *name,
                                            void *p, int type, int flags,
                                            void *min, void *max,
                                            const char *shortDesc,
                                            const char *longDesc);

  PUBLIC int
    csoundCreateConfigurationVariable(void *csound, const char *name,
                                      void *p, int type, int flags,
                                      void *min, void *max,
                                      const char *shortDesc,
                                      const char *longDesc);

  PUBLIC int csoundCopyGlobalConfigurationVariable(void *csound,
                                                   const char *name, void *p);

  int csoundCopyGlobalConfigurationVariables(void *csound);

  PUBLIC int csoundSetGlobalConfigurationVariable(const char *name,
                                                  void *value);

  PUBLIC int csoundSetConfigurationVariable(void *csound,
                                            const char *name, void *value);

  PUBLIC int csoundParseGlobalConfigurationVariable(const char *name,
                                                    const char *value);

  PUBLIC int csoundParseConfigurationVariable(void *csound, const char *name,
                                              const char *value);

  PUBLIC csCfgVariable_t
    *csoundQueryGlobalConfigurationVariable(const char *name);

  PUBLIC csCfgVariable_t
    *csoundQueryConfigurationVariable(void *csound, const char *name);

  PUBLIC csCfgVariable_t **csoundListGlobalConfigurationVariables(void);

  PUBLIC csCfgVariable_t **csoundListConfigurationVariables(void *csound);

  PUBLIC int csoundDeleteGlobalConfigurationVariable(const char *name);

  PUBLIC int csoundDeleteConfigurationVariable(void *csound, const char *name);

  int csoundDeleteAllGlobalConfigurationVariables(void);

  int csoundDeleteAllConfigurationVariables(void *csound);

  PUBLIC char *csoundCfgErrorCodeToString(int errcode);

#ifdef __cplusplus
};
#endif

#endif  /* CSOUND_CFGVAR_H */

