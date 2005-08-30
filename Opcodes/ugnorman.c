/*
    ugnorman.c:

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

/* ats-csound version 0.1
 * Mon May 10 19:44:46 PDT 2004
 * ATScsound Ugens, adapted by Alex Norman (2003)
 * from the phase vocoder csound code by Richard Karpen
 * If you find bugs contact me at alexnorman@users.sourceforge.net

Some basic info:

ATSinfo:

idata           ATSinfo     iatsfile, idataloc

read functions:

kfreq, kamp     ATSread     ktimepnt, ifile, ipartial
kenergy         ATSreadnz   ktimepnt, ifile, iband

add functions:

ar              ATSadd      ktimepnt, kfmod, iatsfile, ifn, ipartials   \
                            [, ipartialoffset, ipartialincr, igatefn]
ar              ATSaddnz    ktimepnt, iatsfile, ibands                  \
                            [, ibandoffset, ibandincr]

sinnoi function:

ar              ATSsinnoi   ktimepnt, ksinlev, knzlev, kfreqscale,      \
                            iatsfile, ipartials[, ipartialoffset, ipartialincr]

buf/cross functions:

                ATSbufread  ktimepnt, kfmod, iatsfile, ipartials        \
                            [, ipartialoffset, ipartialincr]
ar              ATScross    ktimepnt, kfmod, iatsfile, ifn, kmyamp,     \
                            kbufamp, ipartials[, ipartialoffset, ipartialincr]
kfreq, kamp     ATSpartialtap   ipartialnum
kamp            ATSinterpread   kfreq

*/

#include "ugnorman.h"

#define ATSA_NOISE_VARIANCE 0.04

#define ATSA_CRITICAL_BAND_EDGES                                            \
    { 0.0, 100.0, 200.0, 300.0, 400.0, 510.0, 630.0, 770.0, 920.0, 1080.0,  \
      1270.0, 1480.0, 1720.0, 2000.0, 2320.0, 2700.0, 3150.0, 3700.0,       \
      4400.0, 5300.0, 6400.0, 7700.0, 9500.0, 12000.0, 15500.0, 20000.0 }

typedef struct {
    ATSBUFREAD  *atsbufreadaddr;    /* must be the first structure member */
    long        seed;
    int         swapped_warning;
} ATS_GLOBALS;

/* static variables used for atsbufread and atsbufreadnz */
static inline ATSBUFREAD **get_atsbufreadaddrp(CSOUND *csound, ATSBUFREAD ***p)
{
    if (*p == NULL)
      *p = (ATSBUFREAD**) csound->QueryGlobalVariableNoCheck(csound,
                                                             "ugNorman_");
    return *p;
}

/* byte swaps a double */

static CS_PURE double bswap(const double *swap_me)
{
    double        d;
    const unsigned char *p1 = (const unsigned char *) swap_me;
    unsigned char *p2 = (unsigned char *) &d;

    p2[0] = p1[7];
    p2[1] = p1[6];
    p2[2] = p1[5];
    p2[3] = p1[4];
    p2[4] = p1[3];
    p2[5] = p1[2];
    p2[6] = p1[1];
    p2[7] = p1[0];

    return d;
}

/* load ATS file into memory; returns "is swapped" boolean, or -1 on error */

static int load_atsfile(CSOUND *csound, void *p, MEMFIL **mfp, char *fname,
                                        void *name_arg)
{
    char        opname[64];
    ATS_GLOBALS *pp;
    ATSSTRUCT   *atsh;
    int         i;

    strcpy(opname, csound->GetOpcodeName(p));   /* opcode name */
    for (i = 0; opname[i] != '\0'; i++)
      opname[i] &= (char) 0xDF;                 /* converted to upper case */

    /* copy in ats file name */
    csound->strarg2name(csound, fname, name_arg, "ats.",
                                (int) csound->GetInputArgSMask(p));

    /* load memfile */
    if ((*mfp = csound->ldmemfile(csound, fname)) == NULL) {
      csound->InitError(csound,
                        Str("%s: Ats file %s not read (does it exist?)"),
                        opname, fname);
      return -1;
    }
    atsh = (ATSSTRUCT*) (*mfp)->beginp;

    /* make sure that this is an ats file */
    if (atsh->magic == 123.0)
      return 0;
    /* check to see if it is byteswapped */
    if ((int) bswap(&(atsh->magic)) != 123) {
      csound->InitError(csound, Str("%s: either %s is not an ATS file "
                                    "or the byte endianness is wrong"),
                                opname, fname);
      return -1;
    }
    pp = (ATS_GLOBALS*) csound->QueryGlobalVariableNoCheck(csound, "ugNorman_");
    if (pp->swapped_warning)
      return 1;
    csound->Warning(csound,
                    Str("%s: %s is byte-swapped\n"
                        "\tno future byte-swapping warnings will be given, "
                        "byte-swapped files\n\twill not result in different "
                        "audio, but they may slow down processing."),
                    opname, fname);
    pp->swapped_warning = 1;
    return 1;
}

/* ats info simply reads data out of the header of an atsfile. (i-rate) */
static int atsinfo(CSOUND *csound, ATSINFO *p)
{
    char      atsfilname[MAXNAME];
    ATSSTRUCT *atsh;
    MEMFIL    *memfile = NULL;
    double    *ret_data;    /* data to return */
    int       swapped = 0;  /* flag to indicate if data needs to be swapped */

    /* load memfile */
    swapped = load_atsfile(csound, p, &memfile, atsfilname, p->ifileno);
    if (swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) memfile->beginp;

    switch ((int) MYFLT2LRND(*p->ilocation)) {
      case 0:   ret_data = &(atsh->sampr);  break;
      case 1:   ret_data = &(atsh->frmsz);  break;
      case 2:   ret_data = &(atsh->winsz);  break;
      case 3:   ret_data = &(atsh->npartials); break;
      case 4:   ret_data = &(atsh->nfrms);  break;
      case 5:   ret_data = &(atsh->ampmax); break;
      case 6:   ret_data = &(atsh->freqmax); break;
      case 7:   ret_data = &(atsh->dur);    break;
      case 8:   ret_data = &(atsh->type);   break;
      default:
        return csound->InitError(csound,
                                 Str("ATSINFO: location is out of bounds: "
                                     "0-8 are the only possible selections"));
    }
    /* if not swapped then just return the data */
    if (!swapped) {
      *p->ireturn = (MYFLT) *ret_data;
      return OK;
    }
    /* otherwise do byteswapping */
    *p->ireturn = (MYFLT) bswap(ret_data);
    return OK;
}

/************************************************************/
/*********  ATSREAD       ***********************************/
/************************************************************/

static void FetchPartial(ATSREAD *p, MYFLT *buf, MYFLT position)
{
    MYFLT   frac;           /* the distance in time we are between frames */
    int     frame;          /* the number of the first frame */
    double  *frm1, *frm2;   /* a pointer to frame 1 and frame 2 */
    double  frm1amp, frm1freq, frm2amp, frm2freq;

    frame = (int) position;
    frm1 = p->datastart + p->frmInc * frame + p->partialloc;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      if (p->swapped == 1) {
        buf[0] = (MYFLT) bswap(frm1);       /* calc amplitude */
        buf[1] = (MYFLT) bswap(frm1 + 1);   /* calc freq */
      }
      else {
        buf[0] = (MYFLT) *frm1;             /* calc amplitude */
        buf[1] = (MYFLT) *(frm1 + 1);       /* calc freq */
      }
      return;
    }
    frm2 = frm1 + p->frmInc;
    frac = position - frame;

    /* byte swap if needed */
    if (p->swapped == 1) {
      frm1amp = bswap(frm1);
      frm2amp = bswap(frm2);
      frm1freq = bswap(frm1 + 1);
      frm2freq = bswap(frm2 + 1);
    }
    else {
      frm1amp = *frm1;
      frm2amp = *frm2;
      frm1freq = *(frm1 + 1);
      frm2freq = *(frm2 + 1);
    }
    buf[0] = (MYFLT) (frm1amp + frac * (frm2amp - frm1amp));    /* calc amp. */
    buf[1] = (MYFLT) (frm1freq + frac * (frm2freq - frm1freq)); /* calc freq */
}

