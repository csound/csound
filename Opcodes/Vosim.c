/* VOSIM.C: VOice SIMulation implementation
 * by rasmus ekman March 13, 2008, for Csound.
 * This code is released under the Csound license,
 * GNU Lesser General Public License version 2.1,
 * or (at your option) any later version.
 */

/* Programmer note:
 * Pre- and post-conditions are noted at function level.
 * These are essential for the code to work at all.
 * There are some complications because we accept weird user input:
 * (a) kfund < 0: This is forced to positive - no point in "backward" events.
 * (b) kform == 0: This leads to infinite length pulse, ie silence.
 * (c) kform < 0: Table is read backward.
 *     If table is symmetric, kform and -kform should give bit-identical outputs.
 *
 * Don't fiddle with the code unless you understand how the table read
 * params pulsephs and pulseinc are used both to read table and to indicate that
 * a new pulse should start, and that pulseinc can be negative.
 */

//#include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"
#include <math.h>
#include <limits.h>

typedef struct {
        OPDS h;
        MYFLT *ar, *amp, *kfund, *kform, *kdamp, *knofpulse, *kpulsemul,
              *iftab, *iskip;
        FUNC *ftable;
        int32 timrem;    /* samples left of event */
        int32 pulstogo;  /* count of pulses to produce in burst */
        int32 pulsephs;  /* index into table of this pulse (= MAXLEN / kform) */
        int32 pulseinc;  /* increment in table of pulse */
        MYFLT pulseamp;  /* amp of current pulse */
        MYFLT ampdecay;  /* subtract from amp on new pulse */
        MYFLT lenfact;   /* increase length of next pulse */
} VOSIM;


/* Post: unless skipping init, timrem == 0 */
int vosimset(CSOUND* csound, VOSIM *p)
{
    if (*p->iskip)
      return OK;

    p->ftable = csound->FTFind(csound, p->iftab);
    if (UNLIKELY(p->ftable == NULL)) {
      return csound->InitError(csound, Str("vosim: pulse table not found"));
    }

     p->timrem = p->pulstogo = p->pulsephs = p->pulseinc = 0;
     p->pulseamp = p->ampdecay = p->lenfact = FL(0.0);
     return OK;
}


/* Pre: timrem == 0.
  * Post:
  *    IF kform >= 0, pulsephs >= FMAXLEN.
  *    ELSE pulsephs < 0.
  *    timrem > 0.
  */
void vosim_event(CSOUND* csound, VOSIM *p)
{
    MYFLT fundabs = FABS(*p->kfund);
    /* count of pulses, (+1 since decr at start of pulse) */
    p->pulstogo = 1+(int32)*p->knofpulse;
    if (UNLIKELY(fundabs == FL(0.0))) {                /* infinitely long event */
      p->timrem = INT_MAX;
      csound->Warning(csound,
                      Str("vosim: zero kfund. 'Infinite' length event generated."));
    }
    else {
        p->timrem = (int32)(csound->esr / fundabs);
        if (UNLIKELY(p->timrem == 0)) {
          p->timrem = csound->ksmps;
          p->pulstogo = 0;
          csound->Warning(csound,
                          Str("vosim: kfund (%f) > sr. Generating ksmps silence."),
                          *p->kfund);
        }
    }
    p->pulseinc = (int32)(*p->kform * csound->sicvt);
    p->pulsephs = (p->pulseinc >= 0)? MAXLEN : -1;   /* starts a new pulse */
    p->ampdecay = *p->kdamp;
    /* increase initial amp, since it's reduced at pulse start */
    p->pulseamp = *p->amp + p->ampdecay;
    /* if negative, table is read alternately back-/forward */
    p->lenfact  = *p->kpulsemul;
    /* reduce table rate, since it's increased at pulse start */
    if (p->lenfact != FL(0.0))
      p->pulseinc /= p->lenfact;
}


/* Pre: pulsephs >= FMAXLEN OR pulsephs < 0.
 * Post:
 *    pulstogo is decremented or zero.
 *    0 <= pulsephs < FMAXLEN.
 */
void vosim_pulse(CSOUND* csound, VOSIM *p)
{
    int32 pulselen;
    p->pulsephs &= PHMASK;
    p->pulseinc *= p->lenfact;
    /* If pulse can't fit in remaining event time, skip and generate silence */
    pulselen = (p->pulseinc != FL(0.0))?
                (int32)FABS(FMAXLEN / p->pulseinc) : INT_MAX;
    if (p->pulstogo-- <= 0 || pulselen > p->timrem) {
      p->pulstogo = 0;
    }
    p->pulseamp -= p->ampdecay;
}


int vosim(CSOUND* csound, VOSIM *p)
{
    int32 nsmps = csound->ksmps;
    MYFLT *ar = p->ar;
    MYFLT *ftdata;
    int32  lobits;

    FUNC *ftp = p->ftable;
    if (UNLIKELY(ftp == NULL)) goto err1;
    ftdata = ftp->ftable;
    lobits = ftp->lobits;

    while (nsmps > 0) {
      /* new event? */
      if (p->timrem == 0)
        vosim_event(csound, p);

      /* new pulse? */
      if (p->pulsephs >= MAXLEN || p->pulsephs < 0)
        vosim_pulse(csound, p);

      if (p->pulstogo > 0) {
        /* produce one sample */
        p->pulsephs &= PHMASK;
        *ar++ = *(ftdata + (p->pulsephs >> lobits)) * p->pulseamp;
        --p->timrem;
        --nsmps;
        p->pulsephs += p->pulseinc;
      }
      else {
        /* silence after last pulse in burst: */
        /* bypass regular synthesis and fill output with zeros */
        while (p->timrem && nsmps) {
          *ar++ = FL(0.0);
          --p->timrem;
          --nsmps;
        }
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, Str("vosim: not initialised"));
}


/* ar   vosim   kamp, kFund, kForm, kDamp, kPulseCount, kPulseFactor,
                ifn [, iskip] */

#define S(x)    sizeof(x)

static OENTRY vosim_localops[] = {
  { "vosim", S(VOSIM), TR|5, "a", "kkkkkkio", (SUBR)vosimset, NULL, (SUBR)vosim }
};


LINKAGE1(vosim_localops)
