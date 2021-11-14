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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include <setjmp.h>
#include "corfile.h"

typedef struct csUtility_s {
    char                *name;
    struct csUtility_s  *nxt;
    int                 (*UtilFunc)(CSOUND*, int, char**);
    char                *desc;
} csUtility_t;

int csoundAddUtility(CSOUND *csound, const char *name,
                                     int (*UtilFunc)(CSOUND*, int, char**))
{
    csUtility_t *p;
    /* csound->Message(csound, "csoundAddUtility: name: %s  function: 0x%p\n",
                       name, UtilFunc); */
    if (UNLIKELY(csound == NULL || name == NULL ||
                 name[0] == '\0' || UtilFunc == NULL))
      return -1;
    p = (csUtility_t*) csound->utility_db;
    if (LIKELY(p != NULL)) {
      do {
        if (UNLIKELY(strcmp(p->name, name) == 0))
          return -1;    /* name is already in use */
        if (p->nxt == NULL)
          break;
        p = p->nxt;
      } while (1);
      p->nxt = csound->Malloc(csound, sizeof(csUtility_t));
      p = p->nxt;
    }
    else {
      csound->utility_db = csound->Calloc(csound, sizeof(csUtility_t));
      p = (csUtility_t*) csound->utility_db;
    }
    p->name = csound->Malloc(csound, strlen(name) + 1);
    strcpy(p->name, name);
    p->nxt = NULL;
    p->UtilFunc = UtilFunc;
    p->desc = NULL;
    return 0;
}

void print_csound_version(CSOUND *csound);
void print_sndfile_version(CSOUND* csound);