static int atsreadset(CSOUND *csound, ATSREAD *p)
{
    char    atsfilname[MAXNAME];
    ATSSTRUCT *atsh;
    int     n_partials;
    int     type;

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;

    /* byte swap if necessary */
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      n_partials = (int) bswap(&atsh->npartials);
      type = (int) bswap(&atsh->type);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      n_partials = (int) atsh->npartials;
      type = (int) atsh->type;
    }

    /* check to see if partial is valid */
    if ((int) (*p->ipartial) > n_partials || (int) (*p->ipartial) <= 0) {
      return csound->InitError(csound, Str("ATSREAD: partial %i out of range, "
                                           "max allowed is %i"),
                                       (int) (*p->ipartial), n_partials);
    }

    /* point the data pointer to the correct partial */
    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));

    switch (type) {
    case 1:
      p->partialloc = 1 + 2 * (*p->ipartial - 1);
      p->frmInc = n_partials * 2 + 1;
      break;
    case 2:
      p->partialloc = 1 + 3 * (*p->ipartial - 1);
      p->frmInc = n_partials * 3 + 1;
      break;
    case 3:
      p->partialloc = 1 + 2 * (*p->ipartial - 1);
      p->frmInc = n_partials * 2 + 26;
      break;
    case 4:
      p->partialloc = 1 + 3 * (*p->ipartial - 1);
      p->frmInc = n_partials * 3 + 26;
      break;
    default:
      return csound->InitError(csound, Str("Type not implemented"));
    }

    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */
    return OK;
}

static int atsread(CSOUND *csound, ATSREAD *p)
{
    MYFLT   frIndx;
    MYFLT   buf[2];

    if (p->atsmemfile == NULL) {
      return csound->PerfError(csound, Str("ATSREAD: not initialised"));
    }
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSREAD: only positive time pointer "
                                    "values allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSREAD: timepointer out of range, "
                                    "truncated to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    FetchPartial(p, buf, frIndx);
    *p->kamp = buf[0];
    *p->kfreq = buf[1];

    return OK;
}

/*
 * ATSREADNOISE
 */
static MYFLT FetchNzBand(ATSREADNZ *p, MYFLT position)
{
    MYFLT   frac;               /* the distance in time we are between frames */
    int     frame;              /* the time of the first frame */
    double  *frm1, *frm2;
    double  frm1val, frm2val;

    frame = (int) position;
    frm1 = p->datastart + p->frmInc * frame + p->nzbandloc;
    frm1val = (p->swapped == 1) ? bswap(frm1) : *frm1;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr)
      return (MYFLT) frm1val;

    frm2 = frm1 + p->frmInc;
    frac = position - frame;
    frm2val = (p->swapped == 1) ? bswap(frm2) : *frm2;

    return (MYFLT) (frm1val + frac * (frm2val - frm1val));  /* calc energy */
}

static int atsreadnzset(CSOUND *csound, ATSREADNZ *p)
{
    char    atsfilname[MAXNAME];
    ATSSTRUCT *atsh;
    int     n_partials;
    int     type;

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;

    /* byte swap if necessary */
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      n_partials = (int) bswap(&atsh->npartials);
      type = (int) bswap(&atsh->type);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      n_partials = (int) atsh->npartials;
      type = (int) atsh->type;
    }

    /* point the data pointer to the correct partial */
    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));

    /* check to see if band is valid */
    if ((int) (*p->inzbin) > 25 || (int) (*p->inzbin) <= 0) {
      return csound->InitError(csound, Str("ATSREADNZ: band %i out of range, "
                                           "1-25 are the valid band values"),
                                       (int) (*p->inzbin));
    }

    switch (type) {
    case 3:
      /* get past the partial data to the noise */
      p->nzbandloc = (int) (2 * n_partials + *p->inzbin);
      p->frmInc = n_partials * 2 + 26;
      break;

    case 4:
      p->nzbandloc = (int) (3 * n_partials + *p->inzbin);
      p->frmInc = n_partials * 3 + 26;
      break;
    default:
      csound->InitError(csound, Str("ATSREADNZ: Type either not implemented "
                                    "or does not contain noise"));
      return NOTOK;
    }
    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */
    return OK;
}

static int atsreadnz(CSOUND *csound, ATSREADNZ *p)
{
    MYFLT   frIndx;

    if (p->atsmemfile == NULL) {
      return csound->PerfError(csound, Str("ATSREADNZ: not initialised"));
    }
    /* make sure we have not over steped the bounds of the data */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSREADNZ: only positive time pointer "
                                    "values allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSREADNZ: timepointer out of range, "
                                    "truncated to last frame\n"));
      }
    }
    else
      p->prFlg = 1;
    *p->kenergy = FetchNzBand(p, frIndx);
    return OK;
}

/*
 * ATSADD
 */
static  void    FetchADDPartials(ATSADD *, ATS_DATA_LOC *, MYFLT);
static  void    AtsAmpGate(ATS_DATA_LOC *, int, FUNC *, double);

static int atsaddset(CSOUND *csound, ATSADD *p)
{
    char    atsfilname[MAXNAME];
    ATSSTRUCT *atsh;
    FUNC    *ftp, *AmpGateFunc;
    int     memsize, n_partials, type;

    /* set up function table for synthesis */
    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL) {
      csound->ErrorMsg(csound, Str("ATSADD: Function table number "
                                   "for synthesis waveform not valid"));
      return NOTOK;
    }
    p->ftp = ftp;

    /* set up gate function table */
    if (*p->igatefun > FL(0.0)) {
      if ((AmpGateFunc = csound->FTFind(csound, p->igatefun)) == NULL) {
        csound->ErrorMsg(csound, Str("ATSADD: Gate Function table number "
                                     "not valid"));
        return NOTOK;
      }
      else
        p->AmpGateFunc = AmpGateFunc;
    }

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;

    /* calculate how much memory we have to allocate for this */
    memsize =   (int) (*p->iptls) * sizeof(ATS_DATA_LOC)
              + (int) (*p->iptls) * sizeof(double);
    /* allocate space if we need it */
    /* need room for a buffer and an array of oscillator phase increments */
    csound->AuxAlloc(csound, (long) memsize, &p->auxch);

    /* set up the buffer, phase, etc. */
    p->buf = (ATS_DATA_LOC *) (p->auxch.auxp);
    p->oscphase = (double *) (p->buf + (int) (*p->iptls));
    /* byte swap if necessary */
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      n_partials = (int) bswap(&atsh->npartials);
      p->MaxAmp = bswap(&atsh->ampmax);  /* store the maxium amplitude */
      type = (int) bswap(&atsh->type);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      n_partials = (int) atsh->npartials;
      p->MaxAmp = atsh->ampmax;  /* store the maxium amplitude */
      type = (int) atsh->type;
    }

    /* make sure partials are in range */
    if ((int) (*p->iptloffset + *p->iptls * *p->iptlincr) > n_partials ||
        (int) (*p->iptloffset) < 0) {
      return csound->InitError(csound, Str("ATSADD: Partial(s) out of range, "
                                           "max partial allowed is %i"),
                                       n_partials);
    }
    /* get a pointer to the beginning of the data */
    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));

    /* get increments for the partials */
    switch (type) {
    case 1:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 2 + 1;
      break;

    case 2:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 3 + 1;
      break;

    case 3:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 2 + 26;
      break;

    case 4:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 3 + 26;
      break;

    default:
      return csound->InitError(csound, Str("ATSADD: Type not implemented"));
    }

    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */
    return OK;
}

