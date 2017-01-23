/*

    scugens.c

*/

#include <math.h>
#include <csdl.h>

#define LOG001 FL(-6.907755278982137)
#define CALCSLOPE(next,prev,nsmps) ((next - prev) * (FL(1)/nsmps))
#define SR (csound->GetSr(csound))

#define LOOP1(length, stmt)    \
  {	int xxn = (length);		   \
	do {					   \
	  stmt;					   \
	} while (--xxn);		   \
  }

#define ZXP(z) (*(z)++)

static inline MYFLT zapgremlins(MYFLT x)
{
	MYFLT absx = abs(x);
	// very small numbers fail the first test, eliminating denormalized numbers
	//    (zero also fails the first test, but that is OK since it returns zero.)
	// very large numbers fail the second test, eliminating infinities
	// Not-a-Numbers fail both tests and are eliminated.
	return (absx > (MYFLT)1e-15 && absx < (MYFLT)1e15) ? x : (MYFLT)0.;
}


static inline MYFLT sc_wrap(MYFLT in, MYFLT lo, MYFLT hi) {
 	MYFLT range;
	// avoid the divide if possible
	if (in >= hi) {
		range = hi - lo;
		in -= range;
		if (in < hi) return in;
	} else if (in < lo) {
		range = hi - lo;
		in += range;
		if (in >= lo) return in;
	} else return in;

	if (hi == lo) return lo;
	// return in - range * floor((in - lo)/range);
	return in - range * FLOOR((in - lo) / range);
}
 

/*

  lag

  This is essentially the same as OnePole except that instead of 
  supplying the coefficient directly, it is calculated from a 60 dB lag time. 
  This is the time required for the filter to converge to within 0.01% 
  of a value. This is useful for smoothing out control signals.

  ksmooth = lag(kx, klagtime, [initialvalue=0])
  asmooth = lag(ka, klagtime, [initialvalue=0])
  
*/

typedef struct {
  OPDS    h;
  MYFLT   *out, *in, *lagtime, *first;
  MYFLT   lag, b1, y1;
  MYFLT   sr;
} LAG;

static int lagk_next(CSOUND *csound, LAG *p) {
  MYFLT lag = *p->lagtime;
  MYFLT y0 = *p->in;
  MYFLT y1 = p->y1;
  MYFLT b1;
  if (lag == p->lag) {
	b1 = p->b1;
	p->y1 = y1 = y0 + b1 * (y1 - y0);
	*p->out = y1;
	return OK;
  } else {
	// faust uses tau2pole = exp(-1 / (lag*sr))
	b1 = lag == FL(0) ? FL(0) : exp(LOG001 / (lag * p->sr));
	*p->out = y0 + b1 * (y1 - y0);
	p->lag = lag;
	p->b1 = b1;
	return OK;
  }
}

static int lag_init0(CSOUND *csound, LAG *p) {
  p->lag = -1;
  p->b1 = FL(0);
  p->y1 = *p->first;
  return OK;
}

static int lagk_init(CSOUND *csound, LAG *p) {
  lag_init0(csound, p);
  p->sr = csound->GetKr(csound);
  return lagk_next(csound, p);
}

static int laga_init(CSOUND *csound, LAG *p) {
  lag_init0(csound, p);
  p->sr = csound->GetSr(csound);
  return OK;
}


static int laga_next(CSOUND *csound, LAG *p) {
  uint32_t n, nsmps = CS_KSMPS;
  MYFLT *in = p->in, *out = p->out;
  MYFLT lag = *p->lagtime;
  MYFLT y1 = p->y1;
  MYFLT b1 = p->b1;
  MYFLT y0;
  if (lag == p->lag) {
	LOOP1(nsmps,
		  y0 = *in; in++;
		  y1 = y0 + b1 * (y1 - y0);
		  *out = y1; out++;
		  );
	p->y1 = y1;
	return OK;
  } else {
	// faust uses tau2pole = exp(-1 / (lag*sr))
	p->b1 = lag == FL(0) ? FL(0) : exp(LOG001 / (lag * p->sr));
	MYFLT b1_slope = CALCSLOPE(p->b1, b1, nsmps);
	p->lag = lag;
	LOOP1(nsmps,
		  b1 += b1_slope;
		  y0 = *in; in++;
		  y1 = y0 + b1 * (y1 - y0);
		  *out = y1; out++;
		  );
	p->y1 = y1;
	return OK;
  }
}
 
