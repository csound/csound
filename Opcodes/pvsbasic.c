/* pvsbasic.c:
   basic opcodes for transformation of streaming PV signals

   (c) Victor Lazzarini, 2004
*/


/* pvsmix */

#include "csdl.h"
#include "pvsbasic.h"
#include <math.h>

int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2);

int pvsmixset(PVSMIX *p){

    long N = p->fa->N;

    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fa->overlap;
    p->fout->winsize = p->fa->winsize;
    p->fout->wintype = p->fa->wintype;
    p->fout->format = p->fa->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format==PVS_AMP_FREQ) || (p->fout->format==PVS_AMP_PHASE))
      return initerror("pvsmix: signal format must be amp-phase or amp-freq.\n");

    return OK;
}

int pvsmix(PVSMIX *p)
{
    int i;
    long framesize;
    int test;
    float *fout,*fa, *fb;

    if (!fsigs_equal(p->fa,p->fb))
      return perferror("pvsmix : formats are different.\n");
    fout = (float *) p->fout->frame.auxp;
    fa = (float *) p->fa->frame.auxp;
    fb = (float *) p->fb->frame.auxp;

    framesize = p->fa->N + 2;

    if(p->lastframe < p->fa->framecount) {
      for (i=0;i < framesize;i+=2){

        test = fa[i] >= fb[i];
        if(test){
          fout[i] = fa[i];
          fout[i+1] = fa[i+1];
        }
        else {
          fout[i] = fb[i];
          fout[i+1] = fb[i+1];
        }

      }
      p->fout->framecount = p->lastframe = p->fa->framecount;
    }
    return OK;
}

/* pvsfilter  */

int pvsfilterset(PVSFILTER *p)
{
    long N = p->fin->N;


    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format==PVS_AMP_FREQ) || (p->fout->format==PVS_AMP_PHASE))
      return
        initerror("pvsfilter: signal format must be amp-phase or amp-freq.\n");

    return OK;
}