static int atsadd(CSOUND *csound, ATSADD *p)
{
    MYFLT   frIndx;
    MYFLT   *ar, amp, fract, v1, *ftab;
    FUNC    *ftp;
    long    lobits, phase, inc;
    double  *oscphase;
    int     i, nsmps = csound->ksmps;
    int     numpartials = (int) *p->iptls;
    ATS_DATA_LOC *buf;

    buf = p->buf;

    /* ftp is a poiter to the ftable */
    if (p->auxch.auxp == NULL || (ftp = p->ftp) == NULL) {
      return csound->PerfError(csound, Str("ATSADD: not initialised"));
    }

    /* make sure time pointer is within range */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;
        csound->Message(csound, Str("ATSADD: only positive time pointer "
                                    "values are allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSADD: time pointer out of range, "
                                    "truncating to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    FetchADDPartials(p, buf, frIndx);

    oscphase = p->oscphase;
    /* initialise output to zero */
    ar = p->aoutput;
    for (i = 0; i < nsmps; i++)
      ar[i] = FL(0.0);

    if (*p->igatefun > FL(0.0))
      AtsAmpGate(buf, *p->iptls, p->AmpGateFunc, p->MaxAmp);

    for (i = 0; i < numpartials; i++) {
      lobits = ftp->lobits;
      amp = (MYFLT) p->buf[i].amp;
      phase = MYFLT2LONG(*oscphase);
      ar = p->aoutput;         /* ar is a pointer to the audio output */
      nsmps = csound->ksmps;
      /* put in * kfmod */
      inc = MYFLT2LONG(p->buf[i].freq * csound->sicvt * *p->kfmod);
      do {
        ftab = ftp->ftable + (phase >> lobits);
        v1 = *ftab++;
        fract = (MYFLT) PFRAC(phase);
        *ar += (v1 + fract * (*ftab - v1)) * amp;
        ar++;
        phase += inc;
        phase &= PHMASK;
      } while (--nsmps);
      *oscphase = (double) phase;
      oscphase++;
    }
    return OK;
}

static void FetchADDPartials(ATSADD *p, ATS_DATA_LOC *buf, MYFLT position)
{
    MYFLT   frac;               /* the distance in time we are between frames */
    double  *frm0, *frm1;
    double  temp0amp, temp1amp;
    double  temp0freq, temp1freq;
    int     frame;
    int     i;                  /* for the for loop */
    int     partialloc = p->firstpartial;
    int     npartials = (int) *p->iptls;

    frame = (int) position;
    frm0 = p->datastart + frame * p->frmInc;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      for (i = 0; i < npartials; i++) {
        if (p->swapped == 1) {
          buf[i].amp = bswap(&frm0[partialloc]);        /* calc amplitude */
          buf[i].freq = bswap(&frm0[partialloc + 1]);   /* freq */
        }
        else {
          buf[i].amp = frm0[partialloc];                /* calc amplitude */
          buf[i].freq = frm0[partialloc + 1];           /* freq */
        }
        partialloc += p->partialinc;
      }
      return;
    }

    frac = position - frame;
    frm1 = frm0 + p->frmInc;

    for (i = 0; i < npartials; i++) {
      if (p->swapped == 1) {
        temp0amp = bswap(&frm0[partialloc]);
        temp1amp = bswap(&frm1[partialloc]);
        temp0freq = bswap(&frm0[partialloc + 1]);
        temp1freq = bswap(&frm1[partialloc + 1]);
      }
      else {
        temp0amp = frm0[partialloc];
        temp1amp = frm1[partialloc];
        temp0freq = frm0[partialloc + 1];
        temp1freq = frm1[partialloc + 1];
      }
      buf[i].amp = temp0amp + frac * (temp1amp - temp0amp); /* calc amplitude */
      buf[i].freq = temp0freq + frac * (temp1freq - temp0freq); /* calc freq */
      partialloc += p->partialinc;                /* get to the next partial */
    }
}

static void AtsAmpGate(            /* adaption of PvAmpGate by Richard Karpen */
                ATS_DATA_LOC *buf, /* where to get our mag/freq pairs */
                int npartials,     /* number of partials we are working with */
                FUNC *ampfunc, double MaxAmpInData)
{
    int     j;
    long    funclen, mapPoint;

    funclen = ampfunc->flen;

    for (j = 0; j < npartials; ++j) {
      /* use normalised amp as index into table for amp scaling */
      mapPoint = (long) ((buf[j].amp / MaxAmpInData) * funclen);
      buf[j].amp *= (double) *(ampfunc->ftable + mapPoint);
    }
}

/* oscbnk_rand31 from oscbnk.c; written by Istvan Varga */

static CS_PURE long oscbnk_rand31(long seed)
{
    uint64_t tmp1;
    uint32_t tmp2;

    /* x = (16807 * x) % 0x7FFFFFFF */
    tmp1 = (uint64_t) ((int32_t) seed * (int64_t) 16807);
    tmp2 = (uint32_t) tmp1 & (uint32_t) 0x7FFFFFFF;
    tmp2 += (uint32_t) (tmp1 >> 31);
    if ((int32_t) tmp2 < (int32_t) 0)
      tmp2 = (tmp2 + (uint32_t) 1) & (uint32_t) 0x7FFFFFFF;
    return (long) tmp2;
}

/************************************************************/
/*********  ATSADDNZ      ***********************************/
/************************************************************/

/* copied directly from atsh synth-funcs.c
 * with names changed so as not to conflict with csound
 * --------------------------------------------------------
 * randi output random numbers in the range of 1,-1
 * getting a new number at frequency freq and interpolating
 * the intermediate values.
 */

static void randiats_setup(CSOUND *csound,
                           MYFLT freq, long *seed, RANDIATS *radat)
{
    long    tmp;

    tmp = *seed;
    tmp = (tmp <= 0x7FFFFFE7L ? tmp + 23L : tmp - 0x7FFFFFE7L);
    *seed = tmp;

    radat->size = (int) MYFLT2LRND(csound->esr / freq);
    radat->cnt = 0;
    radat->a1 = oscbnk_rand31(tmp);
    radat->a2 = oscbnk_rand31(radat->a1);
}

/* ------------------------------------------------------------------ */

static MYFLT randiats(RANDIATS *radat)
{
    long    second;
    MYFLT   output;

    if (radat->cnt == radat->size) {  /* get a new random value */
      radat->a1 = radat->a2;
      second = oscbnk_rand31(radat->a2);
      radat->a2 = second;
      radat->cnt = 0;
    }

    output = (((MYFLT) (radat->a2 - radat->a1) / (MYFLT) radat->size)
              * (MYFLT) radat->cnt) + (MYFLT) radat->a1;
    radat->cnt++;
    return (FL(1.0) - ((MYFLT) output * (FL(2.0) / (MYFLT) 0x7FFFFFFF)));
}

/* ------------------------------------------------------------------ */

static MYFLT randifats(CSOUND *csound, RANDIATS *radat, MYFLT freq)
{
    long    second;
    MYFLT   output;

    if (radat->cnt == radat->size) {  /* get a new random value */
      radat->a1 = radat->a2;
      second = oscbnk_rand31(radat->a2);
      radat->a2 = second;
      radat->cnt = 0;
      radat->size = (int) MYFLT2LRND(csound->esr / freq);
    }

    output = (((MYFLT) (radat->a2 - radat->a1) / (MYFLT) radat->size)
              * (MYFLT) radat->cnt) + (MYFLT) radat->a1;
    radat->cnt++;

    return (FL(1.0) - ((MYFLT) output * (FL(2.0) / (MYFLT) 0x7FFFFFFF)));
}

static void FetchADDNZbands(ATSADDNZ *p, double *buf, MYFLT position)
{
    double  frac;               /* the distance in time we are between frames */
    double  *frm0, *frm1;
    double  frm0val, frm1val;
    int     frame;
    int     i;                  /* for the for loop */
    int     firstband = p->firstband;

    frame = (int) position;
    frm0 = p->datastart + frame * p->frmInc;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      for (i = 0; i < 25; i++)
        buf[i] = (p->swapped == 1 ? bswap(&frm0[firstband + i])
                                    : frm0[firstband + i]); /* output value */
      return;
    }

    frm1 = frm0 + p->frmInc;
    frac = (double) (position - frame);

    for (i = 0; i < 25; i++) {
      if (p->swapped == 1) {
        frm0val = bswap(&(frm0[firstband + i]));
        frm1val = bswap(&(frm1[firstband + i]));
      }
      else {
        frm0val = frm0[firstband + i];
        frm1val = frm1[firstband + i];
      }
      buf[i] = frm0val + frac * (frm1val - frm0val);  /* calc energy */
    }
}

