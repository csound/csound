#include "csdl.h"

typedef struct {
    OPDS  h;
    MYFLT *ktrig, *unit_time, *kstart, *kloop, *initndx, *kfn;
    long  ndx;
    int   done, first_flag;
    double start, newtime;
    long  pfn;
    MYFLT *table, curr_unit_time;
} SEQTIM;


typedef struct {
    OPDS  h;
    MYFLT *ktrig, *ktrigin, *unit_time, *kstart, *kloop, *kinitndx, *kfn;
    long ndx;
    int done, first_flag;
    double start, newtime;
    long  pfn;
    MYFLT *table, curr_unit_time;
} SEQTIM2;

int seqtim_set(ENVIRON *csound, SEQTIM *p)   /* by G.Maldonado */
{
    FUNC *ftp;
    long start, loop;
    long *ndx = &p->ndx;
    p->pfn = (long) *p->kfn;
    if ((ftp = csound->FTFind(csound, p->kfn)) == NULL) {
      return csound->InitError(csound, Str("seqtime: incorrect table number"));
    }
    *ndx = (long) *p->initndx;
    p->done = 0;
    p->table =  ftp->ftable;
    if (p->ndx  > 0)
      p->newtime = p->table[p->ndx-1];
    else
      p->newtime = 0;
    p->start = csound->kcounter * onedkr ;
    start = (long) *p->kstart;
    loop = (long) *p->kloop;
    if (loop > 0) {
      *ndx %= loop;
      if (*ndx == 0) {
        *ndx += start;
      }
    }
    else if (loop < 0) {
      (*ndx)--;
      while (*ndx < start) {
        *ndx -= loop + start;
      }
    }
    p->curr_unit_time = *p->unit_time;
    p->first_flag= 1;
    return OK;
}

int seqtim(ENVIRON *csound, SEQTIM *p)
{
    if (p->done)
      *p->ktrig=FL(0.0);
    else {
      long start = (long) *p->kstart, loop = (long) *p->kloop;
      long *ndx = &p->ndx;
      if (p->pfn != (long)*p->kfn) {
        FUNC *ftp;
        if ((ftp = csound->FTFindP(csound, p->kfn)) == NULL) {
          return csound->PerfError(csound,
                                   Str("seqtime: incorrect table number"));
        }
        p->pfn = (long)*p->kfn;
        p->table = ftp->ftable;
      }

      if (p->curr_unit_time != *p->unit_time) {
        double constant = p->start - csound->kcounter * onedkr;
        double difference_new = p->newtime * p->curr_unit_time + constant;
        double difference_old = p->newtime * *p->unit_time     + constant;
        double difference = difference_new - difference_old;
        p->start = p->start + difference;
        p->curr_unit_time = *p->unit_time;
      }
      if (csound->kcounter * onedkr > p->newtime * *p->unit_time + p->start) {
        float curr_val = p->table[p->ndx];
        p->first_flag = 0;
        p->newtime += curr_val;
        if (loop > 0) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0){
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx += start;
          }
        }
        else if (loop < 0 ){
          (*ndx)--;
          while (p->ndx < 0) {
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx -= loop + start;
          }
        }
        *p->ktrig = curr_val * p->curr_unit_time;
      }
      else {
        if(p->first_flag) {
          *p->ktrig = p->table[p->ndx];
          p->first_flag=0;
        }
        else {
          *p->ktrig=FL(0.0);
        }
      }
    }
    return OK;
}



/**---------------------------------------**/


int seqtim2_set(ENVIRON *csound, SEQTIM2 *p)
{
    FUNC *ftp;
    long start, loop;
    long *ndx = &p->ndx;
    p->pfn = (long) *p->kfn;
    if ((ftp = csound->FTFind(csound, p->kfn)) == NULL) {
      return csound->InitError(csound, "seqtim: incorrect table number");
    }
    *ndx = (long) *p->kinitndx;
    p->done=0;
    p->table =  ftp->ftable;
    p->newtime = p->table[p->ndx];
    p->start = csound->kcounter * onedkr ;
    start = (long) *p->kstart;
    loop = (long) *p->kloop;
    if (loop > 0 ) {
      (*ndx)++;
      *ndx %= loop;
      if (*ndx == 0)  {
        *ndx += start;
      }
    }
    else if (loop < 0 ){
      (*ndx)--;
      while (*ndx < start) {
        *ndx -= loop + start;
      }
    }
    p->curr_unit_time = *p->unit_time;
    p->first_flag=1;
    return OK;
}

int seqtim2(ENVIRON *csound, SEQTIM2 *p)
{
    if (*p->ktrigin) {
      p->ndx = (long) *p->kinitndx;
    }

    if (p->done)
      goto end;
    else {
      long start = (long) *p->kstart, loop = (long) *p->kloop;
      long *ndx = &p->ndx;

      if (p->pfn != (long)*p->kfn) {
        FUNC *ftp;
        if ( (ftp = csound->FTFindP(csound, p->kfn) ) == NULL) {
          return csound->PerfError(csound, "seqtim: incorrect table number");
        }
        p->pfn = (long)*p->kfn;
        p->table = ftp->ftable;
      }
      if (p->curr_unit_time != *p->unit_time) {
        double constant = p->start - csound->kcounter * onedkr;
        double difference_new = p->newtime * p->curr_unit_time + constant;
        double difference_old = p->newtime * *p->unit_time     + constant;
        double difference = difference_new - difference_old;
        p->start = p->start + difference;
        p->curr_unit_time = *p->unit_time;
      }
      if (csound->kcounter * onedkr > p->newtime * *p->unit_time + p->start) {
        float curr_val = p->table[p->ndx];
        p->newtime += p->table[p->ndx];
        if (loop > 0 ) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0){
            if (start == loop) {
              p->done =1;
              return OK;
            }
            *ndx += start;
          }
        }
        else if (loop < 0 ){
          (*ndx)--;
          while (p->ndx < 0) {
            if (start == loop) {
              p->done =1;
              return OK;
            }
            *ndx -= loop + start;
          }
        }
        *p->ktrig = curr_val * p->curr_unit_time;
      }
      else {
        if(p->first_flag) {
          *p->ktrig = p->table[p->ndx];
          p->first_flag=0;
        }
        else {
        end:
          *p->ktrig=FL(0.0);
        }
      }
    }
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "seqtime", S(SEQTIM),  3, "k",    "kkkkk", (SUBR)seqtim_set, (SUBR)seqtim   },

{ "seqtime2", S(SEQTIM2),3, "k",    "kkkkkk", (SUBR)seqtim2_set, (SUBR)seqtim2}
};

LINKAGE

