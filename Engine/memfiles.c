/*
    memfiles.c:

    Copyright (C) 1991, 2001 Barry Vercoe, John ffitch, Richard Dobson
              (C) 2005 Istvan Varga

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

#include "csoundCore.h"     /*                              MEMFILES.C      */
#include "csound.h"

static int Load_File_(void *csound,
                      const char *filnam, char **allocp, long *len)
{
    FILE *f;

    *allocp = NULL;
    f = fopen(filnam, "rb");
    if (f == NULL)                              /* if cannot open the file */
      return 1;                                 /*    return 1             */
    fseek(f, 0L, SEEK_END);                     /* else get its length     */
    *len = (long) ftell(f);
    fseek(f, 0L, SEEK_SET);
    if (*len < 1L)
      goto err_return;
    *allocp = mmalloc(csound, (size_t) (*len)); /*   alloc as reqd     */
    if (fread(*allocp, (size_t) 1,              /*   read file in      */
              (size_t) (*len), f) != (size_t) (*len))
      goto err_return;
    fclose(f);                                  /*   and close it      */
    return 0;                                   /*   return 0 for OK   */

 err_return:
    if (*allocp != NULL) {
      mfree(csound, *allocp);
      *allocp = NULL;
    }
    fclose(f);
    return 1;
}

MEMFIL *ldmemfile(void *csound, const char *filnam)
{                               /* read an entire file into memory and log it */
    MEMFIL  *mfp, *last = NULL; /* share the file with all subsequent requests*/
    char    *allocp;            /* if not fullpath, look in current directory,*/
    long    len;                /*   then SADIR (if defined).                 */
    char    *pathnam;           /* Used by adsyn, pvoc, and lpread            */

    mfp = ((ENVIRON*) csound)->memfiles;
    while (mfp != NULL) {                               /* Checking chain */
      if (strcmp(mfp->filename, filnam) == 0)           /*   if match     */
        return mfp;                                     /*   we have it   */
      last = mfp;
      mfp = mfp->next;
    }
    /* Add new file description */
    mfp = (MEMFIL*) mcalloc(csound, sizeof(MEMFIL));
    if (last != NULL)
      last->next = mfp;
    else
      ((ENVIRON*) csound)->memfiles = mfp;
    mfp->next = NULL;
    strcpy(mfp->filename, filnam);

    pathnam = csoundFindInputFile(csound, filnam, "SADIR");
    if (pathnam == NULL) {
      csoundMessage(csound, Str("cannot load %s\n"), filnam);
      delete_memfile(csound, filnam);
      return NULL;
    }
    if (Load_File_(csound, pathnam, &allocp, &len) != 0) {  /* loadfile */
      csoundMessage(csound, Str("cannot load %s, or SADIR undefined\n"),
                            pathnam);
      mfree(csound, pathnam);
      delete_memfile(csound, filnam);
      return NULL;
    }
    /* init the struct */
    mfp->beginp = allocp;
    mfp->endp = allocp + len;
    mfp->length = len;
    csoundMessage(csound, Str("file %s (%ld bytes) loaded into memory\n"),
                          pathnam, len);
    mfree(csound, pathnam);
    return mfp;                                          /* rtn new slotadr */
}

/* clear the memfile array, & free all allocated space */

void rlsmemfiles(void *csound)
{
    MEMFIL  *mfp = ((ENVIRON*) csound)->memfiles, *nxt;

    while (mfp != NULL) {
      nxt = mfp->next;
      mfree(csound, mfp->beginp);       /*   free the space */
      mfree(csound, mfp);
      mfp = nxt;
    }
    ((ENVIRON*) csound)->memfiles = NULL;
}

int delete_memfile(void *csound, const char *filnam)
{
    MEMFIL  *mfp, *prv;

    prv = NULL;
    mfp = ((ENVIRON*) csound)->memfiles;
    while (mfp != NULL) {
      if (strcmp(mfp->filename, filnam) == 0)
        break;
      prv = mfp;
      mfp = mfp->next;
    }
    if (mfp == NULL)
      return -1;
    if (prv == NULL)
      ((ENVIRON*) csound)->memfiles = mfp->next;
    else
      prv->next = mfp->next;
    mfree(csound, mfp->beginp);
    mfree(csound, mfp);
    return 0;
}

 /* ------------------------------------------------------------------------ */

/* RWD 8:2001 (maybe temporary) external memfile support for pvocex */

int find_memfile(void *csound, const char *fname, MEMFIL **pp_mfp)
{
    MEMFIL *mfp = ((ENVIRON*) csound)->rwd_memfiles;
    while (mfp != NULL) {
      if (strcmp(mfp->filename, fname) == 0) {
        *pp_mfp = mfp;
        return 1;
      }
      mfp = mfp->next;
    }
    return 0;
}

void add_memfil(void *csound, MEMFIL *mfp)
{
    if (((ENVIRON*) csound)->rwd_memfiles == NULL)
      ((ENVIRON*) csound)->rwd_memfiles = mfp;
    else {
      /* cheeky! add it at the top */
      /* umm, I hope this doesn't break anything..... */
      mfp->next = ((ENVIRON*) csound)->rwd_memfiles;
      ((ENVIRON*) csound)->rwd_memfiles = mfp;
    }
}

