/*
    diskin.c:

    Copyright (C) 1998, 2001 matt ingalls, Richard Dobson, John ffitch
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

#include "cs.h"
#include "soundio.h"
#include "diskin2.h"

#include <sndfile.h>
#include <math.h>

extern char *type2string(int x);

int newsndinset(ENVIRON *csound, DISKIN2 *p)    /* init routine for diskin   */
{
    MYFLT tmp1, tmp2, tmp3;
    MYFLT *saved_ptr;
    int   retval;

    /* this hack is safe to do because the variables are never accessed */
    /* at performance time */
    tmp1 = FL(2.0);         /* iWinSize */
    tmp2 = FL(4096.0);      /* iBufSize */
    tmp3 = *(p->iWinSize);  /* iSkipInit */
    saved_ptr = p->iWinSize;
    p->iWinSize = &tmp1;
    p->iBufSize = &tmp2;
    p->iSkipInit = &tmp3;
    retval = diskin2_init(csound, p);
    /* restore original pointers */
    p->iWinSize = saved_ptr;
    p->iBufSize = NULL;
    p->iSkipInit = NULL;
    return retval;
}

/* RWD:DBFS: NB: thse funcs all supposed to write to a 'raw' file, so
   what will people want for 0dbfs handling? really need to update
   opcode with more options. */

int sndo1set(ENVIRON *csound, SNDOUT *p) /* init routine for instr soundout   */
{
    int    soutfd, filno;
    char   *sfname, sndoutname[128];
    SF_INFO sfinfo;
    SNDFILE *outfile;

    if (p->c.fdch.fd != NULL)   return OK;  /* if file already open, rtn  */
    if (*p->c.ifilcod == SSTRCOD)
      strcpy(sndoutname, unquote(p->STRARG));
    else if ((filno = (int)*p->c.ifilcod) <= strsmax && strsets != NULL &&
             strsets[filno])
      strcpy(sndoutname, strsets[filno]);
    else
      sprintf(sndoutname,"soundout.%d", filno);
    sfname = sndoutname;
    if ((soutfd = openout(sfname, 1)) < 0) {   /* if openout successful */
      sprintf(errmsg,Str("soundout cannot open %s"), sfname);
      goto errtn;
    }
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (esr + FL(0.5));
    sfinfo.channels = 1;
    p->c.filetyp = TYP_RAW;
    switch ((int) (*(p->c.iformat) + FL(0.5))) {
      case 0: p->c.format = O.outformat; break;
      case 1: p->c.format = AE_CHAR; break;
      case 4: p->c.format = AE_SHORT; break;
      case 5: p->c.format = AE_LONG; break;
      case 6: p->c.format = AE_FLOAT; break;
      default:
        sprintf(errmsg, Str("soundout: invalid sample format: %d"),
                        (int) (*(p->c.iformat) + FL(0.5)));
        goto errtn;
    }
    sfinfo.format = type2sf(p->c.filetyp)|format2sf(p->c.format);
    sfinfo.sections = 0;
    sfinfo.seekable = 0;
    outfile = sf_open_fd(soutfd, SFM_WRITE, &sfinfo, SF_TRUE);
    if (p->c.format != AE_FLOAT)
      sf_command(outfile, SFC_SET_CLIPPING, NULL, SF_TRUE);
#ifdef USE_DOUBLE
    sf_command(outfile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
    sf_command(outfile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    sfname = retfilnam;
    printf(Str("opening %s outfile %s\n"), type2string(p->c.filetyp), sfname);
    p->c.outbufp = p->c.outbuf;             /* fix - isro 20-11-96 */
    p->c.bufend = p->c.outbuf + SNDOUTSMPS; /* fix - isro 20-11-96 */
    p->c.fdch.fd = outfile; /* WRONG */     /*     store & log the fd     */
    fdrecord(&p->c.fdch);                   /*     instr will close later */
    return OK;
 errtn:
    return initerror(errmsg);               /* else just print the errmsg */
}

int soundout(ENVIRON *csound, SNDOUT *p)
{
    MYFLT  *outbufp, *asig;
    int    nn, nsamps, ospace;

    asig = p->asig;
    outbufp = p->c.outbufp;
    nsamps = ksmps;
    ospace = (p->c.bufend - outbufp);
 nchk:
    if ((nn = nsamps) > ospace)
      nn = ospace;
    nsamps -= nn;
    ospace -= nn;
    memmove(outbufp, asig, nn*sizeof(MYFLT));
    outbufp += nn; asig += nn;
    if (!ospace) {              /* when buf is full  */
      sf_write_MYFLT(p->c.fdch.fd, p->c.outbuf, p->c.bufend - p->c.outbuf);
      outbufp = p->c.outbuf;
      ospace = SNDOUTSMPS;
      if (nsamps) goto nchk;    /*   chk rem samples */
    }
    p->c.outbufp = outbufp;
    return OK;
}