// ------------------------- LagUD ---------------------------

typedef struct {
  OPDS    h;
  MYFLT   *out, *in, *lagtimeU, *lagtimeD, *first;
  MYFLT   lagu, lagd, b1u, b1d, y1;
} LagUD;

static int lagud_a(CSOUND *csound, LagUD *p) {
  MYFLT
	*out = p->out,
	*in = p->in,
	lagu = *p->lagtimeU,
	lagd = *p->lagtimeD,
	y1 = p->y1,
	b1u = p->b1u,
	b1d = p->b1d;
  
  uint32_t nsmps = CS_KSMPS;

  if ((lagu == p->lagu) && (lagd == p->lagd)) {
	LOOP1(nsmps,
		  MYFLT y0 = *in; in++;
		  if (y0 > y1)
			y1 = y0 + b1u * (y1 - y0); 
		  else
			y1 = y0 + b1d * (y1 - y0);
		  *out = y1; out++;
		  );
  } else {
	MYFLT sr = csound->GetSr(csound);
	// faust uses tau2pole = exp(-1 / (lag*sr))
	p->b1u = lagu == FL(0) ? FL(0) : exp(LOG001 / (lagu * sr));
	MYFLT b1u_slope = CALCSLOPE(p->b1u, b1u, nsmps);
	p->lagu = lagu;
	p->b1d = lagd == FL(0) ? FL(0) : exp(LOG001 / (lagd * sr));
	MYFLT b1d_slope = CALCSLOPE(p->b1d, b1d, nsmps);
	p->lagd = lagd;
	LOOP1(nsmps,
		  b1u += b1u_slope;
		  b1d += b1d_slope;
		  MYFLT y0 = *in; in++;
		  if (y0 > y1)
			y1 = y0 + b1u * (y1-y0);
		  else
			y1 = y0 + b1d * (y1-y0);
		  *out = y1; out++;
		  );
  }
  p->y1 = zapgremlins(y1);
  return OK;
}

static int lagud_k(CSOUND *csound, LagUD *p) {
  MYFLT
	*in = p->in,
	lagu = *p->lagtimeU,
	lagd = *p->lagtimeD,
	y1 = p->y1;
  
  uint32_t nsmps = CS_KSMPS;

  if ((lagu == p->lagu) && (lagd == p->lagd)) {
	MYFLT y0 = *in;
	if (y0 > y1)
	  p->y1 = y1 = y0 + p->b1u * (y1 - y0); 
	else
	  p->y1 = y1 = y0 + p->b1d * (y1 - y0);
	*(p->out) = y1;
	
  } else {
	MYFLT sr = csound->GetKr(csound);
	// faust uses tau2pole = exp(-1 / (lag*sr)), sc uses log(0.01)
	p->b1u = lagu == FL(0) ? FL(0) : exp(LOG001 / (lagu * sr));
	p->lagu = lagu;
	p->b1d = lagd == FL(0) ? FL(0) : exp(LOG001 / (lagd * sr));
	p->lagd = lagd;
	MYFLT y0 = *in;
	if (y0 > y1)
	  y1 = y0 + p->b1u * (y1 - y0);
	else
	  y1 = y0 + p->b1d * (y1 - y0);
	*(p->out) = y1;
  }
  p->y1 = y1;
  return OK;
}

static int lagud_init(CSOUND *csound, LagUD *p) {
  p->lagu = -1;
  p->lagd = -1;
  p->b1u = FL(0);
  p->b1d = FL(0);
  p->y1 = *p->first;
  return OK;
}

// ------------------ Trig -------------------------
// trig(signal, duration)
// Returns 1 for given duration whenever signal crosses from
// non-positive to positiv


