/*
    sfheader.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, matt ingalls

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

#include "cs.h"                         /*             SFHEADER.C       */
#include "soundio.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

extern OPARMS O;

#ifdef SFIRCAM

#include "sfheader.h"
static  char    *incodbeg,  *incodend;   /* re-defined by each readheader */
static  char    *outcodbeg, *outcodend;  /* defined during writeheader */

typedef struct {                  /* header for each sfcode block */
        short   type;
        short   blksiz;
} CODE_HDR;

static short codblksiz[] = {   sizeof(CODE_HDR),
                               sizeof(CODE_HDR) + sizeof(SFMAXAMP),
                               sizeof(CODE_HDR) + sizeof(SFAUDIOENCOD),
                               sizeof(CODE_HDR) + sizeof(SFPVDATA),
                               sizeof(CODE_HDR) + sizeof(SFCOMMENT)  };

static void putendcode(char *cp)
{
        CODE_HDR *cdhp = (CODE_HDR *)cp;
        cdhp->type = SF_END;
        cdhp->blksiz = sizeof(CODE_HDR);  /* header, no body */
}

/* These next rtn ptr to beg of struct reqd (not the CODE_HDR preceding it).
   SR, NCHNLS, magic number, bytes/chan, are NOT coded via these routines. */

char *findsfcode(int ctype)     /* locate start of sfcode in current in_header */
                                /*    incodbeg,incodend prev set by readheader */
{                               /*      used here,  & by ugens8.c (PVOC)       */
        char     *cp;
        CODE_HDR *cdhp;

        if (ctype <= 0 || ctype > SF_CODMAX)
            die(Str("illegal sfcode type"));
        for (cp = incodbeg; cp < incodend;) { /* starting from beg codespace */
            cdhp = (CODE_HDR *)cp;
            if (cdhp->type == ctype)             /* if find required code */
                return(cp + sizeof(CODE_HDR));   /*   return ptr to data  */
            if (cdhp->type == SF_END)            /* cannot find -- exit    */
                break;
            if (cdhp->blksiz <= 0                /* if false-sized struct or */
             || (cp += cdhp->blksiz) < incodbeg) {/* wrap-around from bad hdr */
                err_printf(Str("sfheader codes corrupted\n"));    /* complain */
                break;
            }
        }
        return(NULL);                        /* no-find: return NULL pointer */
}

char *creatsfcode(int ctype)    /* add a new sfcode struct to current out_header */
                                /*   outcodbeg,outcodend prev set by writeheader */
{
        char     *cp;
        CODE_HDR *cdhp;

        if (ctype <= 0 || ctype > SF_CODMAX)
            die(Str("illegal sfcode type"));
        for (cp=outcodbeg; cp<outcodend; ) { /* starting from beg codespace  */
            cdhp = (CODE_HDR *)cp;
            if (cdhp->type == SF_END) {             /* if find end code      */
                cdhp->type = ctype;                 /*   redo as newtyp hdr  */
                cdhp->blksiz = codblksiz[ctype];
                putendcode(cp+cdhp->blksiz);        /*   reconstruct endcode */
                return(cp+sizeof(CODE_HDR));        /*   & rtn newtyp datptr */
            }
            if (cdhp->blksiz <= 0                /* if false-sized struct or */
             ||(cp += cdhp->blksiz) < outcodbeg) {/* wrap-around from bad hdr */
                err_printf(Str("sfheader codes corrupted\n"));    /* complain */
                break;
            }
        }
        return(NULL);                       /* bad ptrs: return NULL pointer */
}

#endif

