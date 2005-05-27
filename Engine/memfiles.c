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
#include "soundio.h"

extern  const   unsigned char   strhash_tabl_8[256];    /* namedins.c */
#define name_hash(x,y) (strhash_tabl_8[(unsigned char) x ^ (unsigned char) y])

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

 /* ------------------------------------------------------------------------ */

/**
 * Load an entire sound file into memory.
 * 'fileName' is the file name (searched in the current directory first,
 * then search path defined by SSDIR, then SFDIR), and sfinfo (optional,
 * may be NULL) stores the default parameters for opening a raw file.
 * On success, a pointer to an SNDMEMFILE structure (see csoundCore.h) is
 * returned, and sound file parameters are stored in sfinfo (assuming that
 * it is not NULL).
 * Multiple calls of csoundLoadSoundFile() with the same file name will
 * share the same SNDMEMFILE structure, and the file is loaded only once
 * from disk.
 * The return value is NULL if an error occurs (the contents of sfinfo may
 * be undefined in this case).
 */

PUBLIC SNDMEMFILE *csoundLoadSoundFile(void *csound_, const char *fileName,
                                                      SF_INFO *sfinfo)
{
    ENVIRON       *csound = (ENVIRON*) csound_;
    SNDFILE       *sf;
    void          *fd;
    SNDMEMFILE    *p = NULL;
    SF_INFO       tmp;
    unsigned char *c, h;

    if (fileName == NULL || fileName[0] == '\0')
      return NULL;
    /* check if file is already loaded */
    c = (unsigned char*) fileName - 1;
    h = (unsigned char) 0;
    while (*++c) h = name_hash(h, *c);
    if (csound->sndmemfiles != NULL) {
      p = ((SNDMEMFILE**) csound->sndmemfiles)[(int) h];
      while (p != NULL && strcmp(p->name, fileName) != 0)
        p = p->nxt;
    }
    else {
      int i;
      /* if no files loaded yet, allocate table */
      csound->sndmemfiles = csound->Malloc(csound, sizeof(SNDMEMFILE*) * 256);
      for (i = 0; i < 256; i++)
        ((SNDMEMFILE**) csound->sndmemfiles)[i] = NULL;
    }
    if (p != NULL) {
      /* if file was loaded earlier: */
      if (sfinfo != NULL) {
        memset(sfinfo, 0, sizeof(SF_INFO));
        sfinfo->frames = (sf_count_t) p->nFrames;
        sfinfo->samplerate = ((int) p->sampleRate + 0.5);
        sfinfo->channels = p->nChannels;
        sfinfo->format = FORMAT2SF(p->sampleFormat) | TYPE2SF(p->fileType);
      }
      return p;
    }
    /* open file */
    if (sfinfo == NULL) {
      memset(&tmp, 0, sizeof(SF_INFO));
      sfinfo = &tmp;
    }
    fd = csound->FileOpen(csound, &sf, CSFILE_SND_R, fileName, sfinfo,
                                  "SFDIR;SSDIR");
    if (fd == NULL) {
      csound->MessageS(csound, CSOUNDMSG_ERROR, Str("csoundLoadSoundFile(): "
                                                    "failed to open '%s'\n"),
                                                fileName);
      return NULL;
    }
    p = (SNDMEMFILE*)
            csound->Malloc(csound, sizeof(SNDMEMFILE)
                                   + (size_t) sfinfo->frames * sizeof(float));
    if (p == NULL) {
      csound->FileClose(csound, fd);
      return NULL;
    }
    /* set parameters */
    p->name = (char*) csound->Malloc(csound, strlen(fileName) + 1);
    strcpy(p->name, fileName);
    p->fullName = (char*) csound->Malloc(csound,
                                         strlen(csound->GetFileName(fd)) + 1);
    strcpy(p->fullName, csound->GetFileName(fd));
    p->sampleRate = (double) sfinfo->samplerate;
    p->nFrames = (size_t) sfinfo->frames;
    p->nChannels = sfinfo->channels;
    p->sampleFormat = SF2FORMAT(sfinfo->format);
    p->fileType = SF2TYPE(sfinfo->format);
    p->loopMode = 0;    /* FIXME: not implemented yet */
    p->startOffs = 0.0;
    p->loopStart = 0.0;
    p->loopEnd = 0.0;
    p->baseFreq = 1.0;
    p->scaleFac = 1.0;
    p->nxt = ((SNDMEMFILE**) csound->sndmemfiles)[(int) h];
    if ((size_t) sf_readf_float(sf, &(p->data[0]), (sf_count_t) p->nFrames)
        != p->nFrames) {
      csound->FileClose(csound, fd);
      csound->Free(csound, p->name);
      csound->Free(csound, p->fullName);
      csound->Free(csound, p);
      csound->MessageS(csound, CSOUNDMSG_ERROR, Str("csoundLoadSoundFile(): "
                                                    "error reading '%s'\n"),
                                                fileName);
      return NULL;
    }
    p->data[p->nFrames] = 0.0f;
    csound->FileClose(csound, fd);
    csound->Message(csound, Str("File '%s' (sr = %d Hz, %d channel(s), %lu "
                                "sample frames) loaded into memory\n"),
                            p->fullName, (int) sfinfo->samplerate,
                            (int) sfinfo->channels,
                            (unsigned long) sfinfo->frames);
    /* link into database */
    ((SNDMEMFILE**) csound->sndmemfiles)[(int) h] = p;
    /* return with pointer to file structure */
    return p;
}