static int atsaddnzset(CSOUND *csound, ATSADDNZ *p)
{
    char        atsfilname[MAXNAME];
    ATS_GLOBALS *pp;
    ATSSTRUCT   *atsh;
    int         i, type, n_partials;

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;

    /* make sure that this file contains noise */
    type = (p->swapped == 1) ? (int) bswap(&atsh->type) : (int) atsh->type;

    if (type != 4 && type != 3) {
      if (type < 5)
        csound->InitError(csound, Str("ATSADDNZ: "
                                      "This file type contains no noise"));
      else
        csound->InitError(csound, Str("ATSADDNZ: This file type has not been "
                                      "implemented in this code yet."));
      return NOTOK;
    }

    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));
    /* byte swap if necessary */
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      n_partials = (int) bswap(&atsh->npartials);
      p->winsize = (MYFLT) bswap(&atsh->winsz);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      n_partials = (int) atsh->npartials;
      p->winsize = (MYFLT) atsh->winsz;
    }

    /* make sure partials are in range */
    if ((int) (*p->ibandoffset + *p->ibands * *p->ibandincr) > 25 ||
        (int) (*p->ibandoffset) < 0) {
      return csound->InitError(csound, Str("ATSADDNZ: Band(s) out of range, "
                                           "max band allowed is 25"));
    }

    /* point the data pointer to the correct partials */
    switch (type) {
    case 3:
      p->firstband = 1 + 2 * n_partials;
      p->frmInc = n_partials * 2 + 26;
      break;

    case 4:
      p->firstband = 1 + 3 * n_partials;
      p->frmInc = n_partials * 3 + 26;
      break;

    default:
      return csound->InitError(csound, Str("ATSADDNZ: Type either has no noise "
                                           "or is not implemented "
                                           "(only type 3 and 4 work now)"));
    }

    /* save bandwidths for creating noise bands */
    p->nfreq[0] = 100.0;
    p->nfreq[1] = 100.0;
    p->nfreq[2] = 100.0;
    p->nfreq[3] = 100.0;
    p->nfreq[4] = 110.0;
    p->nfreq[5] = 120.0;
    p->nfreq[6] = 140.0;
    p->nfreq[7] = 150.0;
    p->nfreq[8] = 160.0;
    p->nfreq[9] = 190.0;
    p->nfreq[10] = 210.0;
    p->nfreq[11] = 240.0;
    p->nfreq[12] = 280.0;
    p->nfreq[13] = 320.0;
    p->nfreq[14] = 380.0;
    p->nfreq[15] = 450.0;
    p->nfreq[16] = 550.0;
    p->nfreq[17] = 700.0;
    p->nfreq[18] = 900.0;
    p->nfreq[19] = 1100.0;
    p->nfreq[20] = 1300.0;
    p->nfreq[21] = 1800.0;
    p->nfreq[22] = 2500.0;
    p->nfreq[23] = 3500.0;
    p->nfreq[24] = 4500.0;

    /* initialise frequencies to modulate noise by */
    p->phaseinc[0] = TWOPI * 50.0 / csound->esr;
    p->phaseinc[1] = TWOPI * 150.0 / csound->esr;
    p->phaseinc[2] = TWOPI * 250.0 / csound->esr;
    p->phaseinc[3] = TWOPI * 350.0 / csound->esr;
    p->phaseinc[4] = TWOPI * 455.0 / csound->esr;
    p->phaseinc[5] = TWOPI * 570.0 / csound->esr;
    p->phaseinc[6] = TWOPI * 700.0 / csound->esr;
    p->phaseinc[7] = TWOPI * 845.0 / csound->esr;
    p->phaseinc[8] = TWOPI * 1000.0 / csound->esr;
    p->phaseinc[9] = TWOPI * 1175.0 / csound->esr;
    p->phaseinc[10] = TWOPI * 1375.0 / csound->esr;
    p->phaseinc[11] = TWOPI * 1600.0 / csound->esr;
    p->phaseinc[12] = TWOPI * 1860.0 / csound->esr;
    p->phaseinc[13] = TWOPI * 2160.0 / csound->esr;
    p->phaseinc[14] = TWOPI * 2510.0 / csound->esr;
    p->phaseinc[15] = TWOPI * 2925.0 / csound->esr;
    p->phaseinc[16] = TWOPI * 3425.0 / csound->esr;
    p->phaseinc[17] = TWOPI * 4050.0 / csound->esr;
    p->phaseinc[18] = TWOPI * 4850.0 / csound->esr;
    p->phaseinc[19] = TWOPI * 5850.0 / csound->esr;
    p->phaseinc[20] = TWOPI * 7050.0 / csound->esr;
    p->phaseinc[21] = TWOPI * 8600.0 / csound->esr;
    p->phaseinc[22] = TWOPI * 10750.0 / csound->esr;
    p->phaseinc[23] = TWOPI * 13750.0 / csound->esr;
    p->phaseinc[24] = TWOPI * 17750.0 / csound->esr;

    /* initialise phase */
    p->oscphase[0] = 0.0;
    p->oscphase[1] = 0.0;
    p->oscphase[2] = 0.0;
    p->oscphase[3] = 0.0;
    p->oscphase[4] = 0.0;
    p->oscphase[5] = 0.0;
    p->oscphase[6] = 0.0;
    p->oscphase[7] = 0.0;
    p->oscphase[8] = 0.0;
    p->oscphase[9] = 0.0;
    p->oscphase[10] = 0.0;
    p->oscphase[11] = 0.0;
    p->oscphase[12] = 0.0;
    p->oscphase[13] = 0.0;
    p->oscphase[14] = 0.0;
    p->oscphase[15] = 0.0;
    p->oscphase[16] = 0.0;
    p->oscphase[17] = 0.0;
    p->oscphase[18] = 0.0;
    p->oscphase[19] = 0.0;
    p->oscphase[20] = 0.0;
    p->oscphase[21] = 0.0;
    p->oscphase[22] = 0.0;
    p->oscphase[23] = 0.0;
    p->oscphase[24] = 0.0;

    /* initialise band limited noise parameters */
    pp = (ATS_GLOBALS*) csound->QueryGlobalVariableNoCheck(csound, "ugNorman_");
    for (i = 0; i < 25; i++) {
      randiats_setup(csound, p->nfreq[i], &(pp->seed), &(p->randinoise[i]));
    }

    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */

    return OK;
}

static int atsaddnz(CSOUND *csound, ATSADDNZ *p)
{
    MYFLT   frIndx;
    MYFLT   *ar, amp;
    double  *buf;
    int     i, nsmps;
    int     synthme;
    int     nsynthed;

    /* make sure time pointer is within range */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;
        csound->Message(csound, Str("ATSADDNZ: only positive time pointer "
                                    "values are allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSADDNZ: time pointer out of range, "
                                    "truncating to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    buf = p->buf;

    FetchADDNZbands(p, p->buf, frIndx);

    /* set local pointer to output and initialise output to zero */
    ar = p->aoutput;

    for (i = 0; i < csound->ksmps; i++)
      *ar++ = FL(0.0);

    synthme = (int) *p->ibandoffset;
    nsynthed = 0;

    for (i = 0; i < 25; i++) {
      /* do we even have to synthesize it? */
      if (i == synthme && nsynthed < (int) *p->ibands) {
        /* synthesize cosine */
        amp =
            (MYFLT)
            sqrt((p->buf[i] / ((p->winsize) * (MYFLT) ATSA_NOISE_VARIANCE)));
        ar = p->aoutput;
        nsmps = csound->ksmps;
        do {
          *ar += cos(p->oscphase[i]) * amp * randiats(&(p->randinoise[i]));
          p->oscphase[i] += p->phaseinc[i];
          ar++;
        } while (--nsmps);
        /* make sure that the phase does not overflow */
        /*
           while (phase >= costabsz)
             phase = phase - costabsz;
         */
        nsynthed++;
        synthme += (int) *p->ibandincr;
      }
    }
    return OK;
}

static void band_energy_to_res(CSOUND *csound, ATSSINNOI *p)
{
    int     i, j, k;
    MYFLT   edges[] = ATSA_CRITICAL_BAND_EDGES;
    double  *curframe = p->datastart;
    double  bandsum[25];
    double  partialfreq;
    double  partialamp;
    double  **partialband;
    int     *bandnum;

    partialband = (double **) csound->Malloc(csound,
                                             sizeof(double*)
                                             * (int) p->atshead->npartials);
    bandnum = (int *) csound->Malloc(csound, sizeof(int)
                                             * (int) p->atshead->npartials);

    for (i = 0; i < (int) p->atshead->nfrms; i++) {
      /* init sums */
      for (k = 0; k < 25; k++)
        bandsum[k] = 0.0;
      /* find sums per band */
      for (j = 0; j < (int) p->atshead->npartials; j++) {
        partialfreq = *(curframe + 2 + j * (int) p->partialinc);
        partialamp = *(curframe + 1 + j * (int) p->partialinc);
        for (k = 0; k < 25; k++) {
          if ((partialfreq < edges[k + 1]) && (partialfreq >= edges[k])) {
            bandsum[k] += partialamp;
            bandnum[j] = k;
            partialband[j] = (curframe + (int) p->firstband + k);
            break;
          }
        }
      }

      /* compute energy per partial */
      for (j = 0; j < (int) p->atshead->npartials; j++) {
        if (bandsum[bandnum[j]] > 0.0)
          *(p->nzdata + i * (int) p->atshead->npartials + j) =
              (*(curframe + 1 + j * (int) p->partialinc) * *(partialband[j])) /
              bandsum[bandnum[j]];
        else
          *(p->nzdata + i * (int) p->atshead->npartials + j) = 0.0;
      }
      curframe += p->frmInc;
    }

    csound->Free(csound, partialband);
    csound->Free(csound, bandnum);
}

static void fetchSINNOIpartials(ATSSINNOI *, MYFLT);

static int atssinnoiset(CSOUND *csound, ATSSINNOI *p)
{
    char        atsfilname[MAXNAME];
    ATS_GLOBALS *pp;
    ATSSTRUCT   *atsh;
    int         i, memsize, nzmemsize, type;

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;
    p->atshead = atsh;

    /* calculate how much memory we have to allocate for this */
    /* need room for a buffer and the noise data and the noise info */
    /* per partial for synthesizing noise */
    memsize = (int) (*p->iptls) * (sizeof(ATS_DATA_LOC) + 2 * sizeof(double)
                                                        + sizeof(RANDIATS));
    /* allocate space if we need it */
    /* need room for a buffer and an array of oscillator phase increments */
    csound->AuxAlloc(csound, (long) memsize, &p->auxch);

    /* set up the buffer, phase, etc. */
    p->oscbuf = (ATS_DATA_LOC *) (p->auxch.auxp);
    p->randinoise = (RANDIATS *) (p->oscbuf + (int) (*p->iptls));
    p->oscphase = (double *) (p->randinoise + (int) (*p->iptls));
    p->nzbuf = (double *) (p->oscphase + (int) (*p->iptls));

    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      p->npartials = (int) bswap(&atsh->npartials);
      nzmemsize = (int) (p->npartials * bswap(&atsh->nfrms));
      type = (int) bswap(&atsh->type);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      p->npartials = (int) atsh->npartials;
      nzmemsize = (int) (p->npartials * atsh->nfrms);
      type = (int) atsh->type;
    }

    /* see if we have to allocate memory for the nzdata */
    if (nzmemsize != p->nzmemsize) {
      if (p->nzdata != NULL)
        csound->Free(csound, p->nzdata);
      p->nzdata = (double *) csound->Malloc(csound, sizeof(double) * nzmemsize);
    }

    /* make sure partials are in range */
    if ((int) (*p->iptloffset + *p->iptls * *p->iptlincr) > p->npartials ||
        (int) (*p->iptloffset) < 0) {
      return csound->InitError(csound,
                               Str("ATSSINNOI: Partial(s) out of range, "
                                   "max partial allowed is %i"), p->npartials);
    }
    /* get a pointer to the beginning of the data */
    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));
    /* get increments for the partials */

    switch (type) {
    case 1:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = p->npartials * 2 + 1;
      p->firstband = -1;
      break;

    case 2:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = p->npartials * 3 + 1;
      p->firstband = -1;
      break;

    case 3:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = p->npartials * 2 + 26;
      p->firstband = 1 + 2 * p->npartials;
      break;

    case 4:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = p->npartials * 3 + 26;
      p->firstband = 1 + 3 * p->npartials;
      break;

    default:
      return csound->InitError(csound, Str("ATSSINNOI: Type not implemented"));
    }
    /* convert noise per band to noise per partial */
    /* make sure we do not do this if we have done it already. */
    if ((p->firstband != -1) &&
        ((p->filename == NULL) || (strcmp(atsfilname, p->filename) != 0) ||
         (p->nzmemsize != nzmemsize))) {
      if (p->filename != NULL)
        csound->Free(csound, p->filename);
      p->filename = (char *) csound->Malloc(csound,
                                            sizeof(char) * strlen(atsfilname));
      strcpy(p->filename, atsfilname);
   /* csound->Message(csound, "\n band to energy res calculation %s \n",
                              p->filename); */
      /* calculate the band energys */
      band_energy_to_res(csound, p);
    }
    /* save the memory size of the noise */
    p->nzmemsize = nzmemsize;

    /* initialise band limited noise parameters */
    pp = (ATS_GLOBALS*) csound->QueryGlobalVariableNoCheck(csound, "ugNorman_");
    for (i = 0; i < (int) *p->iptls; i++) {
      randiats_setup(csound, FL(10.0), &(pp->seed), &(p->randinoise[i]));
    }
    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */

    return OK;
}

