/*

    emugens.c

*/

// #include <math.h>
#include <csdl.h>

/*

  linlin 

  linear to linear conversion

  ky = linlin(kx, kxlow, kxhigh, kylow, kyhigh)
  
  ky = (kx - kxlow) / (kxhigh - kxlow) * (kyhigh - kylow) + kylow

  linlin(0.25, 0, 1, 1, 3) ; --> 1.5

 */

typedef struct {
  OPDS    h;
  MYFLT   *kout, *kx, *kx0, *kx1, *ky0, *ky1;
} LINLINK;

static int linlink(CSOUND *csound, LINLINK *p) {
  MYFLT x0 = *p->kx0;
  MYFLT y0 = *p->ky0;
  MYFLT x = *p->kx;
  *p->kout = (x - x0) / (*(p->kx1) -x0) * (*(p->ky1) - y0) + y0;
  return OK;
}


/* ------------- xyscale --------------

2d linear interpolation (normalized)

Given values for four points at (0, 0), (0, 1), (1, 0), (1, 1), 
calculate the interpolated value at a given coord (x, y) inside this square

inputs: kx, ky, v00, v10, v01, v11

kx, ky: coord, between 0-1

This is conceptually the same as:

ky0 = scale(kx, v01, v00)
ky1 = scale(kx, v11, v10)
kout = scale(ky, ky1, ky0)

*/

typedef struct {
  OPDS    h;
  MYFLT   *kout, *kx, *ky, *v00, *v10, *v01, *v11;
  MYFLT   d0, d1;
} XYSCALE;

static int xyscalei_init(CSOUND *csound, XYSCALE *p) {
  p->d0 = (*p->v01) - (*p->v00);
  p->d1 = (*p->v11) - (*p->v10);
}

static int xyscalei(CSOUND *csound, XYSCALE *p) {
  // x, y: between 0-1
  MYFLT x = *p->kx;
  MYFLT y0 = x*(p->d0)+(*p->v00);
  MYFLT y1 = x*(p->d1)+(*p->v10);
  *p->kout = (*p->ky)*(y1-y0)+y0;
  return OK;
}

static int xyscale(CSOUND *csound, XYSCALE *p) {
  // x, y: between 0-1
  // x, y will interpolate between the values at the 4 corners
  MYFLT v00 = *p->v00;
  MYFLT v10 = *p->v10;
  MYFLT x = *p->kx;
  MYFLT y0 = x*(*p->v01 - v00)+v00;
  MYFLT y1 = x*(*p->v11 - v10)+v10;
  *p->kout = (*p->ky)*(y1-y0)+y0;
  return OK;
}

/*  mtof -- ftom 

midi to frequency conversion

kfreq = mtof(69, 442)  ; A4 is optional, default=442
kfreq = mtof(69)

*/

typedef struct {
  OPDS    h;
  MYFLT *r, *k;
  MYFLT freqA4;
} PITCHCONV;

static int mtof(CSOUND *csound, PITCHCONV *p) {
  *p->r = pow(FL(2.0), (*p->k - FL(69.0)) / FL(12.0)) * p->freqA4;
  return OK;
}

static int mtof_init(CSOUND *csound, PITCHCONV *p) {
  p->freqA4 = csound->GetA4(csound);
  mtof(csound, p);
  return OK;
}

static int ftom(CSOUND *csound, PITCHCONV *p) {
  *p->r = FL(12.0) * log2(*p->k / p->freqA4) + FL(69.0);
  return OK;
}

static int ftom_init(CSOUND *csound, PITCHCONV *p) {
  p->freqA4 = csound->GetA4(csound);
  ftom(csound, p);
  return OK;
}

static int pchtom(CSOUND *csound, PITCHCONV *p) {
  MYFLT pch = *p->k;
  MYFLT oct = floor(pch);
  MYFLT note = pch - oct;
  *p->r = (oct-FL(3.0))*FL(12.0)+note*FL(100.0);
  return OK;
}


/*
  bpf  --> break point function with linear interpolation

  Useful for smaller cases where:

  * defining a table is overkill
  * higher accuracy in the x coord 
  * values are changing at k-rate

  ky  bpf  kx, kx0, ky0, kx1, ky1, ...

*/


#define INTERP_L(X, X0, X1, Y0, Y1) ((X) < (X0) ? (Y0) : (((X)-(X0))/((X1)-(X0)) * ((Y1)-(Y0)) + (Y0)))

inline MYFLT interpol_l(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1) {
  return x < x0 ? y0 : ((x-x0)/(x1-x0) * (y1-y0) + y0);
}