PUBLIC int csoundRunUtility(CSOUND *csound, const char *name,
                            int argc, char **argv)
{
    csUtility_t   *p;
    char          **lst;
    volatile void *saved_exitjmp;
    volatile int  n;

    if (UNLIKELY(csound == NULL))
      return -1;

   /* VL - 08-07-21 messages moved here so we can switch them off */
    if(csound->oparms->msglevel || csound->oparms->odebug) {
      print_csound_version(csound);
      print_sndfile_version(csound);
    }

    saved_exitjmp = (void*) csound->Malloc(csound, sizeof(jmp_buf));
    if (UNLIKELY(saved_exitjmp == NULL))
      return -1;
    memcpy((void*) saved_exitjmp, (void*) &(csound->exitjmp), sizeof(jmp_buf));

    if (UNLIKELY((n = setjmp(csound->exitjmp)) != 0)) {
      n = (n - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS;
      goto err_return;
    }

    if (UNLIKELY(name == NULL || name[0] == '\0'))
      goto notFound;
    p = (csUtility_t*) csound->utility_db;
    while (1) {
      if (UNLIKELY(p == NULL))
        goto notFound;
      if (strcmp(p->name, name) == 0)
        break;
      p = p->nxt;
    }
    csound->engineStatus |= CS_STATE_UTIL;
    csound->scorename = csound->orchname = (char*) name;    /* needed ? */
    csound->Message(csound, Str("util %s:\n"), name);
    n = p->UtilFunc(csound, argc, argv);
    goto err_return;

 notFound:
    if (name != NULL && name[0] != '\0') {
      print_opcodedir_warning(csound);
      csound->ErrorMsg(csound, Str("Error: utility '%s' not found"), name);
    }
    else
      csound->ErrorMsg(csound, Str("Error: utility not found"));
    lst = csound->ListUtilities(csound);
    if (lst != NULL && lst[0] != NULL) {
      int i;
      csound->Message(csound, Str("The available utilities are:\n"));
      for (i = 0; lst[i] != NULL; i++) {
        const char *desc = csound->GetUtilityDescription(csound, lst[i]);
        if (desc != NULL)
          csound->Message(csound, "    %s\t%s\n", lst[i], Str(desc));
        else
          csound->Message(csound, "    %s\n", lst[i]);
      }
    }
    csoundDeleteUtilityList(csound, lst);
    n = -1;
 err_return:
    memcpy((void*) &(csound->exitjmp), (void*) saved_exitjmp, sizeof(jmp_buf));
    csound->Free(csound, (void*) saved_exitjmp);
    return n;
}

static int cmp_func(const void *a, const void *b)
{
    return strcmp(*((char**) a), *((char**) b));
}

/**
 * Returns a NULL terminated list of registered utility names.
 * The caller is responsible for freeing the returned array with
 * csoundDeleteUtilityList(), however, the names should not be
 * changed or freed.
 * The return value may be NULL in case of an error.
 */

PUBLIC char **csoundListUtilities(CSOUND *csound)
{
    csUtility_t *p = (csUtility_t*) csound->utility_db;
    char        **lst;
    int         utilCnt = 0;

    /* find out the number of utilities */
    while (p != NULL)
      p = p->nxt, utilCnt++;
    /* allocate list */
    lst = (char**) csound->Malloc(csound, sizeof(char*) * (utilCnt + 1));
    if (UNLIKELY(lst == NULL))
      return NULL;
    /* store pointers to utility names */
    utilCnt = 0;
    p = (csUtility_t*) csound->utility_db;
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
 * Releases an utility list previously returned by csoundListUtilities().
 */

PUBLIC void csoundDeleteUtilityList(CSOUND *csound, char **lst)
{
    if (lst != NULL)
      csound->Free(csound, lst);
}

/**
 * Set description text for the specified utility.
 * Returns zero on success.
 */

int csoundSetUtilityDescription(CSOUND *csound, const char *utilName,
                                                const char *utilDesc)
{
    csUtility_t *p = (csUtility_t*) csound->utility_db;
    char        *desc = NULL;

    /* check for valid parameters */
    if (UNLIKELY(utilName == NULL))
      return CSOUND_ERROR;
    /* find utility in database */
    while (p != NULL && strcmp(p->name, utilName) != 0)
      p = p->nxt;
    if (UNLIKELY(p == NULL))
      return CSOUND_ERROR;      /* not found */
    /* copy description text */
    if (utilDesc != NULL && utilDesc[0] != '\0') {
      desc = (char*) csound->Malloc(csound, strlen(utilDesc) + 1);
      if (UNLIKELY(desc == NULL))
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

PUBLIC const char *csoundGetUtilityDescription(CSOUND *csound,
                                               const char *utilName)
{
    csUtility_t *p = (csUtility_t*) csound->utility_db;

    /* check for valid parameters */
    if (UNLIKELY(utilName == NULL))
      return NULL;
    /* find utility in database */
    while (p != NULL && strcmp(p->name, utilName) != 0)
      p = p->nxt;
    if (UNLIKELY(p == NULL))
      return NULL;      /* not found */
    /* return with utility description (if any) */
    return (const char*) p->desc;
}

 /* ------------------------------------------------------------------------ */

/**
 * Sorts score file 'inFile' and writes the result to 'outFile'.
 * The Csound instance should be initialised with csoundPreCompile()
 * before calling this function, and csoundReset() should be called
 * after sorting the score to clean up. On success, zero is returned.
 */

PUBLIC int csoundScoreSort(CSOUND *csound, FILE *inFile, FILE *outFile)
{
    int   err;
    CORFIL *inf = corfile_create_w(csound);
    int c;
    if ((err = setjmp(csound->exitjmp)) != 0) {
      return ((err - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    while ((c=getc(inFile))!=EOF) corfile_putc(csound, c, inf);
    corfile_puts(csound, "\ne\n#exit\n", inf);
    corfile_rewind(inf);
    /* scsortstr() ignores the second arg - Jan 5 2012 */
    csound->scorestr = inf;
    scsortstr(csound, inf);
    while ((c=corfile_getc(csound->scstr))!=EOF)
      putc(c, outFile);
    corfile_rm(csound, &csound->scstr);
    return 0;
}

/**
 * Extracts from 'inFile', controlled by 'extractFile', and writes
 * the result to 'outFile'. The Csound instance should be initialised
 * with csoundPreCompile() before calling this function, and csoundReset()
 * should be called after score extraction to clean up.
 * The return value is zero on success.
 */
PUBLIC int csoundScoreExtract(CSOUND *csound,
                              FILE *inFile, FILE *outFile, FILE *extractFile)
{
    int   err;
    CORFIL *inf = corfile_create_w(csound);
    int c;
    if ((err = setjmp(csound->exitjmp)) != 0) {
      return ((err - CSOUND_EXITJMP_SUCCESS) | CSOUND_EXITJMP_SUCCESS);
    }
    while ((c=getc(inFile))!=EOF) corfile_putc(csound, c, inf);
    corfile_rewind(inf);
    scxtract(csound, inf, extractFile);
    while ((c=corfile_getc(csound->scstr))!=EOF)
      putc(c, outFile);
    corfile_rm(csound, &csound->scstr);
    return 0;
}