static int atssinnoi(CSOUND *csound, ATSSINNOI *p)
{
    MYFLT   frIndx;
    int     nsmps;
    MYFLT   *ar;
    double  noise;
    double  inc;
    int     i;
    double  phase;
    double  *nzbuf;
    double  amp;
    double  nzamp;              /* noize amp */
    double  sinewave;
    MYFLT   freq;
    MYFLT   nzfreq;
    ATS_DATA_LOC *oscbuf;

    /* make sure time pointer is within range */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;
        csound->Message(csound, Str("ATSSINNOI: only positive time pointer "
                                    "values are allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSSINNOI: time pointer out of range, "
                                    "truncating to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    fetchSINNOIpartials(p, frIndx);

    /* set local pointer to output and initialise output to zero */
    ar = p->aoutput;

    for (i = 0; i < csound->ksmps; i++)
      *ar++ = FL(0.0);

    oscbuf = p->oscbuf;
    nzbuf = p->nzbuf;

    /* do synthesis */
    if (p->firstband != -1) {
      for (i = 0; i < (int) *p->iptls; i++) {
        phase = p->oscphase[i];
        ar = p->aoutput;
        nsmps = csound->ksmps;
        amp = oscbuf[i].amp;
        freq = (MYFLT) oscbuf[i].freq * *p->kfreq;
        inc = TWOPI * freq / csound->esr;
        nzamp =
            sqrt(*(p->nzbuf + i) / (p->atshead->winsz * ATSA_NOISE_VARIANCE));
        nzfreq = (freq < 500.0 ? 50.0 : freq * 0.05);
        do {
          /* calc sine wave */
          sinewave = cos(phase);
          phase += inc;

          /* calc noise */
          noise = nzamp * sinewave
                        * randifats(csound, &(p->randinoise[i]), nzfreq);
          /* calc output */
          *ar +=   (MYFLT) (amp * sinewave) * *p->ksinamp
                 + (MYFLT) noise **p->knzamp;
          ar++;
        } while (--nsmps);
        p->oscphase[i] = phase;
      }
    }
    else {
      for (i = 0; i < (int) *p->iptls; i++) {
        phase = p->oscphase[i];
        ar = p->aoutput;
        nsmps = csound->ksmps;
        amp = oscbuf[i].amp;
        freq = (MYFLT) oscbuf[i].freq * *p->kfreq;
        inc = TWOPI * freq / csound->esr;
        do {
          /* calc sine wave */
          sinewave = cos(phase) * amp;
          phase += inc;
          /* calc output */
          *ar += (MYFLT) sinewave **p->ksinamp;

          ar++;
        } while (--nsmps);
        p->oscphase[i] = phase;
      }
    }
    return OK;
}

static void fetchSINNOIpartials(ATSSINNOI *p, MYFLT position)
{
    double  frac;               /* the distance in time we are between frames */
    double  *frm0, *frm1;
    double  frm0amp, frm0freq, frm1amp, frm1freq;
    double  nz0, nz1;
    ATS_DATA_LOC *oscbuf;
    double  *nzbuf;
    int     frame;
    int     i;                  /* for the for loop */
    int     npartials = p->npartials;

    frame = (int) position;
    frm0 = p->datastart + frame * p->frmInc;

    oscbuf = p->oscbuf;
    nzbuf = p->nzbuf;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      if (p->firstband == -1) { /* there is no noise data */
        if (p->swapped == 1) {
          for (i = (int) *p->iptloffset; i < (int) *p->iptls;
               i += (int) *p->iptlincr) {
            oscbuf->amp = bswap(frm0 + 1 + i * (int) p->partialinc);  /* amp */
            oscbuf->freq = bswap(frm0 + 2 + i * (int) p->partialinc); /* freq */
            oscbuf++;
          }
        }
        else {
          for (i = (int) *p->iptloffset; i < (int) *p->iptls;
               i += (int) *p->iptlincr) {
            oscbuf->amp = *(frm0 + 1 + i * (int) p->partialinc);    /* amp */
            oscbuf->freq = *(frm0 + 2 + i * (int) p->partialinc);   /* freq */
            oscbuf++;
          }
        }
      }
      else {
        if (p->swapped == 1) {
          for (i = (int) *p->iptloffset; i < (int) *p->iptls;
               i += (int) *p->iptlincr) {
            oscbuf->amp = bswap(frm0 + 1 + i * (int) p->partialinc);  /* amp */
            oscbuf->freq = bswap(frm0 + 2 + i * (int) p->partialinc); /* freq */
            *nzbuf = bswap(p->nzdata + frame * npartials + i);
            nzbuf++;
            oscbuf++;
          }
        }
        else {
          for (i = (int) *p->iptloffset; i < (int) *p->iptls;
               i += (int) *p->iptlincr) {
            oscbuf->amp = *(frm0 + 1 + i * (int) p->partialinc);    /* amp */
            oscbuf->freq = *(frm0 + 2 + i * (int) p->partialinc);   /* freq */
            *nzbuf = *(p->nzdata + frame * npartials + i);
            nzbuf++;
            oscbuf++;
          }
        }
      }

      return;
    }
    frm1 = frm0 + p->frmInc;
    frac = (double) (position - frame);

    if (p->firstband == -1) {   /* there is no noise data */
      if (p->swapped == 1) {
        for (i = (int) *p->iptloffset; i < (int) *p->iptls;
             i += (int) *p->iptlincr) {
          frm0amp = bswap(frm0 + 1 + i * (int) p->partialinc);
          frm1amp = bswap(frm1 + 1 + i * (int) p->partialinc);
          frm0freq = bswap(frm0 + 2 + i * (int) p->partialinc);
          frm1freq = bswap(frm1 + 2 + i * (int) p->partialinc);
          oscbuf->amp = frm0amp + frac * (frm1amp - frm0amp);       /* amp */
          oscbuf->freq = frm0freq + frac * (frm1freq - frm0freq);   /* freq */
          oscbuf++;
        }
      }
      else {
        for (i = (int) *p->iptloffset; i < (int) *p->iptls;
             i += (int) *p->iptlincr) {
          frm0amp = *(frm0 + 1 + i * (int) p->partialinc);
          frm1amp = *(frm1 + 1 + i * (int) p->partialinc);
          frm0freq = *(frm0 + 2 + i * (int) p->partialinc);
          frm1freq = *(frm1 + 2 + i * (int) p->partialinc);
          oscbuf->amp = frm0amp + frac * (frm1amp - frm0amp);       /* amp */
          oscbuf->freq = frm0freq + frac * (frm1freq - frm0freq);   /* freq */
          oscbuf++;
        }
      }
    }
    else {
      if (p->swapped == 1) {
        for (i = (int) *p->iptloffset; i < (int) *p->iptls;
             i += (int) *p->iptlincr) {
          frm0amp = bswap(frm0 + 1 + i * (int) p->partialinc);
          frm1amp = bswap(frm1 + 1 + i * (int) p->partialinc);
          frm0freq = bswap(frm0 + 2 + i * (int) p->partialinc);
          frm1freq = bswap(frm1 + 2 + i * (int) p->partialinc);
          nz0 = bswap(p->nzdata + frame * npartials + i);
          nz1 = bswap(p->nzdata + (frame + 1) * npartials + i);
          oscbuf->amp = frm0amp + frac * (frm1amp - frm0amp);       /* amp */
          oscbuf->freq = frm0freq + frac * (frm1freq - frm0freq);   /* freq */
          /* noise */
          *nzbuf = nz0 + frac * (nz1 - nz0);
          nzbuf++;
          oscbuf++;
        }
      }
      else {
        for (i = (int) *p->iptloffset; i < (int) *p->iptls;
             i += (int) *p->iptlincr) {
          frm0amp = *(frm0 + 1 + i * (int) p->partialinc);
          frm1amp = *(frm1 + 1 + i * (int) p->partialinc);
          frm0freq = *(frm0 + 2 + i * (int) p->partialinc);
          frm1freq = *(frm1 + 2 + i * (int) p->partialinc);
          nz0 = *(p->nzdata + frame * npartials + i);
          nz1 = *(p->nzdata + (frame + 1) * npartials + i);
          oscbuf->amp = frm0amp + frac * (frm1amp - frm0amp);       /* amp */
          oscbuf->freq = frm0freq + frac * (frm1freq - frm0freq);   /* freq */
          /* noise */
          *nzbuf = nz0 + frac * (nz1 - nz0);
          nzbuf++;
          oscbuf++;
        }
      }
    }
}

/************************************************************/
/*********** ATSBUFREAD *************************************/
/************************************************************/

static int atsbufreadset(CSOUND *csound, ATSBUFREAD *p)
{
    char    atsfilname[MAXNAME];
    MEMFIL  *mfp;
    ATS_DATA_LOC *fltp;
    ATSSTRUCT *atsh;
    int     type, n_partials;
    int     memsize;            /* the size of the memory to request for AUX */

    /* load memfile */
    p->swapped = load_atsfile(csound, p, &mfp, atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) mfp->beginp;

    /* get past the header to the data, point frptr at time 0 */
    p->datastart = (double *) atsh + 10;
    p->prFlg = 1;               /* true */

    /* is swapped? */
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      type = (int) bswap(&atsh->type);
      n_partials = (int) bswap(&atsh->npartials);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      type = (int) atsh->type;
      n_partials = (int) atsh->npartials;
    }

    /* we need room for 2 * (1 table + 2 for 20 and 20,000 hz) */
    /* (one sorted one unsorted) */
    memsize = 2 * (int) (*p->iptls + 2);

    csound->AuxAlloc(csound, memsize * (long) sizeof(ATS_DATA_LOC), &p->auxch);

    fltp = (ATS_DATA_LOC *) p->auxch.auxp;
    p->table = fltp;
    p->utable = fltp + (int) (*p->iptls + 2);

    /* check to see if partial is valid */
    if ((int) (*p->iptloffset + *p->iptls * *p->iptlincr) > n_partials ||
        (int) (*p->iptloffset) < 0) {
      return csound->InitError(csound, Str("ATSBUFREAD: Partial out of range, "
                                           "max partial is %i"), n_partials);
    }

    /* set up partial locations and frame increments */

    switch (type) {
    case 1:
      p->firstpartial = 1 + 2 * (*p->iptloffset);
      p->partialinc = 2;
      p->frmInc = n_partials * 2 + 1;
      break;

    case 2:
      p->firstpartial = 1 + 3 * (*p->iptloffset);
      p->partialinc = 3;
      p->frmInc = n_partials * 3 + 1;
      break;

    case 3:
      p->firstpartial = 1 + 2 * (*p->iptloffset);
      p->partialinc = 2;
      p->frmInc = n_partials * 2 + 26;
      break;

    case 4:
      p->firstpartial = 1 + 3 * (*p->iptloffset);
      p->partialinc = 3;
      p->frmInc = n_partials * 3 + 26;
      break;

    default:
      return csound->InitError(csound, Str("ATSBUFREAD: Type not implemented"));
    }

    /* put 20 hertz = 0amp and 20000 hz = 0amp */
    /* to make interpolation easier later */
    p->table[0].freq = p->utable[0].freq = 20;
    p->table[0].amp = p->utable[0].amp = 0;
    p->table[(int) *p->iptls + 1].freq = p->utable[(int) *p->iptls + 1].freq =
        20000;
    p->table[(int) *p->iptls + 1].amp = p->utable[(int) *p->iptls + 1].amp = 0;

    *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp))) = p;

    return OK;
}