#define INTERP_R(X, X0, X1, Y0, Y1) ((X) > (X1) ? (Y1) : (((X)-(X0))/((X1)-(X0)) * ((Y1)-(Y0)) + (Y0)))
									 
inline MYFLT interpol_r(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1) {
  return x > x1 ? y0 : ((x-x0)/(x1-x0) * (y1-y0) + y0);
}

#define INTERP_M(X, X0, X1, Y0, Y1) (((X)-(X0))/((X1)-(X0)) * ((Y1)-(Y0)) + (Y0))

inline MYFLT interpol_m(MYFLT x, MYFLT x0, MYFLT x1, MYFLT y0, MYFLT y1) {
  return (x-x0)/(x1-x0) * (y1-y0) + y0;
}

  
typedef struct {
  OPDS    h;
  MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2;
} BPF3;

  
static int bpf3(CSOUND *csound, BPF3 *p) {
  MYFLT x = *p->x;
  MYFLT n, m;
  if(x<*p->x1) {
	m = *p->x0; n = *p->y0;
	*p->r = INTERP_L(x, m, *p->x1, n, *p->y1);
  } else {
	m = *p->x1;
	n = *p->y1; 
	*p->r = INTERP_R(x, m, *p->x2, n, *p->y2);
  }
  return OK;
}

typedef struct {
  OPDS    h;
  MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3;
} BPF4;

  
static int bpf4(CSOUND *csound, BPF4 *p) {
  MYFLT x = *p->x;
  MYFLT m, n;
  if(x < (*p->x1)) {
	m = *p->x0; n = *p->y0;
	*p->r = INTERP_L(x, m, *p->x1, n, *p->y1);
  } else if (x < (*p->x2)) {
	m = *p->x1; n = *p->y1;
	*p->r = INTERP_M(x, m, *p->x2, n, *p->y2);
  }  else {
	m = *p->x2; n = *p->y2;
	*p->r = INTERP_R(x, m, *p->x3, n, *p->y3);
  }
  return OK;
}

typedef struct {
  OPDS    h;
  MYFLT *r, *x, *x0, *y0, *x1, *y1, *x2, *y2, *x3, *y3, *x4, *y4;
} BPF5;

static int bpf5(CSOUND *csound, BPF5 *p) {
  MYFLT x = *p->x;
  if(x < (*p->x2)) { 
	if(x < (*p->x1)) {
	  *p->r = INTERP_L(x, *p->x0, *p->x1, *p->y0, *p->y1);
	} else {
	  *p->r = INTERP_M(x, *p->x1, *p->x2, *p->y1, *p->y2);
	}
  }  else if (x < (*p->x3)) {
	*p->r = INTERP_M(x, *p->x2, *p->x3, *p->y2, *p->y3);
  }	else {
	*p->r = INTERP_R(x, *p->x3, *p->x4, *p->y3, *p->y4);
  }
  return OK;
}


/*  ntom  - mton

	midi to notename conversion

 */

typedef struct {
  OPDS h;
  MYFLT *r;
  STRINGDAT *notename;
} NTOM;

int _pcs[] = {9, 11, 0, 2, 4, 5, 7};

static int ntom(CSOUND *csound, NTOM *p) {
  /*
	formats accepted: 8D+ (equals to +50 cents), 4C#, 8A-31 7Bb+30
	- no lowercase
	- octave is necessary and comes always first
	- no negative octaves, no octaves higher than 9
  */
  char *n = (char *) p->notename->data;
  int octave = n[0] - '0';
  int pcidx = n[1] - 'A';
  if(pcidx < 0 || pcidx >= 7) {
	printf("expecting a chr between A and G, but got %c\n", n[1]);
	return NOTOK;
  }
  int pc = _pcs[pcidx];
  int cents = 0;
  int cursor;
  if(n[2] == '#') {
	pc += 1;
	cursor = 3;
  } else if (n[2] == 'b') {
	pc -= 1;
	cursor = 3;
  } else {
	cursor = 2;
  }
  int rest = p->notename->size - 1 - cursor;
  if(rest > 0) {
	int sign = n[cursor] == '+' ? 1 : -1;
	if(rest == 1) {
	  cents = 50;
	} else if(rest == 2) {
	  cents = n[cursor+1] - '0';
	} else if (rest == 3) {
	  cents = 10*(n[cursor+1] - '0') + (n[cursor+2] - '0');
	} else {
	  printf("format not understood\n");
	  return NOTOK;
	}
	cents *= sign;
  }
  *p->r = ((octave + 1)*12 + pc) + cents/FL(100.0);
  return OK;
}

