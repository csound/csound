/*
    pvoc.c:

    Copyright (C) 1990 Dan Ellis, John ffitch

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

/***************************************************************\
*       pvoc.c                                                  *
*       file in/out functions for pvoc FFT files                *
*       'inspired' by the NeXT SND system                       *
*       01aug90 dpwe                                            *
\***************************************************************/

#include "sysdep.h"

#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif

#if defined(mac_classic) || defined(SYMANTEC) || defined(__FreeBSD__) || defined(__NetBSD__)
#  define READMODE "rb"
#  define WRITEMODE "wb+"
#else
#  ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#  endif
#  define READMODE "r"
#  define WRITEMODE "w+"
#endif /* SYMANTEC */

#include "cs.h"
#include "pvoc.h"

/* static variables */
static PVSTRUCT tmphdr;         /* scratch space for pre-load */
/* want to 'fill in' at      012345678901234567890... [20..27] */
#define USMINDX 20      /* where to write into above string ^ */

char *PVDataLoc(PVSTRUCT *phdr)
{
    return( ((char *)phdr)+phdr->headBsize );
}

int PVReadHdr(ENVIRON *csound, FILE *fil, PVSTRUCT *phdr)
    /* read just the header from a candidate pvoc file */
    /* returns PVE_RDERR if read fails (or file too small)
            or PVE_NPV   if magic number doesn't fit
            or PVE_OK     otherwise                     */
{
    size_t      num;

    phdr->magic = 0L;
    if (fseek(fil, 0L, SEEK_SET) != 0)
        return(PVE_RDERR);
    if ((num = fread((void *)phdr, (size_t)1, (size_t)sizeof(PVSTRUCT), fil))
                    < (size_t)sizeof(PVSTRUCT)) {
      csound->Message(csound, Str("PVRdH: wanted %d got %d\n"),
                      (size_t)sizeof(PVSTRUCT), num);
      return(PVE_RDERR);
    }
    if (phdr->magic != PVMAGIC)
        return(PVE_NPV);
    return(PVE_OK);
}

int PVWriteHdr(FILE *fil, PVSTRUCT *phdr)
{
    long        bSize;

    if (phdr->magic != PVMAGIC)
      return PVE_NPV;
    if (fseek(fil, 0L, SEEK_SET) != 0)
      return PVE_RDERR;
    bSize = phdr->headBsize;
    if (fwrite((void *)phdr, (size_t)1, (size_t)bSize, fil) < (size_t)bSize )
      return PVE_WRERR;
    return PVE_OK;
}

FILE *PVOpenAllocRdHdr(ENVIRON *csound, char *path, PVSTRUCT **phdr)
    /* read all of candidate header - variable length header */
{
    FILE        *pvf;
    long        hSize, rem;
    int         err = 0;

    if ((pvf = fopen(path,"r"))!= NULL) {
      if (PVReadHdr(csound, pvf, &tmphdr) == PVE_OK ) {
        hSize = tmphdr.headBsize;
        *phdr = (PVSTRUCT *)malloc((size_t)hSize);
        if ((*phdr)!=NULL) {
          **phdr = tmphdr;      /* copies elements of struct ?? */
          rem = hSize - sizeof(PVSTRUCT);
          if (rem > 0)
            fread((void *)((*phdr)+1),(size_t)1,(size_t)rem,pvf);
        }
        else
          err = 1;      /* malloc error */
      }
      else
        err = 1;                /* header read error - e.g. not pv file */
    }
    if (err) {
      fclose(pvf);
      pvf = NULL;
    }
    return(pvf);
}

FILE *PVOpenWrHdr(char *filename, PVSTRUCT *phdr)
{
    FILE        *fil = NULL;

    if (phdr->magic != PVMAGIC)
      return NULL;    /* PVE_NPV   */
    if ((fil = fopen(filename,WRITEMODE)) == NULL)
      return NULL;    /* PVE_NOPEN */
    if (PVWriteHdr(fil, phdr)!= PVE_OK ) {
      fclose(fil);
      return NULL;        /* PVE_WRERR */
    }
    return fil;
}

int PVWriteFile(char *filename, PVSTRUCT *phdr)
    /* write out a PVOC block in memory to a file
       returns PV_NOPEN  if cannot open file
               PV_NPV    if *phdr isn't magic
               PV_WRERR  if write fails  */
{
    FILE        *fil;
    int         err = PVE_OK;
    long        bSize;
    char        *buf;

    if (phdr->magic != PVMAGIC)
      return(PVE_NPV);
    if ((fil = PVOpenWrHdr(filename, phdr)) == NULL)
      return(PVE_NOPEN);
    if (phdr->dataBsize == PV_UNK_LEN)
      bSize = 0;
    else
      bSize = phdr->dataBsize;
    buf   = (char *)PVDataLoc(phdr);
    if (fwrite(buf, (size_t)1, (size_t)bSize, fil) < (size_t)bSize )
      err = PVE_WRERR;
    fclose(fil);
    return(err);
}

void PVCloseWrHdr(ENVIRON *csound, FILE *file, PVSTRUCT *phdr)
{
    long len;

    len = ftell(file);
    if (PVReadHdr(csound, file, &tmphdr) == PVE_OK ) {
      /* only if we can seek back */
      len -= tmphdr.headBsize;
      if (phdr == NULL) {
        tmphdr.dataBsize = len;
        PVWriteHdr(file, &tmphdr);
      }
      else {
        if (phdr->dataBsize == 0 || phdr->dataBsize == PV_UNK_LEN)
          phdr->dataBsize = len;
        PVWriteHdr(file, phdr);
      }
    }
}

