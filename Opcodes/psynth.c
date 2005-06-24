

#include "csdl.h"
#include "pstream.h"

typedef struct _psyn {
OPDS h;
MYFLT *out;
PVSDAT *fin;
MYFLT  *scal, *pitch, *maxtracks, *ftb;
int tracks, pos, numbins, hopsize;
FUNC *func;
AUXCH sum, amps, freqs, phases, trackID;

} _PSYN;

int 
psynth_init(ENVIRON *csound, _PSYN *p){
int numbins = p->fin->N/2 + 1;
if(p->fin->format != PVS_TRACKS){
  csound->InitError(csound, "psynth: first input not in TRACKS format \n");
  return NOTOK;
}
p->func = csound->FTnp2Find(p->h.insdshead->csound,p->ftb);
if(p->func==NULL) {
      csound->InitError(csound, "psynth: function table not found\n");
      return NOTOK;
   }
   
p->tracks = 0;// (*p->maxtracks < p->numbins ? *p->maxtracks : p->numbins);   
p->hopsize = p->fin->overlap;
p->pos = 0;
p->numbins = numbins;

if(p->amps.auxp==NULL && p->amps.size < sizeof(MYFLT) * numbins)
		csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->amps);	 
if(p->freqs.auxp==NULL && p->freqs.size < sizeof(MYFLT) * numbins)
		csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->freqs);	 
if(p->phases.auxp==NULL && p->phases.size < sizeof(MYFLT) * numbins)
		csound->AuxAlloc(csound, sizeof(MYFLT) * numbins, &p->phases);
if(p->sum.auxp==NULL && p->sum.size < sizeof(MYFLT) * p->hopsize)
		csound->AuxAlloc(csound, sizeof(MYFLT) * p->hopsize, &p->sum);
if(p->trackID.auxp==NULL && p->trackID.size < sizeof(int) * numbins)
		csound->AuxAlloc(csound, sizeof(int) * numbins, &p->trackID);	 
		
		return OK;
}


int psynth_process(ENVIRON *csound, _PSYN *p){
	
	MYFLT ampnext,amp,freq,freqnext,phase, ratio;
	MYFLT a,f,frac,incra,incrph, factor;
	MYFLT scale = *p->scal, pitch = *p->pitch;
	int ndx, size = p->func->flen;
	int i, j, k, n, m, id;
	int notcontin = 0;
	int contin = 0;
	int tracks = p->tracks, maxtracks = *p->maxtracks;
	MYFLT *tab = p->func->ftable, *out = p->out;
	float *fin = (float *)p->fin->frame.auxp; 
	int ksmps = csound->ksmps, pos = p->pos;
	MYFLT *amps = (MYFLT *)p->amps.auxp, *freqs = (MYFLT *) p->freqs.auxp;
	MYFLT *phases = (MYFLT *)p->phases.auxp, *outsum = (MYFLT *) p->sum.auxp;
	int   *trackID = (int *)p->trackID.auxp;
	int hopsize = p->hopsize;
	
	ratio = size/csound->esr;
	factor = hopsize/csound->esr;
	
	maxtracks = p->numbins > maxtracks ? maxtracks : p->numbins;
	
	for(n=0; n < ksmps; n++) {
		out[n] = outsum[pos];
		pos++;
		if(pos == hopsize) {
			memset(outsum, 0, sizeof(MYFLT)*hopsize);
			/* for each track */
			i = j = k = 0;
			while(i < maxtracks*4){
				ampnext =  (MYFLT) fin[i]*scale;
				freqnext = (MYFLT) fin[i+1]*pitch; 
				if((id = (int) fin[i+3]) != -1){
					j = k+notcontin;
	
					if(k < tracks-notcontin){
						if(trackID[j]==id){	
							/* if this is a continuing track */  	
							contin = 1;	
							freq = freqs[j];
							phase = phases[j];
							amp = amps[j];
							
						}
						else {
							/* if this is  a dead track */
							contin = 0;
							freqnext = freq = freqs[j];
							phase = phases[j];
							amp = amps[j]; 
							ampnext = 0.f;
						}
					}
					else{ 
						/* new track */
						contin = 1;
						freq = freqnext;
						phase = -freq*factor;
						amp = 0.f;
	
					}
					/* interpolation & track synthesis loop */
						
					a = amp;
					f = freq;
					incra = (ampnext - amp)/hopsize;
					incrph = (freqnext - freq)/hopsize;
					for(m=0; m < hopsize; m++){
						/* table lookup oscillator */
						phase += f*ratio;
						while(phase < 0) phase += size;
						while(phase >= size) phase -= size;
						ndx = (int) phase;
						frac = phase -  ndx;
						outsum[m] += a*(tab[ndx] + (tab[ndx+1] - tab[ndx])*frac);
						a += incra;
						f += incrph;
					}
					/* keep amp, freq, and phase values for next time */
					if(contin){
						amps[k] = ampnext;
						freqs[k] = freqnext;
						phases[k] = phase;
						trackID[k] = id;    
						i += 4;
						k++;
					} else notcontin++;
				}
				else break;					
			}
			pos = 0;
			p->tracks = k;
		}
	}
p->pos = pos;


return OK;

}

static OENTRY localops[] = {
	{"tradsyn", sizeof(_PSYN), 5, "a", "fkkki", (SUBR)psynth_init, NULL, (SUBR)psynth_process },
};

LINKAGE