int pvsfilter(PVSFILTER *p)
{
    long i,N = p->fout->N;
    float g = (float) *p->gain;
    MYFLT dirgain, kdepth = (MYFLT) *p->kdepth;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;
	float *fil = (float *) p->fil->frame.auxp;

    if(fout==NULL)
      return perferror("pvsfilter: not initialised\n");
    if (!fsigs_equal(p->fin,p->fil))
      return perferror("pvsfilter: formats are different.\n");

    if(p->lastframe < p->fin->framecount) {
      kdepth = kdepth >= 0 ? (kdepth <= 1 ? kdepth : (MYFLT)1.0): (MYFLT)0.0 ;
      dirgain = 1-kdepth;
      for(i=0;i < N+2;i+=2) {
        fout[i] = (float)(fin[i]*(dirgain + fil[i]*kdepth))*g;
        fout[i+1] = fin[i+1];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

/* pvscale  */

int pvsscaleset(PVSSCALE *p)
{
    long N = p->fin->N;

    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);     /* RWD MUST be 32bit */
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    return OK;
}

int pvsscale(PVSSCALE *p)
{
    long i,chan,newchan,N = p->fout->N;
    float max = 0.f;
    MYFLT pscal = (MYFLT) fabs(*p->kscal);
    int  keepform = (int) *p->keepform;
    float g = (float) *p->gain;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if(fout==NULL)
      return perferror("pvsscale: not initialised\n");

    if(p->lastframe < p->fin->framecount) {

      fout[0] = fin[0];
      fout[N] = fin[N];

      for(i=2;i<N;i+=2){
        max =  max < fin[i] ? fin[i] : max;
        fout[i] = 0.f;
        fout[i+1] = -1.0f;
      }

      for(i=2,chan=1;i < N;chan++,i+=2) {

        newchan = (int)(chan*pscal)<<1;

        if(newchan < N && newchan > 0){
          fout[newchan] = keepform ?
            (keepform == 1 || !max  ? fin[newchan] : fin[i]*(fin[newchan]/max))
            : fin[i];
          fout[newchan+1] = (float) (fin[i+1]*pscal);
        }
      }

      for(i=2;i<N;i+=2){
        if(fout[i+1] == -1.f) fout[i] = 0.f;
        else fout[i] *= g;
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

/* pvshift */

int pvsshiftset(PVSSHIFT *p)
{
    long N = p->fin->N;

    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);     /* RWD MUST be 32bit */
    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    return OK;
}

int pvsshift(PVSSHIFT *p)
{
    long i,chan,newchan,N = p->fout->N;
    MYFLT pshift = (MYFLT) *p->kshift;
    int lowest =  abs((int)(*p->lowest*N/esr));
    float max = 0.f;
    int cshift = (int)(pshift*N/esr);
    int  keepform = (int) *p->keepform;
    float g = (float) *p->gain;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if(fout==NULL)
      return perferror("pvsshift: not initialised\n");

    if(p->lastframe < p->fin->framecount) {

      lowest = lowest ? (lowest > N/2 ? N/2 : lowest<<1 ) : 2;

      fout[0] = fin[0];
      fout[N] = fin[N];

      for(i=2;i<N;i+=2){
        max =  max < fin[i] ? fin[i] : max;

        if(i < lowest) {
          fout[i] = fin[i];
          fout[i+1] = fin[i+1];
        }
        else {
          fout[i] = 0.f;
          fout[i+1] = -1.f;
        }

      }

      for(i=lowest,chan=lowest>>1;i < N;chan++,i+=2) {

        newchan = (chan+cshift)<<1;

        if(newchan < N && newchan > lowest){
          fout[newchan] = keepform ?
            (keepform == 1 || !max ? fin[newchan] : fin[i]*(fin[newchan]/max))
            : fin[i];
          fout[newchan+1] = (float) (fin[i+1]+pshift);
        }
      }

      for(i=lowest;i<N;i+=2){
        if(fout[i+1] == -1.f) fout[i] = 0.f;
        else fout[i] *= g;
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}
/* pvsblur */

int pvsblurset(PVSBLUR *p)
{
    float* delay;
    long N = p->fin->N, i, j;
    int olap = p->fin->overlap;
    int delayframes, framesize = N+2 ;
    p->frpsec = esr/olap;

    delayframes = (int)(*p->maxdel*p->frpsec);

    if(p->fout->frame.auxp==NULL)
		auxalloc((N+2)*sizeof(float),&p->fout->frame);

    if(p->delframes.auxp==NULL)
      auxalloc((N+2)*sizeof(float)*delayframes, &p->delframes);

    delay = (float *) p->delframes.auxp;

    for(j=0; j < framesize*delayframes; j+=framesize)
      for(i=0; i < N+2; i+=2){
        delay[i+j] = 0.f;
        delay[i+j+1] = i*esr/N;
      }

    p->fout->N =  N;
    p->fout->overlap = olap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    p->count = 0;
    return OK;
}

int pvsblur(PVSBLUR *p)
{
    long j,i,N = p->fout->N, first, framesize=N+2;
    long countr = p->count;
    double amp=0., freq=0.;
    int delayframes = (int) (*p->kdel*p->frpsec);
    int kdel = delayframes*framesize;
    int mdel = (int)(*p->maxdel*p->frpsec)*framesize;
    float *fin = (float *) p->fin->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;
    float *delay = (float *) p->delframes.auxp;

    if(fout==NULL || delay==NULL)
      return perferror("pvsblur: not initialised\n");

    if(p->lastframe < p->fin->framecount) {

      kdel = kdel >= 0 ? (kdel < mdel ? kdel : mdel-framesize): 0;

      for(i=0;i < N+2;i+=2) {

        delay[countr+i] = fin[i];
        delay[countr+i+1] = fin[i+1];

        if(kdel){

          if((first = countr-kdel)< 0)
            first += mdel;

          for(j=first; j != countr; j=(j+framesize)%mdel){
            amp += delay[j+i];
            freq += delay[j+i+1];

          }

          fout[i] = (float)(amp/delayframes);
          fout[i+1] = (float)(freq/delayframes);
          amp = freq = 0.;
        }
        else {
          fout[i] = fin[i];
          fout[i+1] = fin[i+1];

        }

      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
      countr+=(N+2);
      p->count = countr < mdel ? countr : 0;

    }
    return OK;
}


/* pvstencil  */
int pvstencilset(PVSTENCIL *p)
{
    long N = p->fin->N, i;
    long chans = N/2+1;
    MYFLT *ftable;

    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);

    p->fout->N =  N;
    p->fout->overlap = p->fin->overlap;
    p->fout->winsize = p->fin->winsize;
    p->fout->wintype = p->fin->wintype;
    p->fout->format = p->fin->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    if (!(p->fout->format==PVS_AMP_FREQ) || (p->fout->format==PVS_AMP_PHASE))
      return
        initerror("pvstencil: signal format must be amp-phase or amp-freq.\n");

    p->func = ftfind(p->ifn);
    if (p->func==NULL)
      return OK;

    if (p->func->flen + 1 < chans)
      return initerror("pvstencil: ftable needs to equal the number of bins");

    ftable = p->func->ftable;
    for (i=0;i < p->func->flen+1;i++)
      if (ftable[i] < (MYFLT)0.0 ) ftable[i] = (MYFLT)0.0 ;

    return OK;
}

int pvstencil(PVSTENCIL *p)
{
    long framesize, i, j;
    int test;
    float *fout,*fin;
    MYFLT *ftable;
    float g = (float) fabs(*p->kgain);
    float masklevel = (float) fabs(*p->klevel);

    fout = (float *) p->fout->frame.auxp;
    fin =(float *) p->fin->frame.auxp;
    ftable = p->func->ftable;

    framesize = p->fin->N + 2;

     if(fout==NULL)
       return perferror("pvstencil: not initialised\n");

    if(p->lastframe < p->fin->framecount){

      for (i=0, j=0;i < framesize;i+=2, j++){
        test = fin[i] > ftable[j]*masklevel;
        if(test)fout[i] = fin[i];
        else fout[i] = fin[i]*g;

        fout[i+1] = fin[i+1];
      }

      p->fout->framecount = p->lastframe = p->fin->framecount;
    }
    return OK;
}

int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
    if ((f1->overlap    == f2->overlap)
        && (f1->winsize == f2->winsize)
        && (f1->wintype == f2->wintype) /* harsh, maybe... */
        && (f1->N       == f2->N)
        && (f1->format  == f2->format)
        )
      return 1;
    return 0;

}


#define S       sizeof

static OENTRY localops[] = {
   {"pvscale", S(PVSSCALE), 3,"f", "fkop", (SUBR)pvsscaleset, (SUBR)pvsscale },
   {"pvshift", S(PVSSHIFT), 3,"f", "fkkop", (SUBR)pvsshiftset, (SUBR)pvsshift },
   {"pvsmix", S(PVSMIX),    3,"f", "ff",    (SUBR)pvsmixset, (SUBR)pvsmix, NULL},
   {"pvsfilter", S(PVSFILTER), 3, "f", "ffkp", (SUBR)pvsfilterset,(SUBR)pvsfilter},
   {"pvsblur", S(PVSBLUR), 3, "f", "fki", (SUBR)pvsblurset, (SUBR)pvsblur, NULL},
   {"pvstencil", S(PVSTENCIL), 3, "f", "fkki", (SUBR)pvstencilset, (SUBR)pvstencil}
};

LINKAGE