static int mycomp(const void *p1, const void *p2)
{
    const ATS_DATA_LOC *a1 = p1;
    const ATS_DATA_LOC *a2 = p2;

    if ((*a1).freq < (*a2).freq)
      return -1;
    else if ((*a1).freq == (*a2).freq)
      return 0;
    else
      return 1;
}

static void FetchBUFPartials(ATSBUFREAD *p,
                             ATS_DATA_LOC *buf, ATS_DATA_LOC *buf2,
                             MYFLT position)
{
    MYFLT   frac;               /* the distance in time we are between frames */
    double  *frm0, *frm1;
    double  frm0amp, frm0freq, frm1amp, frm1freq;
    int     frame;
    int     i;                  /* for the for loop */
    int     partialloc = p->firstpartial;
    int     npartials = (int) *p->iptls;

    frame = (int) position;
    frm0 = p->datastart + frame * p->frmInc;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      if (p->swapped == 1) {
        for (i = 0; i < npartials; i++) {                   /* calc amplitude */
          buf[i].amp = buf2[i].amp = bswap(&frm0[partialloc]);
          buf[i].freq = buf2[i].freq = bswap(&frm0[partialloc + 1]);
          partialloc += p->partialinc;
        }
      }
      else {
        for (i = 0; i < npartials; i++) {
          buf[i].amp = buf2[i].amp = frm0[partialloc];      /* calc amplitude */
          buf[i].freq = buf2[i].freq = frm0[partialloc + 1];
          partialloc += p->partialinc;
        }
      }
      return;
    }

    frac = position - frame;
    frm1 = frm0 + p->frmInc;
    if (p->swapped == 1) {
      for (i = 0; i < npartials; i++) {
        frm0amp = bswap(&frm0[partialloc]);
        frm0freq = bswap(&frm0[partialloc + 1]);
        frm1amp = bswap(&frm1[partialloc]);
        frm1freq = bswap(&frm1[partialloc + 1]);
        /* calc amplitude */
        buf[i].amp = buf2[i].amp = frm0amp + frac * (frm1amp - frm0amp);
        /* calc freq */
        buf[i].freq = buf2[i].freq =
            *p->kfmod * (frm0freq + frac * (frm1freq - frm0freq));
        partialloc += p->partialinc;  /* get to the next partial */
      }
    }
    else {
      for (i = 0; i < npartials; i++) {
        /* calc amplitude */
        buf[i].amp = buf2[i].amp =
            frm0[partialloc] + frac * (frm1[partialloc] - frm0[partialloc]);
        /* calc freq */
        buf[i].freq = buf2[i].freq =
            *p->kfmod * (frm0[partialloc + 1]
                         + frac * (frm1[partialloc + 1]
                                   - frm0[partialloc + 1]));
        partialloc += p->partialinc;  /* get to the next partial */
      }
    }
}

