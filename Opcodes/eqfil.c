#include "csdl.h"

typedef struct _equ {
  OPDS h;
  MYFLT *out;
  MYFLT *sig, *fr, *bw, *g, *ini;  /* in, freq, bw, gain, ini */
  double z1,z2;              /* delay memory */
  MYFLT frv, bwv;            /* bandwidth and frequency */
  double c,d;                /* filter vars */  

} equ;

static int equ_init(CSOUND *csound, equ *p)
{
    if(*p->ini==0){
	MYFLT sr = csound->GetSr(csound);
    p->z1 = p->z2 = 0.0;
    p->frv = *p->fr; p->bwv = *p->bw;
    p->d = cos(2*PI*p->frv/sr);
    p->c = tan(PI*p->bwv/sr);
	}

    return OK;
}

static int equ_process(CSOUND *csound, equ *p)
{
	double z1 = p->z1, z2 = p->z2,c,d,w,a,y;
	MYFLT  *in= p->sig,*out=p->out,g;
	int i, ksmps = csound->GetKsmps(csound);

	if(*p->bw != p->bwv || *p->fr != p->frv){
		MYFLT sr = csound->GetSr(csound);
		p->frv = *p->fr; p->bwv = *p->bw;
        p->d = cos(2*PI*p->frv/sr);
        p->c = tan(PI*p->bwv/sr);
	}
	c = p->c;
	d = p->d;
    a = (1-c)/(1+c);
	g = *p->g;

	for(i=0; i < ksmps; i++){
       w = in[i] + d*(1.0 + a)*z1 - a*z2;
	   y = w*a - d*(1.0 + a)*z1 + z2;
       z2 = z1;
       z1 = w;
       out[i] = (MYFLT) (0.5*(y + in[i] + g*(in[i] - y)));

	}
    p->z1 = z1;
	p->z2 = z2;

    return OK;
}

static OENTRY localops[] = {
  {"eqfil", sizeof(equ), 5,
   "a", "akkko", (SUBR)equ_init, NULL, (SUBR)equ_process}
};

LINKAGE