typedef struct {
  OPDS    h;
  MYFLT   *out, *in, *dur;
  MYFLT   level, prevtrig;
  long counter;
} Trig;

static int trig_a(CSOUND *csound, Trig *p) {
  MYFLT
	*out = p->out,
	*in = p->in;
  MYFLT
	dur = *p->dur,
	sr = csound->GetSr(csound),
	prevtrig = p->prevtrig,
	level = p->level;
  unsigned long counter = p->counter;
  uint32_t nsmps = CS_KSMPS;
  for(uint32_t n=0; n<nsmps; n++) {
	MYFLT curtrig = *in; in++;
	MYFLT zout;
	if (counter > 0) {
	  zout = --counter ? level : FL(0);
	} else {
	  if (curtrig > FL(0) && prevtrig <= FL(0)) {
		counter = (long)(dur * sr + FL(0.5));
		if (counter < 1) counter = 1;
		level = curtrig;
		zout = level;
	  } else {
		zout = FL(0);
	  }
	}
	prevtrig = curtrig;
	*out = zout; out++;
  }
  p->prevtrig = prevtrig;
  p->counter = counter;
  p->level = level;
  return OK;
}
  
static int trig_k(CSOUND *csound, Trig *p) {
  MYFLT curtrig = *p->in;
  MYFLT dur = *p->dur;
  MYFLT kr = csound->GetKr(csound);
  MYFLT prevtrig = p->prevtrig;
  MYFLT level = p->level;
  unsigned long counter = p->counter;
  if (counter > 0) {
	*p->out = --counter ? level : FL(0);
  } else {
	if (curtrig > FL(0) && prevtrig <= FL(0)) {
	  counter = (long)(dur * kr + FL(0.5));
	  if (counter < 1)
		counter = 1;
	  level = curtrig;
	  *p->out = level;
	} else {
	  *p->out = FL(0);
	}
  }
  p->prevtrig = curtrig;
  p->counter = counter;
  p->level = level;
  return OK;
}

static int trig_init(CSOUND *csound, Trig *p) {
  p->counter = 0;
  p->prevtrig = FL(0);
  p->level = FL(0);
  trig_k(csound, p);
}


/*

Phasor

Phasor is a linear ramp between start and end values. When its trigger 
input crosses from non-positive to positive, Phasor's output will jump 
to its reset position. Upon reaching the end of its ramp Phasor will wrap 
back to its start.

NOTE: N.B. Since end is defined as the wrap point, its value is never 
      actually output.

NOTE: If one wants Phasor to output a signal with frequency freq 
      oscillating between start and end, then the rate should be 
      (end - start) * freq / sr where sr is the sampling rate.

Phasor is commonly used as an index control.

aindex phasor atrig, xrate, kstart, kend, kresetPos
kindex phasor ktrig, krate, kstart, kend, kresetPos

trig: When triggered, jump to resetPos (default: 0, equivalent to start).
rate: The amount of change per sample, i.e at a rate of 1 the value 
      of each sample will be 1 greater than the preceding sample.
start: Start point of the ramp.
end:   End point of the ramp.
resetPos: The value to jump to upon receiving a trigger.
*/


typedef struct {
  OPDS    h;
  MYFLT   *out, *trig, *rate, *start, *end, *resetPos;
  MYFLT   level, previn, resetk;
} Phasor;

static int phasor_init(CSOUND *csound, Phasor *p) {
  p->previn = 0;
  p->level = 0;
  p->resetk = 1;
  
}

static int phasor_init0(CSOUND *csound, Phasor *p) {
  p->previn = 0;
  p->level = 0;
  p->resetk = 0;
}

