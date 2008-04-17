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

#include "csdl.h"
#include <math.h>
#include <limits.h>

typedef struct {
        OPDS h;
        MYFLT *ar, *amp, *kfund, *kform, *kdamp, *knofpulse, *kpulsemul,
              *iftab, *iskip;
        FUNC *ftable;
        int32  timrem;    /* samples left of event */
        int32  pulstogo;  /* count of pulses to produce in burst */
        int32  pulsephs;  /* index into table of this pulse (= MAXLEN / kform) */
        int32  pulseinc;  /* increment in table of pulse */
        MYFLT pulseamp;  /* amp of current pulse */
        MYFLT ampdecay;  /* subtract from amp on new pulse */
        MYFLT lenfact;   /* increase length of next pulse */
} VOSIM;


/* Post: unless skipping init, timrem == 0 */
int vosimset(CSOUND* csound, VOSIM *p)
{
    if (*p->iskip)
      return;

    p->ftable = csound->FTFind(csound, p->iftab);
    if (p->ftable == NULL) {
      csound->InitError(csound, Str("vosim: pulse table not found"));
      return;
    }

    p->timrem = p->pulstogo = p->pulsephs = p->pulseinc = 0;
    p->pulseamp = p->ampdecay = p->lenfact = FL(0.0);
    return OK;
}


/* Pre: timrem == 0.
 * Post:
 *    IF kform is positive (or zero), pulsephs >= FMAXLEN.
 *    IF kform is negative, pulsephs < 0.
 *    timrem > 0.
 */
void vosim_event(CSOUND* csound, VOSIM *p)
{
    p->pulstogo = *p->knofpulse + 1;  /* count of pulses, (+1 since decr at start of pulse) */
    if (*p->kfund == FL(0.0))         /* infinitely long event */
      p->timrem = INT_MAX;
    else
      p->timrem = (int32)(csound->esr / fabs(*p->kfund));
    p->pulseinc = (int32)(*p->kform * csound->sicvt);
    p->pulsephs = (p->pulseinc >= 0)? MAXLEN : -1;   /* starts a new pulse */
    p->ampdecay = *p->kdamp;
    p->pulseamp = *p->amp + p->ampdecay;  /* increase initial amp, since it's reduced at pulse start */
    p->lenfact  = *p->kpulsemul;      /* if negative, table is read alternately back-/forward */
    if (p->lenfact != FL(0.0))        /* reduce table rate, since it's increased at pulse start */
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
    pulselen = (p->pulseinc != FL(0.0))? abs(FMAXLEN / p->pulseinc) : INT_MAX;
    if (--p->pulstogo <= 0 || pulselen > p->timrem) {
      p->pulstogo = 0;
      return;
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
    if (ftp == NULL) {
      csound->PerfError(csound, Str("vosim: not initialised"));
      return NOTOK;
    }
    ftdata = ftp->ftable;
    lobits = ftp->lobits;

    do {
      /* new event? */
      if (p->timrem-- == 0)
        vosim_event(csound, p);

      /* new pulse? */
      if (p->pulsephs >= MAXLEN || p->pulsephs < 0)
        vosim_pulse(csound, p);

      /* silence after last pulse in burst */
      if (p->pulstogo == 0) {
        /* bypass regular synthesis and fill output with zeros */
        while (p->timrem && nsmps) {
          *ar++ = FL(0.0);
          --p->timrem;
          --nsmps;
        }
        /* Done with perf-block? */
        if (nsmps == 0) {
          /* Here we could override (extreme) timrem if kfund has increased...? */
          /* But then we'd also have to record event start time(?) */
          break;
        }
        /* else goto top of while loop to make a new event */
        ++nsmps;
        continue;
      }

      p->pulsephs &= PHMASK;
      *ar++ = *(ftdata + (p->pulsephs >> lobits)) * p->pulseamp;
      p->pulsephs += p->pulseinc;
    } while (--nsmps);
    return OK;
}

/* ar   vosim   kamp, kFund, kForm, kDamp, kPulseCount, kPulseFactor,
                ifn [, iskip] */

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "vosim", S(VOSIM), 5, "a", "kkkkkkio", (SUBR)vosimset, NULL, (SUBR)vosim }
};


LINKAGE