static int atsbufread(CSOUND *csound, ATSBUFREAD *p)
{
    MYFLT         frIndx;
    ATS_DATA_LOC  *buf;
    ATS_DATA_LOC  *buf2;

    *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp))) = p;

    if (p->auxch.auxp == NULL) {  /* RWD fix */
      return csound->PerfError(csound, Str("ATSBUFREAD: not initialised"));
    }

    /* make sure time pointer is within range */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;
        csound->Message(csound, Str("ATSBUFREAD: only positive time pointer "
                                    "values are allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSBUFREAD: time pointer out of range, "
                                    "truncating to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    /* skip the first value in the table because */
    /* we will never have to change it as it is 20hz with amp 0 */
    buf = p->table + 1;
    buf2 = p->utable + 1;
    FetchBUFPartials(p, buf, buf2, frIndx);
    /* must sort the buffered values */
    qsort(buf, (int) *p->iptls, sizeof(ATS_DATA_LOC), mycomp);

    return OK;
}

/* ATS partial tap */

static int atspartialtapset(CSOUND *csound, ATSPARTIALTAP *p)
{
    ATSBUFREAD  *atsbufreadaddr;

    atsbufreadaddr = *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp)));
    if (atsbufreadaddr == NULL) {
      return csound->InitError(csound,
                               Str("ATSPARTIALTAP: you must have an "
                                   "atsbufread before an atspartialtap"));
    }
    if ((int) *p->iparnum > (int) *(atsbufreadaddr->iptls)) {
      return csound->InitError(csound, Str("ATSPARTIALTAP: exceeded "
                                           "max partial %i"),
                                       (int) *(atsbufreadaddr->iptls));
    }
    if ((int) *p->iparnum <= 0) {
      return csound->InitError(csound, Str("ATSPARTIALTAP: partial must be "
                                           "positive and nonzero"));
    }
    return OK;
}

static int atspartialtap(CSOUND *csound, ATSPARTIALTAP *p)
{
    ATSBUFREAD  *atsbufreadaddr;

    atsbufreadaddr = *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp)));
    if (atsbufreadaddr == NULL) {
      return csound->PerfError(csound,
                               Str("ATSPARTIALTAP: you must have an "
                                   "atsbufread before an atspartialtap"));
    }
    *p->kfreq = (MYFLT) ((atsbufreadaddr->utable)[(int) (*p->iparnum)].freq);
    *p->kamp = (MYFLT) ((atsbufreadaddr->utable)[(int) (*p->iparnum)].amp);
    return OK;
}

/* ATS interpread */

static int atsinterpreadset(CSOUND *csound, ATSINTERPREAD *p)
{
    if (*(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp))) == NULL)
      return csound->InitError(csound,
                               Str("ATSINTERPREAD: you must have an "
                                   "atsbufread before an atsinterpread"));
    p->overflowflag = 1;       /* true */
    return OK;
}

static int atsinterpread(CSOUND *csound, ATSINTERPREAD *p)
{
    ATSBUFREAD  *atsbufreadaddr;
    int         i;
    MYFLT       frac;

    /* make sure we have data to read from */
    atsbufreadaddr = *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp)));
    if (atsbufreadaddr == NULL) {
      return csound->PerfError(csound,
                               Str("ATSINTERPREAD: you must have an "
                                   "atsbufread before an atsinterpread"));
    }
    /* make sure we are not asking for unreasonble frequencies */
    if (*p->kfreq <= FL(20.0) || *p->kfreq >= FL(20000.0)) {
      if (p->overflowflag) {
        csound->Warning(csound, Str("ATSINTERPREAD: frequency must be greater "
                                    "than 20 and less than 20000 Hz"));
        p->overflowflag = 0;
      }
      *p->kamp = FL(0.0);
      return OK;
    }
    /* find the location in the table */
    for (i = 0; i < (int) *(atsbufreadaddr->iptls); i++) {
      /* find i such that table i+1 is greater than the specified frequency */
      if ((MYFLT) ((atsbufreadaddr->table[i + 1]).freq) > *p->kfreq)
        break;
    }
    if (i == 0) {
      *p->kamp = FL(0.0);
      return OK;
    }
    /* linear interpolation */
    frac =
        (*p->kfreq -
         (atsbufreadaddr->table[i]).freq) /
        ((atsbufreadaddr->table[i + 1]).freq - (atsbufreadaddr->table[i]).freq);
    *p->kamp =
        (MYFLT) ((atsbufreadaddr->table[i]).amp +
                 frac * ((atsbufreadaddr->table[i + 1]).amp -
                         (atsbufreadaddr->table[i]).amp));
    /* *p->kamp = (MYFLT) (atsbufreadaddr->table[i]).amp; */
    return OK;
}

/* ATS cross */

static int atscrossset(CSOUND *csound, ATSCROSS *p)
{
    char    atsfilname[MAXNAME];
    ATSSTRUCT *atsh;
    FUNC    *ftp;
    int     memsize;
    int     type, n_partials;

    /* set up function table for synthesis */
    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL) {
      csound->ErrorMsg(csound, Str("ATSCROSS: Function table number for "
                                   "synthesis waveform not valid"));
      return NOTOK;
    }
    p->ftp = ftp;

    /* load memfile */
    p->swapped = load_atsfile(csound,
                              p, &(p->atsmemfile), atsfilname, p->ifileno);
    if (p->swapped < 0)
      return NOTOK;
    atsh = (ATSSTRUCT*) p->atsmemfile->beginp;

    /* calculate how much memory we have to allocate for this */
    memsize =   (int) (*p->iptls) * sizeof(ATS_DATA_LOC)
              + (int) (*p->iptls) * sizeof(double);
    /* allocate space if we need it */
    /* need room for a buffer and an array of oscillator phase increments */
    csound->AuxAlloc(csound, (long) memsize, &p->auxch);

    /* set up the buffer, phase, etc. */
    p->buf = (ATS_DATA_LOC *) (p->auxch.auxp);
    p->oscphase = (double *) (p->buf + (int) (*p->iptls));
    if (p->swapped == 1) {
      p->maxFr = (int) bswap(&atsh->nfrms) - 1;
      p->timefrmInc = bswap(&atsh->nfrms) / bswap(&atsh->dur);
      type = (int) bswap(&atsh->type);
      n_partials = (int) bswap(&atsh->npartials);
    }
    else {
      p->maxFr = (int) atsh->nfrms - 1;
      p->timefrmInc = atsh->nfrms / atsh->dur;
      type = (int) atsh->type;
      n_partials = (int) atsh->npartials;
    }

    /* make sure partials are in range */
    if ((int) (*p->iptloffset + *p->iptls * *p->iptlincr) > n_partials ||
        (int) (*p->iptloffset) < 0) {
      return csound->InitError(csound, Str("ATSCROSS: Partial(s) out of range, "
                                           "max partial allowed is %i"),
                                       n_partials);
    }
    /* get a pointer to the beginning of the data */
    p->datastart = (double *) (p->atsmemfile->beginp + sizeof(ATSSTRUCT));

    /* get increments for the partials */
    switch (type) {
    case 1:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 2 + 1;
      break;

    case 2:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 3 + 1;
      break;

    case 3:
      p->firstpartial = (int) (1 + 2 * (*p->iptloffset));
      p->partialinc = 2 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 2 + 26;
      break;

    case 4:
      p->firstpartial = (int) (1 + 3 * (*p->iptloffset));
      p->partialinc = 3 * (int) (*p->iptlincr);
      p->frmInc = n_partials * 3 + 26;
      break;

    default:
      return csound->InitError(csound, Str("ATSCROSS: Type not implemented"));
    }

    /* flag set to reduce the amount of warnings sent out */
    /* for time pointer out of range */
    p->prFlg = 1;               /* true */

    return OK;
}

