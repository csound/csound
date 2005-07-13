/*
    utility.c:

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
#include "cs_util.h"

typedef struct csUtility_s {
    char                *name;
    struct csUtility_s  *nxt;
    int                 (*UtilFunc)(void*, int, char**);
    char                *desc;
} csUtility_t;

static const char list_var[] = "utilities::list";

PUBLIC int csoundAddUtility(void *csound_, const char *name,
                            int (*UtilFunc)(void*, int, char**))
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p;

    if (csound == NULL || name == NULL || name[0] == '\0' || UtilFunc == NULL)
      return -1;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    if (p != NULL) {
      do {
        if (strcmp(p->name, name) == 0)
          return -1;    /* name is already in use */
        if (p->nxt == NULL)
          break;
        p = p->nxt;
      } while (1);
      p->nxt = csound->Malloc(csound, sizeof(csUtility_t));
      p = p->nxt;
    }
    else {
      if (csound->CreateGlobalVariable(csound, list_var,
                                               sizeof(csUtility_t)) != 0)
        return -1;
      p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    }
    p->name = csound->Malloc(csound, strlen(name) + 1);
    strcpy(p->name, name);
    p->nxt = NULL;
    p->UtilFunc = UtilFunc;
    p->desc = NULL;
    return 0;
}

PUBLIC int csoundRunUtility(void *csound_, const char *name,
                            int argc, char **argv)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p;
    char        **lst;

    if (csound == NULL)
      return -1;
    if (name == NULL || name[0] == '\0')
      goto notFound;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    do {
      if (p == NULL)
        goto notFound;
      if (strcmp(p->name, name) == 0)
        break;
      p = p->nxt;
    } while (1);
    csound->Message(csound, Str("util %s:\n"), name);
    return (p->UtilFunc(csound, argc, argv));

 notFound:
    csound->MessageS(csound, CSOUNDMSG_ERROR, Str("Error: utility "));
    if (name != NULL && name[0] != '\0')
      csound->MessageS(csound, CSOUNDMSG_ERROR, "'%s' ", name);
    csound->MessageS(csound, CSOUNDMSG_ERROR, Str("not found\n"));
    lst = csound->ListUtilities(csound);
    if (lst != NULL && lst[0] != NULL) {
      int i;
      csound->Message(csound, Str("The available utilities are:\n"));
      for (i = 0; lst[i] != NULL; i++) {
        char  *desc = csound->GetUtilityDescription(csound, lst[i]);
        if (desc != NULL)
          csound->Message(csound, "    %s\t%s\n", lst[i], Str(desc));
        else
          csound->Message(csound, "    %s\n", lst[i]);
      }
    }
    if (lst != NULL)
      csound->Free(csound, lst);
    return -1;
}

static int cmp_func(const void *a, const void *b)
{
    return strcmp(*((char**) a), *((char**) b));
}

/**
 * Returns a NULL terminated list of registered utility names.
 * The caller is responsible for freeing the returned array (with mfree(),
 * or csound->Free()), however, the names should not be freed.
 * The return value may be NULL in case of an error.
 */

PUBLIC char **csoundListUtilities(void *csound_)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p = (csUtility_t*) csoundQueryGlobalVariable(csound, list_var);
    char        **lst;
    int         utilCnt = 0;

    /* find out the number of utilities */
    while (p != NULL)
      p = p->nxt, utilCnt++;
    /* allocate list */
    lst = (char**) csound->Malloc(csound, sizeof(char*) * (utilCnt + 1));
    if (lst == NULL)
      return NULL;
    /* store pointers to utility names */
    utilCnt = 0;
    p = (csUtility_t*) csound->QueryGlobalVariable(csound, list_var);
    while (p != NULL) {
      lst[utilCnt++] = (char*) p->name;
      p = p->nxt;
    }
    lst[utilCnt] = NULL;
    qsort(lst, utilCnt, sizeof(char*), cmp_func);
    /* return with pointer to list */
    return lst;
}

/**
 * Set description text for the specified utility.
 * Returns zero on success.
 */

PUBLIC int csoundSetUtilityDescription(void *csound_, const char *utilName,
                                                      const char *utilDesc)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p = (csUtility_t*) csoundQueryGlobalVariable(csound, list_var);
    char        *desc = NULL;

    /* check for valid parameters */
    if (utilName == NULL)
      return CSOUND_ERROR;
    /* find utility in database */
    while (p != NULL && strcmp(p->name, utilName) != 0)
      p = p->nxt;
    if (p == NULL)
      return CSOUND_ERROR;      /* not found */
    /* copy description text */
    if (utilDesc != NULL && utilDesc[0] != '\0') {
      desc = (char*) csound->Malloc(csound, strlen(utilDesc) + 1);
      if (desc == NULL)
        return CSOUND_MEMORY;
      strcpy(desc, utilDesc);
    }
    if (p->desc != NULL)
      csound->Free(csound, p->desc);
    p->desc = desc;
    /* report success */
    return CSOUND_SUCCESS;
}

/**
 * Get utility description.
 * Returns NULL if the utility was not found, or it has no description,
 * or an error occured.
 */

PUBLIC char *csoundGetUtilityDescription(void *csound_, const char *utilName)
{
    ENVIRON     *csound = (ENVIRON*) csound_;
    csUtility_t *p = (csUtility_t*) csoundQueryGlobalVariable(csound, list_var);

    /* check for valid parameters */
    if (utilName == NULL)
      return NULL;
    /* find utility in database */
    while (p != NULL && strcmp(p->name, utilName) != 0)
      p = p->nxt;
    if (p == NULL)
      return NULL;      /* not found */
    /* return with utility description (if any) */
    return p->desc;
}

/**
 * Main function for stand-alone utilities.
 */

PUBLIC int csoundUtilMain(const char *name, int argc, char **argv)
{
    volatile  void  *csound;
    volatile  int   n;

    csound = (void*) csoundCreate(NULL);
    if (csound == NULL)
      return -1;
    if ((n = setjmp(((ENVIRON*) csound)->exitjmp)) != 0)
      return (n - CSOUND_EXITJMP_SUCCESS);
    if ((n = csoundPreCompile((ENVIRON*) csound)) == 0) {
      ((ENVIRON*) csound)->orchname = (char*) name;
      ((ENVIRON*) csound)->scorename = (char*) name;
      n = ((ENVIRON*) csound)->Utility((ENVIRON*) csound, name, argc, argv);
    }
    csoundDestroy((ENVIRON*) csound);

    return n;
}