static int phasor_aa(CSOUND *csound, Phasor *p) {
  MYFLT *out  = p->out;
  MYFLT *in = p->trig;
  MYFLT *rate = p->rate;
  MYFLT start = *p->start;
  MYFLT end   = *p->end;
  MYFLT resetPos = p->resetk ? (*p->resetPos) : 0;
  MYFLT previn = p->previn;
  MYFLT level = p->level;
  for (uint32_t n=0; n<CS_KSMPS; n++) {
	MYFLT curin = *in; in++;
	MYFLT zrate = *rate; rate++;
	if (previn <= FL(0) && curin > FL(0)) {
	  MYFLT frac = FL(1) - previn/(curin-previn);
	  level = resetPos + frac * zrate;
	}
	*out = level; out++;
	level += zrate;
	level = sc_wrap(level, start, end);
	previn = curin;
  }
  p->previn = previn;
  p->level = level;
  return OK;
}

static int phasor_ak(CSOUND *csound, Phasor *p) {
  MYFLT *out  = p->out;
  MYFLT *in   = p->trig;
  MYFLT rate  = *p->rate;
  MYFLT start = *p->start;
  MYFLT end   = *p->end;
  // MYFLT resetPos = *p->resetPos;
  MYFLT resetPos = p->resetk ? (*p->resetPos) : 0;
  MYFLT previn = p->previn;
  MYFLT level = p->level;
  for (uint32_t n=0; n<CS_KSMPS; n++) {
	MYFLT curin = *in; in++;
	if (previn <= FL(0) && curin > FL(0)) {
	  MYFLT frac = FL(1) - previn/(curin-previn);
	  level = resetPos + frac * rate;
	}
	*out = level; out++;
	level += rate;
	level = sc_wrap(level, start, end);
	previn = curin;
  }
  p->previn = previn;
  p->level = level;
  return OK;
}

static int phasor_kk(CSOUND *csound, Phasor *p) {
  MYFLT curin = *p->trig;
  MYFLT rate  = *p->rate;
  MYFLT start = *p->start;
  MYFLT end   = *p->end;
  // MYFLT resetPos = *p->resetPos;
  MYFLT resetPos = p->resetk ? (*p->resetPos) : 0;
  MYFLT previn = p->previn;
  MYFLT level = p->level;

  if (previn <= FL(0) && curin > FL(0)) {
	level = resetPos;
  }
  level = sc_wrap(level, start, end);
  *p->out = level;
  level += rate;
  p->previn = curin;
  p->level = level;
  return OK;
}


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "sc_lag",     S(LAG),   0, 3,   "k", "kko", (SUBR)lagk_init, (SUBR)lagk_next },
  { "sc_lag",     S(LAG),   0, 5,   "a", "ako", (SUBR)laga_init, NULL, (SUBR)laga_next },
  { "sc_lagud",   S(LagUD), 0, 3,   "k", "kkko", (SUBR)lagud_init, (SUBR)lagud_k },
  { "sc_lagud",   S(LagUD), 0, 5,   "a", "akko", (SUBR)lagud_init, NULL, (SUBR)lagud_a },
  { "sc_trig",    S(Trig),  0, 3,   "k", "kk", (SUBR)trig_init, (SUBR)trig_k },
  { "sc_trig",    S(Trig),  0, 5,   "a", "ak", (SUBR)trig_init, NULL, (SUBR)trig_a },
  { "sc_phasor",  S(Phasor),  0, 3,   "k", "kkkkk", (SUBR)phasor_init, (SUBR)phasor_kk },
  { "sc_phasor",  S(Phasor),  0, 5,   "a", "akkkk", (SUBR)phasor_init, NULL, (SUBR)phasor_ak },
  { "sc_phasor",  S(Phasor),  0, 5,   "a", "aakkk", (SUBR)phasor_init, NULL, (SUBR)phasor_aa },
  { "sc_phasor",  S(Phasor),  0, 3,   "k", "kkkk", (SUBR)phasor_init0, (SUBR)phasor_kk },
  { "sc_phasor",  S(Phasor),  0, 5,   "a", "akkk", (SUBR)phasor_init0, NULL, (SUBR)phasor_ak },
  { "sc_phasor",  S(Phasor),  0, 5,   "a", "aakk", (SUBR)phasor_init0, NULL, (SUBR)phasor_aa }
};

LINKAGE