typedef struct {
  OPDS h;
  STRINGDAT *Sdst;
  MYFLT *kmidi;
} MTON;

//               C  C# D D#  E  F  F# G G# A Bb B
int _pc2idx[] = {2, 2, 3, 3, 4, 5, 5, 6, 6, 0, 1, 1};
int _pc2alt[] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 2, 0};
char _alts[] = " #b";

static int mton(CSOUND *csound, MTON *p) {
  char *dst;
  MYFLT m = *p->kmidi;
  int maxsize = 7;  // 4C#+99\0
  if (p->Sdst->data == NULL) {
      p->Sdst->data = csound->Calloc(csound, maxsize);
      p->Sdst->size = maxsize;
  }
  dst = (char*) p->Sdst->data;
  int octave = m / 12 - 1;
  int pc = (int)m % 12;
  int cents = round((m - floor(m))*100.0);
  int sign, cursor;
  
  if(cents == 0) {
	sign = 0;
  } else if (cents <= 50) {
	sign = 1;
  } else {
	cents = 100 - cents;
	sign = -1;
	pc += 1;
	if(pc == 12) {
	  pc = 0;
	  octave += 1;
	}
  }
  if(octave >= 0) {
	dst[0] = '0' + octave;
	cursor = 1;
  } else {
	dst[0] = '-';
	dst[1] = '0' - octave;
	cursor = 2;
  }
  dst[cursor] = 'A' + _pc2idx[pc];
  cursor += 1;
  int alt = _pc2alt[pc];
  if(alt > 0) {
	dst[cursor] = _alts[alt];
	cursor++;
  }
  if(sign == 1) {
	dst[cursor] = '+';
	cursor++;
	if(cents < 10) {
	  dst[cursor] = '0'+cents;
	  cursor++;
	} else if(cents != 50) {
	  dst[cursor] = '0' + (int)(cents / 10);
	  dst[cursor+1] = '0' + (cents % 10);
	  cursor += 2;
	} 
  } else if(sign == -1) {
	dst[cursor] = '-';
	cursor++;
	if(cents < 10) {
	  dst[cursor] = '0'+cents;
	  cursor++;
	} else if(cents != 50) {
	  dst[cursor] = '0' + (int)(cents / 10);
	  dst[cursor+1] = '0' + (cents % 10);
	  cursor += 2;
	} 
  }
  for(int i=cursor; i<maxsize; i++) {
	dst[i] = '\0';
  }
}

	
#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "linlin",  S(LINLINK),   0, 2,      "k", "kkkkk",   NULL, (SUBR)linlink },
  { "xyscale", S(XYSCALE),   0, 2,      "k", "kkkkkk",  NULL, (SUBR)xyscale },
  { "xyscale", S(XYSCALE),   0, 3,      "k", "kkiiii",  (SUBR)xyscalei_init, (SUBR)xyscalei },
  { "mtof",    S(PITCHCONV), 0, 3,      "k", "k",  (SUBR)mtof_init, (SUBR)mtof},
  { "mtof",    S(PITCHCONV), 0, 1,      "i", "i",  (SUBR)mtof_init},
  { "ftom",    S(PITCHCONV), 0, 3,      "k", "k",  (SUBR)ftom_init, (SUBR)ftom},
  { "ftom",    S(PITCHCONV), 0, 1,      "i", "i",  (SUBR)ftom_init},
  { "pchtom",  S(PITCHCONV), 0, 1,      "i", "i",   (SUBR)pchtom},
  { "pchtom",  S(PITCHCONV), 0, 2,      "k", "k",   NULL, (SUBR)pchtom},
  { "bpf",     S(BPF3),      0, 3,      "k", "kkkkkkk",     (SUBR)bpf3, (SUBR)bpf3 },
  { "bpf",     S(BPF4),      0, 3,      "k", "kkkkkkkkk",   (SUBR)bpf4, (SUBR)bpf4 },
  { "bpf",     S(BPF5),      0, 3,      "k", "kkkkkkkkkkk", (SUBR)bpf5, (SUBR)bpf5 },
  { "ntom",    S(NTOM),      0, 3,      "k", "S", (SUBR)ntom, (SUBR)ntom },
  { "ntom",    S(NTOM),      0, 1,      "i", "S", (SUBR)ntom },
  { "mton",    S(MTON),      0, 3,      "S", "k", (SUBR)mton, (SUBR)mton},
  { "mton",    S(MTON),      0, 1,      "S", "i", (SUBR)mton}
};


LINKAGE