static void FetchCROSSPartials(ATSCROSS *p, ATS_DATA_LOC *buf, MYFLT position)
{
    MYFLT   frac;               /* the distance in time we are between frames */
    double  *frm0, *frm1;
    double  frm0amp, frm0freq, frm1amp, frm1freq;
    int     frame;
    int     i;                  /* for the for loop */
    int     partialloc = p->firstpartial;
    int     npartials = (int) *p->iptls;

    frame = (int) position;
    frm0 = p->datastart + frame * p->frmInc;

    /* if we are using the data from the last frame */
    /* we should not try to interpolate */
    if (frame == p->maxFr) {
      if (p->swapped == 1) {
        for (i = 0; i < npartials; i++) {
          buf[i].amp = bswap(&frm0[partialloc]);  /* calc amplitude */
          buf[i].freq = bswap(&frm0[partialloc + 1]);
          partialloc += p->partialinc;
        }
      }
      else {
        for (i = 0; i < npartials; i++) {
          buf[i].amp = frm0[partialloc];          /* calc amplitude */
          buf[i].freq = frm0[partialloc + 1];
          partialloc += p->partialinc;
        }
      }
      return;
    }

    frac = position - frame;
    frm1 = frm0 + p->frmInc;
    if (p->swapped == 1) {
      for (i = 0; i < npartials; i++) {
        frm0amp = frm0[partialloc];
        frm0freq = frm0[partialloc + 1];
        frm1amp = frm1[partialloc];
        frm1freq = frm1[partialloc + 1];

        buf[i].amp = frm0amp + frac * (frm1amp - frm0amp);  /* calc amplitude */
        buf[i].freq = frm0freq + frac * (frm1freq - frm0freq);  /* calc freq */
        partialloc += p->partialinc;              /* get to the next partial */
      }
    }
    else {
      for (i = 0; i < npartials; i++) {
        /* calc amplitude */
        buf[i].amp = frm0[partialloc]
                     + frac * (frm1[partialloc] - frm0[partialloc]);
        /* calc freq */
        buf[i].freq = frm0[partialloc + 1]
                      + frac * (frm1[partialloc + 1] - frm0[partialloc + 1]);
        partialloc += p->partialinc;  /* get to the next partial */
      }
    }
}

static void ScalePartials(
                ATS_DATA_LOC *cbuf, /* the current buffer */
                int cbufnp,     /* the current buffer's number of partials */
                MYFLT cbufamp,  /* the amplitude for the current buffer */
                ATS_DATA_LOC *tbuf, /* the table buffer */
                int tbufnp,     /* the table buffer's n partials */
                MYFLT tbufamp)  /* the amp of the table buffer */
{
    MYFLT   tempamp;            /* hold the cbufn amp for a bit */
    MYFLT   frac;               /* for interpilation */
    int     i, j;               /* for the for loop */

    for (i = 0; i < cbufnp; i++) {
      /* look for closest freqency in buffer */
      for (j = 0; j < tbufnp; j++) {
        if (tbuf[j].freq > cbuf[i].freq)
          break;
      }
      tempamp = FL(0.0);
      /* make sure we are not going to overstep our array */
      if (j < tbufnp && j > 0) {
        /* interp amplitude from table */
        frac =
            (cbuf[i + 1].freq - tbuf[j - 1].freq) / (tbuf[j].freq -
                                                     tbuf[j - 1].freq);
        tempamp = tbuf[j - 1].amp + frac * (tbuf[j].amp - tbuf[j - 1].amp);
      }
      else if (j == tbufnp) {
        /* this means the last value in the table */
        /* is equal to a value in the current buffer */
        if (cbuf[i + 1].freq == tbuf[tbufnp - 1].freq)
          tempamp = tbuf[tbufnp - 1].amp;
      }
      /* do the actual scaling */
      cbuf[i].amp = cbufamp * cbuf[i].amp + tempamp * tbufamp;
    }
}

static int atscross(CSOUND *csound, ATSCROSS *p)
{
    ATSBUFREAD  *atsbufreadaddr;
    MYFLT   frIndx;
    MYFLT   *ar, amp, fract, v1, *ftab;
    FUNC    *ftp;
    long    lobits, phase, inc;
    double  *oscphase;
    int     i, nsmps = csound->ksmps;
    int     numpartials = (int) *p->iptls;
    ATS_DATA_LOC *buf;

    atsbufreadaddr = *(get_atsbufreadaddrp(csound, &(p->atsbufreadaddrp)));
    if (atsbufreadaddr == NULL) {
      return csound->PerfError(csound,
                               Str("ATSCROSS: you must have an "
                                   "atsbufread before an atsinterpread"));
    }

    buf = p->buf;

    /* ftp is a poiter to the ftable */
    if (p->auxch.auxp == NULL || (ftp = p->ftp) == NULL)
      return csound->PerfError(csound, Str("ATSCROSS: not initialised"));

    /* make sure time pointer is within range */
    if ((frIndx = *(p->ktimpnt) * p->timefrmInc) < FL(0.0)) {
      frIndx = FL(0.0);
      if (p->prFlg) {
        p->prFlg = 0;
        csound->Message(csound, Str("ATSCROSS: only positive time pointer "
                                    "values are allowed, setting to zero\n"));
      }
    }
    else if (frIndx > p->maxFr) {
      /* if we are trying to get frames past where we have data */
      frIndx = (MYFLT) p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;           /* set to false */
        csound->Message(csound, Str("ATSCROSS: time pointer out of range, "
                                    "truncating to last frame\n"));
      }
    }
    else
      p->prFlg = 1;

    FetchCROSSPartials(p, buf, frIndx);

    ScalePartials(
        buf,                    /* the current buffer */
        (int) *(p->iptls),      /* the current buffer's number of partials */
        *(p->kmyamp),           /* the amplitude for the current buffer */
        atsbufreadaddr->table,  /* the table buffer */
        (int) *(atsbufreadaddr->iptls), /* the table buffer's n partials */
        *p->katsbufamp);        /* the amp of the table buffer */

    oscphase = p->oscphase;
    /* initialise output to zero */
    ar = p->aoutput;
    for (i = 0; i < nsmps; i++)
      ar[i] = FL(0.0);

    for (i = 0; i < numpartials; i++) {
      lobits = ftp->lobits;
      amp = (MYFLT) p->buf[i].amp;
      phase = MYFLT2LONG(*oscphase);
      ar = p->aoutput;         /* ar is a pointer to the audio output */
      nsmps = csound->ksmps;
      /* put in * kfmod */
      inc = MYFLT2LONG(p->buf[i].freq * csound->sicvt * *p->kfmod);
      do {
        ftab = ftp->ftable + (phase >> lobits);
        v1 = *ftab++;
        fract = (MYFLT) PFRAC(phase);
        *ar += (v1 + fract * (*ftab - v1)) * amp;
        ar++;
        phase += inc;
        phase &= PHMASK;
      } while (--nsmps);
      *oscphase = (double) phase;
      oscphase++;
    }
    return OK;
}

/* end of ugnorman.c */

#define S(x)    sizeof(x)

static OENTRY localops[] = {
    { "ATSread",        S(ATSREAD),         3,  "kk",   "kTi",
        (SUBR) atsreadset,          (SUBR) atsread,         (SUBR) NULL      },
    { "ATSreadnz",      S(ATSREADNZ),       3,  "k",    "kTi",
        (SUBR) atsreadnzset,        (SUBR) atsreadnz,       (SUBR) NULL      },
    { "ATSadd",         S(ATSADD),          5,  "a",    "kkTiiopo",
        (SUBR) atsaddset,           (SUBR) NULL,            (SUBR) atsadd    },
    { "ATSaddnz",       S(ATSADDNZ),        5,  "a",    "kTiop",
        (SUBR) atsaddnzset,         (SUBR) NULL,            (SUBR) atsaddnz  },
    { "ATSsinnoi",      S(ATSSINNOI),       5,  "a",    "kkkkTiop",
        (SUBR) atssinnoiset,        (SUBR) NULL,            (SUBR) atssinnoi },
    { "ATSbufread",     S(ATSBUFREAD),      3,  "",     "kkTiop",
        (SUBR) atsbufreadset,       (SUBR) atsbufread,      (SUBR) NULL      },
    { "ATSpartialtap",  S(ATSPARTIALTAP),   3,  "kk",   "i",
        (SUBR) atspartialtapset,    (SUBR) atspartialtap,   (SUBR) NULL      },
    { "ATSinterpread",  S(ATSINTERPREAD),   3,  "k",    "k",
        (SUBR) atsinterpreadset,    (SUBR) atsinterpread,   (SUBR) NULL      },
    { "ATScross",       S(ATSCROSS),        5,  "a",    "kkTikkiopo",
        (SUBR) atscrossset,         (SUBR) NULL,            (SUBR) atscross  },
    { "ATSinfo",        S(ATSINFO),         1,  "i",    "Ti",
        (SUBR) atsinfo,             (SUBR) NULL,            (SUBR) NULL      }
};

PUBLIC long opcode_size(void)
{
    return (long) sizeof(localops);
}

PUBLIC OENTRY *opcode_init(CSOUND *csound)
{
    ATS_GLOBALS *p;

    if (csound->CreateGlobalVariable(csound, "ugNorman_", sizeof(ATS_GLOBALS))
        != 0)
      csound->Die(csound, Str("ugnorman.c: error allocating globals"));
    p = (ATS_GLOBALS*) csound->QueryGlobalVariableNoCheck(csound, "ugNorman_");
    p->atsbufreadaddr = NULL;
    p->seed = (long) (csound->GetRandomSeedFromTime() & (uint32_t) 0x7FFFFFFD);
    p->seed++;
    p->swapped_warning = 0;

    return localops;
}

