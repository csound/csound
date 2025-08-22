/*
    schedule.c:

    Copyright (C) 1999, 2002 rasmus ekman, Istvan Varga,
                             John ffitch, Gabriel Maldonado, matt ingalls

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <math.h>
#include "csoundCore.h"
#include "namedins.h"
#include "inst_ops.h"
/* Keep Microsoft's schedule.h from being used instead of our schedule.h. */
#ifdef _MSC_VER
#include "H/schedule.h"
#else
#include "schedule.h"
#endif
#include "csound_standard_types.h"
#include "fgens.h"

void csound_input_message(CSOUND *, const char *);
void sense_line(CSOUND *csound, void *userData);

MYFLT named_instr_find(CSOUND *csound, char *s);
static const char *errmsg_1 =
  Str_noop("event: param 1 must be"
           " \"a\", \"i\", \"q\", \"f\", \"d\", or \"e\"");
static const char *errmsg_2 =
  Str_noop("event: string name is allowed only for"
           " \"i\", \"d\", and \"q\" events");

int32_t instr_num(CSOUND *csound, INSTRTXT *instr);
int32_t event_opcode_perf(CSOUND *csound, LINEVENT *p, int32_t pcnt,
                                 int32_t mode, char opcod)
{
    EVTBLK  evt;
    int32_t  res = OK;
    memset(&evt, 0, sizeof(EVTBLK));
    MYFLT insno;
    MYFLT   *aref = NULL;
    MYFLT **args = p->args;

    if (UNLIKELY((opcod != 'a' && opcod != 'i' && opcod != 'q' && opcod != 'f' &&
                  opcod != 'e' && opcod != 'd')))
      return csound->PerfError(csound, &(p->h), "%s", Str(errmsg_1));
    evt.strarg = NULL; evt.scnt = 0;
    evt.opcod = opcod;
    evt.pcnt = pcnt;

    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (mode == 1) {
        if (UNLIKELY(evt.opcod != 'i' && evt.opcod != 'q' && opcod != 'd'))
          return csound->PerfError(csound, &(p->h), "%s", Str(errmsg_2));
        insno = named_instr_find(csound, ((STRINGDAT *) args[0])->data);
        if (UNLIKELY(insno == FL(0.0))) return NOTOK;
        aref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else if (mode == 2) {
        INSTREF *ref = (INSTREF *) args[0];
        if (UNLIKELY(evt.opcod != 'i' && evt.opcod != 'q' && opcod != 'd'))
          return csound->InitError(csound, "%s", Str(errmsg_2));
        insno = instr_num(csound, ref->instr);
        aref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else {
        if (IsStringCode(*args[0])) {
          insno = named_instr_find(csound, ((STRINGDAT *) args[0])->data);
          if (UNLIKELY(insno == FL(0.0))) return NOTOK;
          aref = p->args[0];
          args[0] = &insno;
        } else {                  /* Should check for valid instr num here */
          insno = FABS(*args[0]);
          if (UNLIKELY((opcod == 'i' || opcod == 'd') && (insno ==0 ||
                       insno > csound->engineState.maxinsno ||
                       !csound->engineState.instrtxtp[(int)insno]))) {
            csound->Message(csound, Str("Cannot Find Instrument %d. No action.\n"),
                           (int32_t) insno);
            return OK;
          }
        }
        evt.strarg = NULL; evt.scnt = 0;
      }
    }

    if(opcod == 'd') {
      evt.opcod = 'i';
      aref = p->args[0];
      insno = *(p->args[0])*(-1.0);
      args[0] = &insno;
    }

    if (opcod == 'e' && (int32_t) evt.pcnt >= 1 && *(args[0]) > 0) {
      MYFLT pfields[2] = {*args[0], *args[0]};
      evt.pcnt = 2;
      return insert_score_event_at_sample(csound, &evt, pfields,
                                          csound->icurTimeSamples);
    }

    if (UNLIKELY(insert_score_args_at_sample(csound, &evt, args,
                                              csound->icurTimeSamples) != 0))
      res = csound->PerfError(csound, &(p->h),
                               Str("event: error creating '%c' event"),
                               opcod);
    if(aref != NULL) args[0] = aref;
    return res;
}

int32_t event_opcode(CSOUND *csound, LINEVENT *p)
{
  int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
  return event_opcode_init(csound, p, pcnt, 0, p->opcod->data[0]);
}

int32_t event_opcode_S(CSOUND *csound, LINEVENT *p)
{
    int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
    return event_opcode_perf(csound, p, pcnt, 1, p->opcod->data[0]);
}

int32_t event_opcode_Instr(CSOUND *csound, LINEVENT *p)
{
    int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
    return event_opcode_perf(csound, p, pcnt, 2, p->opcod->data[0]);
}



/* i-time version of event opcode */
int32_t event_opcode_init(CSOUND *csound, LINEVENT *p, int32_t pcnt,
                                 int32_t mode, char opcod)
{
    EVTBLK  evt;
    int32_t err = 0;
    memset(&evt, 0, sizeof(EVTBLK));
    MYFLT insno;
    MYFLT *ref = NULL;
    MYFLT **args = p->args;
    evt.pcnt = pcnt;
   
    if (UNLIKELY((opcod != 'a' && opcod != 'i' && opcod != 'q' && opcod != 'f' &&
                  opcod != 'e' && opcod != 'd')))
      return csound->InitError(csound, "%s", Str(errmsg_1));
    evt.strarg = NULL; evt.scnt = 0;
    evt.opcod = opcod;

    if (evt.pcnt > 0) {
      if (mode == 1) {
        insno = named_instr_find(csound, ((STRINGDAT *) args[0])->data);
        if (UNLIKELY(insno == FL(0.0))) return NOTOK;
        ref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else if (mode == 2) {
        INSTREF *iref = (INSTREF *) args[0];
        if (UNLIKELY(evt.opcod != 'i' && evt.opcod != 'q' && opcod != 'd'))
          return csound->InitError(csound, "%s", Str(errmsg_2));
        insno = instr_num(csound, iref->instr);
        ref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else {
        evt.strarg = NULL; evt.scnt = 0;
        if (IsStringCode(*args[0])) {
          insno = named_instr_find(csound, ((STRINGDAT *)p->args[0])->data);
          if (UNLIKELY(insno == FL(0.0))) return NOTOK;
          ref = args[0];
          args[0] = &insno;
        }
        else {                  /* Should check for valid instr num here */
          insno = FABS(*args[0]);
          if (UNLIKELY((opcod == 'i' || opcod == 'd') && (insno ==0 ||
                       insno > csound->engineState.maxinsno ||
                       !csound->engineState.instrtxtp[(int)insno]))) {
            csound->Message(csound, Str("Cannot Find Instrument %d. No action.\n"),
                           (int32_t) insno);
            return OK;
          }
          evt.strarg = NULL; evt.scnt = 0;
        }
      }

    }
    if(opcod == 'd') {
      evt.opcod = 'i';
      insno = *args[0]*(-1.0);
      ref = args[0];
      args[0] = &insno;
    }


    if (opcod == 'e' && (int32_t) evt.pcnt >= 1 && *args[0] > 0) {
      MYFLT pfields[2] = {*args[0], *args[0]};
      evt.pcnt = 2;
      err = insert_score_event_at_sample(csound, &evt, pfields,
                                         csound->icurTimeSamples);
    }
    else
      err = insert_score_args_at_sample(csound, &evt, args,
                                         csound->icurTimeSamples);
    if (UNLIKELY(err))
      csound->InitError(csound, Str("event_i: error creating '%c' event"),
                                opcod);
    if(ref != NULL) args[0] = ref;
      
    return (err == 0 ? OK : NOTOK);
}

int32_t event_opcode_i(CSOUND *csound, LINEVENT *p){
  int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
  return event_opcode_init(csound, p, pcnt, 0, p->opcod->data[0]);
}

int32_t event_opcode_i_S(CSOUND *csound, LINEVENT *p){
  int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
  return event_opcode_init(csound, p, pcnt, 1, p->opcod->data[0]);
}

int32_t event_opcode_i_Instr(CSOUND *csound, LINEVENT *p){
  int32_t pcnt = (int32_t) (p->INOCOUNT - 1);
  return event_opcode_init(csound, p, pcnt, 2, p->opcod->data[0]);
}
#include "csound_standard_types.h"

int32_t instance_opcode(CSOUND *csound, LINEVENT2 *p,
                        int32_t mode)
{
    EVTBLK  evt;
    int32_t  res = OK;
    evt.strarg = NULL; evt.scnt = 0;
    evt.opcod = 'i';
    evt.pcnt = p->INOCOUNT;
    MYFLT *aref = NULL;
    MYFLT insno;
    MYFLT **args = p->args;
    
     /* pass in the memory to hold the instance after insertion */
    evt.pinstance = (void *) p->inst;
    *((MYFLT **)evt.pinstance) = NULL;
    
    /* IV - Oct 31 2002: allow string argument */
    if (evt.pcnt > 0) {
      if (mode == 2) {
        INSTREF *ref = (INSTREF *) args[0];
        insno = instr_num(csound, ref->instr);
        aref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else if (mode == 1) {
        insno = csound->StringArg2Insno(csound,
                                   ((STRINGDAT*) args[0])->data, 1);
        if (UNLIKELY(insno  == 0)) return NOTOK;
        aref = args[0];
        args[0] = &insno;
        evt.strarg = NULL; evt.scnt = 0;
      }
      else {
        if (IsStringCode(*args[0])) {
          insno = csound->StringArg2Insno(csound,
                                     get_arg_string(csound, *args[0]), 1);
         if (UNLIKELY(insno == 0)) return NOTOK;
         aref = args[0];
          args[0] = &insno;
        } 
        evt.strarg = NULL; evt.scnt = 0;
      }
    }
    if (insert_score_args_at_sample(csound, &evt, args,
                                     csound->icurTimeSamples) != 0) {
        csound->Message(csound, Str("instance: error creating event\n"));
        res = NOTOK;
      }

    if(aref != NULL) args[0] = aref;
    return res;
}

int32_t instance_opcode_(CSOUND *csound, LINEVENT2 *p)
{
    return instance_opcode(csound, p, 0);
}

int32_t instance_opcode_S(CSOUND *csound, LINEVENT2 *p)
{
    return instance_opcode(csound, p, 1);
}

int32_t instance_opcode_Instr(CSOUND *csound, LINEVENT2 *p)
{
    return instance_opcode(csound, p, 2);
}

int32_t schedule_array(CSOUND *csound, SCHED *p)
{
    EVTBLK pp = {0};
    ARRAYDAT *pfields = (ARRAYDAT *) p->which;
    MYFLT *args = pfields->data;
    pp.opcod = 'i';
    pp.pcnt = pfields->sizes[0];
    insert_score_event_at_sample(csound, &pp, args, csound->icurTimeSamples);
    return OK;
}


int32_t schedule(CSOUND *csound, SCHEDO *p)
{
  EVTBLK evt = {0};
  evt.opcod = 'i';
  evt.pcnt = p->INOCOUNT;

  if (GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_INSTR) {
    // handling argum[0] as instrument type
    MYFLT insno;
    int32_t res;
    INSTREF *ref = (INSTREF *) p->argums[0];
    insno = instr_num(csound, ref->instr);
    p->argums[0] = &insno;
    res = insert_score_args_at_sample(csound, &evt, p->argums,
                                  csound->icurTimeSamples);
    p->argums[0] = (MYFLT *) ref;
    return res;
  } else if (GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_S) {
    MYFLT insno;
    int32_t res;
    MYFLT *ref = p->argums[0];
    insno = named_instr_find(csound, ((STRINGDAT *) p->argums[0])->data);
    if (UNLIKELY(insno == FL(0.0))) return NOTOK;  
    p->argums[0] = &insno;
    res = insert_score_args_at_sample(csound, &evt, p->argums,
                                      csound->icurTimeSamples);
    p->argums[0] = ref;
    return res;
  }
  else if (GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_I ||
           GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_C ||
           GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_P ||
	   GetTypeForArg(p->argums[0]) == &CS_VAR_TYPE_K) 
    return insert_score_args_at_sample(csound, &evt, p->argums,
                                      csound->icurTimeSamples);
  
  else return csound->InitError(csound, "invalid instrument argument\n");
}



/* from aops.h */
int32_t instr_num(CSOUND *csound, INSTRTXT *instr);

static void add_string_arg(char *s, const char *arg) {
  int32_t offs = (int32_t) strlen(s) ;
  //char *c = s;
  s += offs;
  *s++ = ' ';

  *s++ ='\"';
  while(*arg != '\0') {
    if(*arg == '\"')
      *s++ = '\\';
    *s++ = *arg++;
  }

  *s++ = '\"';
  *s = '\0';
  //printf("%s \n", c);
}

void sensLine(CSOUND *csound, void *userData);

int32_t schedule_N(CSOUND *csound, SCHED *p)
{
    int32_t i;
    MYFLT insno = *p->which;
    int32_t argno = p->INOCOUNT+1;
    char s[16384], sf[64];
    if (GetTypeForArg(p->which) == &CS_VAR_TYPE_INSTR) {
      INSTREF *ref = (INSTREF *) p->which;
      insno = (MYFLT) instr_num(csound, ref->instr);
    } else if (GetTypeForArg(p->which) != &CS_VAR_TYPE_I &&
           GetTypeForArg(p->which) != &CS_VAR_TYPE_C &&
           GetTypeForArg(p->which) != &CS_VAR_TYPE_P)
      return csound->InitError(csound, "instrument argument invalid\n");
    
    snprintf(s, 16384, "i %f %f %f", insno, *p->when, *p->dur);
    for (i=4; i < argno ; i++) {
       MYFLT *arg = p->argums[i-4];
       if (csoundGetTypeForArg(arg) == &CS_VAR_TYPE_S) {
           add_string_arg(s, ((STRINGDAT *)arg)->data);
       }
       else {
         snprintf(sf, 64, " %f", *arg);
         if(strlen(s) < 16384)
          strncat(s, sf, 16384-strlen(s));
       }
    }

    csound_input_message(csound, s);
    sense_line(csound, NULL);
    return OK;
}

int32_t schedule_SN(CSOUND *csound, SCHED *p)
{
    int32_t i;
    int32_t argno = p->INOCOUNT+1;
    // compensate for sensline happening at the end of kcycle
    MYFLT when = *p->when < 1/csound->ekr ?
      *p->when : *p->when - 1/csound->ekr; 
    char s[16384], sf[64];
    snprintf(s, 16384, "i \"%s\" %f %f", ((STRINGDAT *)p->which)->data, when, *p->dur);
    for (i=4; i < argno ; i++) {
       MYFLT *arg = p->argums[i-4];
        if (csoundGetTypeForArg(arg) == &CS_VAR_TYPE_S)
           add_string_arg(s, ((STRINGDAT *)arg)->data);
        else {
         snprintf(sf, 64, " %f", *arg);
         if(strlen(s) < 16384)
          strncat(s, sf, 16384-strlen(s));
       }
    }

    csound_input_message(csound, s);
    sense_line(csound, NULL);
    return OK;
}



int32_t ifschedule(CSOUND *csound, WSCHED *p)
{                       /* All we need to do is ensure the trigger is set */
    IGN(csound);
    p->todo = 1;
    return OK;
}

int32_t kschedule(CSOUND *csound, WSCHED *p)
{
    if (p->todo && *p->trigger != FL(0.0)) {
      LINEVENT pp = {0};
      int32_t i;
      pp.h = p->h;
      pp.args[0] = p->which;
      pp.args[1] = p->when;
      pp.args[2] = p->dur;
      pp.argno = p->INOCOUNT;
      for(i=3; i < pp.argno ; i++) {
        pp.args[i] = p->argums[i-3];
      }
      p->todo = 0;
      if (IS_STR_ARG(p->which)){
        return event_opcode_perf(csound, &pp, pp.argno, 1, 'i');
      }
      if (GetTypeForArg(p->which) == &CS_VAR_TYPE_INSTR){
        return event_opcode_perf(csound, &pp, pp.argno, 2, 'i');
      }
      else {
        return event_opcode_perf(csound, &pp, pp.argno, 0, 'i');
      }
    }
    else return OK;
}

/* tables are 4096 entries always */

#define MAXPHASE 0x1000000
#define MAXMASK  0x0ffffff
int32_t lfoset(CSOUND *csound, LFO *p)
{
  /* Types: 0:  sine
            1:  triangles
            2:  square (biplar)
            3:  square (unipolar)
            4:  saw-tooth
            5:  saw-tooth(down)
            */
    int32_t type = (int32_t)*p->type;
    if (type == 0) {            /* Sine wave so need to create */
      int32_t i;
      if (p->auxd.auxp==NULL) {
        csound->AuxAlloc(csound, sizeof(MYFLT)*4097L, &p->auxd);
        p->sine = (MYFLT*)p->auxd.auxp;
      }
      for (i=0; i<4096; i++)
        p->sine[i] = SIN(TWOPI_F*(MYFLT)i/FL(4096.0));
/*        csound->Message(csound,"Table set up (max is %d)\n", MAXPHASE>>10); */
    }
    else if (UNLIKELY(type>5 || type<0)) {
      return csound->InitError(csound, Str("LFO: unknown oscilator type %d"),
                                       type);
    }
    p->lasttype = type;
    p->phs = 0;
    return OK;
}

int32_t lfok(CSOUND *csound, LFO *p)
{
    int32_t     phs;
    MYFLT       fract;
    MYFLT       res;
    int32_t     iphs;

    phs = p->phs;
    switch (p->lasttype) {
    default:
      return csound->PerfError(csound, &(p->h),
                               Str("LFO: unknown oscilator type %d"),
                               p->lasttype);
    case 0:
      iphs = phs >> 12;
      fract = (MYFLT)(phs & 0xfff)/FL(4096.0);
      res = p->sine[iphs];
      res = res + (p->sine[iphs+1]-res)*fract;
      break;
    case 1:                     /* Trangular */
      res = (MYFLT)((phs<<2)&MAXMASK)/(MYFLT)MAXPHASE;
      if (phs < MAXPHASE/4) {}
      else if (phs < MAXPHASE/2)
        res = FL(1.0) - res;
      else if (phs < 3*MAXPHASE/4)
        res = - res;
      else
        res = res - FL(1.0);
      break;
    case 2:                     /* Bipole square wave */
      if (phs<MAXPHASE/2) res = FL(1.0);
      else res = -FL(1.0);
      break;
    case 3:                     /* Unipolar square wave */
      if (phs<MAXPHASE/2) res = FL(1.0);
      else res = FL(0.0);
      break;
    case 4:                     /* Saw Tooth */
      res = (MYFLT)phs/(MYFLT)MAXPHASE;
      break;
    case 5:                     /* Reverse Saw Tooth */
      res = FL(1.0) - (MYFLT)phs/(MYFLT)MAXPHASE;
      break;
    }
    phs += (int32_t)(*p->xcps * MAXPHASE * CS_ONEDKR);
    phs &= MAXMASK;
    p->phs = phs;
    *p->res = *p->kamp * res;
    return OK;
}

int32_t lfoa(CSOUND *csound, LFO *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     phs;
    MYFLT       fract;
    MYFLT       res;
    int32_t     iphs, inc;
    MYFLT       *ar, amp;

    phs = p->phs;
    inc = (int32_t)((*p->xcps * (MYFLT)MAXPHASE) * CS_ONEDSR);
    amp = *p->kamp;
    ar = p->res;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      switch (p->lasttype) {
      default:
        return csound->PerfError(csound, &(p->h),
                                 Str("LFO: unknown oscilator type %d"),
                                 p->lasttype);
      case 0:
        iphs = phs >> 12;
        fract = (MYFLT)(phs & 0xfff)/FL(4096.0);
        res = p->sine[iphs];
        res = res + (p->sine[iphs+1]-res)*fract;
        break;
      case 1:                   /* Triangular */
        res = (MYFLT)((phs<<2)&MAXMASK)/(MYFLT)MAXPHASE;
        if (phs < MAXPHASE/4) {}
        else if (phs < MAXPHASE/2)
          res = FL(1.0) - res;
        else if (phs < 3*MAXPHASE/4)
          res = - res;
        else
          res = res - FL(1.0);
        break;
      case 2:                   /* Bipole square wave */
        if (phs<MAXPHASE/2) res = FL(1.0);
        else res = -FL(1.0);
        break;
      case 3:                   /* Unipolar square wave */
        if (phs<MAXPHASE/2) res = FL(1.0);
        else res = FL(0.0);
        break;
      case 4:                   /* Saw Tooth */
        res = (MYFLT)phs/(MYFLT)MAXPHASE;
        break;
      case 5:                   /* Reverse Saw Tooth */
        res = FL(1.0) - (MYFLT)phs/(MYFLT)MAXPHASE;
        break;
      }
      phs += inc;
      phs &= MAXMASK;
      ar[n] = res * amp;
    }
    p->phs = phs;
    return OK;
}

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/* Changes made also to Cs.h, Musmon.c and Insert.c; look for "(re Aug 1999)" */
/******************************************************************************/

/******************************************************************************/
/* triginstr - Ignite instrument events at k-rate from orchestra.             */
/* August 1999 by rasmus ekman.                                               */
/******************************************************************************/

static void unquote(char *dst, char *src, int32_t maxsize)
{
    if (src[0] == '"') {
      //int32_t len = (int32_t) strlen(src) - 2;
      strNcpy(dst, src + 1, maxsize-1);
      //if (len >= 0 && dst[len] == '"') dst[len] = '\0';
    }
    else
      strNcpy(dst, src, maxsize);
}

static int32_t ktriginstr_(CSOUND *csound, TRIGINSTR *p, int32_t stringname);

int32_t triginset(CSOUND *csound, TRIGINSTR *p)
{
    p->prvmintim = *p->mintime;
    p->timrem = 0;
    /* An instrument is initialised before kcounter is incremented for
       this k-cycle, and begins playing after kcounter++.
       Therefore, if we should start something at the very first k-cycle of
       performance, we must thus do it now, lest it be one k-cycle late.
       But in ktriginstr() we'll need to use kcounter-1 to set the start time
       of new events. So add a separate variable for the kcounter offset (-1) */
    if (csound->global_kcounter == 0 &&
        *p->trigger != FL(0.0) /*&& *p->args[1] <= FL(0.0)*/) {
      p->kadjust = 0;   /* No kcounter offset at this time */
      ktriginstr_(csound, p, 0);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (csound->global_kcounter > 0 &&
        *p->trigger != FL(0.0) && p->h.insdshead->p3.value == 0)
      ktriginstr_(csound, p,0);
    return OK;
}

int32_t triginset_S(CSOUND *csound, TRIGINSTR *p)
{
    p->prvmintim = *p->mintime;
    p->timrem = 0;
    /* An instrument is initialised before kcounter is incremented for
       this k-cycle, and begins playing after kcounter++.
       Therefore, if we should start something at the very first k-cycle of
       performance, we must thus do it now, lest it be one k-cycle late.
       But in ktriginstr() we'll need to use kcounter-1 to set the start time
       of new events. So add a separate variable for the kcounter offset (-1) */
    if (csound->global_kcounter == 0 &&
        *p->trigger != FL(0.0) /*&& *p->args[1] <= FL(0.0)*/) {
      p->kadjust = 0;   /* No kcounter offset at this time */
      ktriginstr_(csound, p, 1);
    }
    p->kadjust = -1;      /* Set kcounter offset for perf-time */
    /* Catch p3==0 (i-time only) event triggerings. */
    if (csound->global_kcounter > 0 &&
        *p->trigger != FL(0.0) && p->h.insdshead->p3.value == 0)
      ktriginstr_(csound, p, 1);
    return OK;
}


static int32_t get_absinsno(CSOUND *csound, TRIGINSTR *p, int32_t stringname)
{
    int32_t insno;

    /* Get absolute instr num */
    /* IV - Oct 31 2002: allow string argument for named instruments */
    if (stringname)
      insno = (int32_t)string_arg_to_insno_p(csound, ((STRINGDAT*)p->args[0])->data);
    else if (IsStringCode(*p->args[0])) {
      char *ss = get_arg_string(csound, *p->args[0]);
      insno = (int32_t)string_arg_to_insno_p(csound, ss);
    }
    else
      insno = (int32_t)FABS(*p->args[0]);
    /* Check that instrument is defined */
    if (UNLIKELY(insno < 1 || insno > csound->engineState.maxinsno ||
                 csound->engineState.instrtxtp[insno] == NULL)) {
      csound->Warning(csound, Str("schedkwhen ignored. "
                                  "Instrument %d undefined\n"), insno);
      csound->perferrcnt++;
      return -1;
    }
    return insno;
}

static int32_t ktriginstr_(CSOUND *csound, TRIGINSTR *p, int32_t stringname)
{         /* k-rate event generator */
    int64_t  starttime;
    int32_t     i, argnum;
    EVTBLK  evt;
    char    name[512];
    memset(&evt, 0, sizeof(EVTBLK));
    char  pfields[PMAX+1] = {0};
    evt.p = (MYFLT *) pfields;

    if (p->timrem > 0)
      p->timrem--;
    if (*p->trigger == FL(0.0)) /* Only do something if triggered */
      return OK;

    /* Check if mintime has changed */
    if (p->prvmintim != *p->mintime) {
      int32_t timrem = (int32_t) (*p->mintime * CS_EKR + FL(0.5));
      if (timrem > 0) {
        /* Adjust countdown for new mintime */
        p->timrem += timrem - p->prvktim;
        p->prvktim = timrem;
      }
      else
        p->timrem = 0;
      p->prvmintim = *p->mintime;
    }

    if (*p->args[0] >= FL(0.0) || IsStringCode(*p->args[0])) {
      /* Check for rate limit on event generation */
      if (*p->mintime > FL(0.0) && p->timrem > 0)
        return OK;
      /* See if there are too many instances already */
      if (*p->maxinst >= FL(1.0)) {
        INSDS *ip;
        int32_t absinsno, numinst = 0;
        /* Count active instr instances */
        absinsno = get_absinsno(csound, p, stringname);
        if (UNLIKELY(absinsno < 1))
          return NOTOK;
        ip = &(csound->actanchor);
        while ((ip = ip->nxtact) != NULL)
          if (ip->insno == absinsno) numinst++;
        if (numinst >= (int32_t) *p->maxinst)
          return OK;
      }
    }

    /* Create the new event */
    if (stringname) {
      evt.p[1] = csound->StringArg2Insno(csound,((STRINGDAT *)p->args[0])->data, 1);
      evt.strarg = NULL; evt.scnt = 0;
      /*evt.strarg = ((STRINGDAT*)p->args[0])->data;
        evt.p[1] = SSTRCOD;*/
    }
    else if (IsStringCode(*p->args[0])) {
      unquote(name, get_arg_string(csound, *p->args[0]), 512);
      evt.p[1] = csound->StringArg2Insno(csound,name, 1);
      evt.strarg = NULL;
      /* evt.strarg = name; */
      evt.scnt = 0;
      /* evt.p[1] = SSTRCOD; */
    }
    else if (GetTypeForArg(p->args[0]) == &CS_VAR_TYPE_INSTR) {
      INSTREF *ref = (INSTREF *) p->args[0];
      evt.p[1] = (MYFLT) instr_num(csound, ref->instr); 
    }
    else {
      evt.strarg = NULL; evt.scnt = 0;
      evt.p[1] = *p->args[0];
    }
    evt.opcod = 'i';
    evt.pcnt = argnum = p->INOCOUNT - 3;
    /* Add current time (see note about kadjust in triginset() above) */
    starttime = CS_KSMPS*(csound->global_kcounter + p->kadjust);
    /* Copy all arguments to the new event */
    for (i = 1; i < argnum; i++)
      evt.p[i + 1] = *p->args[i];
    /* Set start time from kwhen */
    if (UNLIKELY(evt.p[2] < FL(0.0))) {
      evt.p[2] = FL(0.0);
      csound->Warning(csound,
                      Str("schedkwhen warning: negative kwhen reset to zero"));
    }
    /* Reset min pause counter */
    if (*p->mintime > FL(0.0))
      p->timrem = (int32_t) (*p->mintime * CS_EKR + FL(0.5));
    else
      p->timrem = 0;
    return
      (insert_score_event_at_sample(csound, &evt, evt.p+1,
                                    starttime) == 0 ? OK : NOTOK);
}

int32_t ktriginstr_S(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,1);
}

int32_t ktriginstr(CSOUND *csound, TRIGINSTR *p){
  return ktriginstr_(csound,p,0);
}

/* Maldonado triggering of events */

int32_t trigseq_set(CSOUND *csound, TRIGSEQ *p)      /* by G.Maldonado */
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) {
      return csound->InitError(csound, Str("trigseq: incorrect table number"));
    }
    p->done  = 0;
    p->table = ftp->ftable;
    p->pfn   = (int32_t)*p->kfn;
    p->ndx   = (int32_t)*p->initndx;
    p->nargs = p->INOCOUNT-5;
    return OK;
}

int32_t trigseq(CSOUND *csound, TRIGSEQ *p)
{
    if (p->done) return OK;
    else {
      int32_t j, nargs = p->nargs;
      int32_t start = (int32_t) *p->kstart, loop = (int32_t) *p->kloop;
      int32_t *ndx = &p->ndx;
      MYFLT **out = p->outargs;

      if (p->pfn != (int32_t)*p->kfn) {
        FUNC *ftp;
        if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL)) {
          return csound->PerfError(csound, &(p->h),
                                   Str("trigseq: incorrect table number"));
        }
        p->pfn = (int32_t)*p->kfn;
        p->table = ftp->ftable;
      }
      if (*p->ktrig) {
        int32_t nn = nargs * (int32_t)*ndx;
        for (j=0; j < nargs; j++) {
          *out[j] = p->table[nn+j];
        }
        if (loop > 0) {
          (*ndx)++;
          *ndx %= loop;
          if (*ndx == 0) {
            if (start == loop) {
              p->done = 1;      /* Was bug here -- JPff 2000 May 28*/
              return OK;
            }
            *ndx += start;
          }
        }
        else if (loop < 0) {
          (*ndx)--;
          while (*ndx < start) {
            if (start == loop) {
              p->done = 1;
              return OK;
            }
            *ndx -= loop + start;
          }
        }
      }
    }
    return OK;
}
char* get_arg_string_from_evt(CSOUND *csound, MYFLT p, EVTBLK *evt);

static int32_t events_match(CSOUND *csound,
                            EVTBLK *evt1, EVTBLK *evt2) {

  if(evt1->opcod == evt2->opcod &&
     evt1->p2orig == evt2->p2orig &&
     evt1->p3orig == evt2->p3orig &&
     evt1->p[1] == evt2->p[1] &&
     evt1->pcnt == evt2->pcnt) {
    int i;
    for(i = 4; i < evt1->pcnt+1; i++) {
      // TODO: encode string in evt1
      // for now we just ignore any string arg
      if(IsStringCode(evt2->p[i])) {
        char *str1 = get_arg_string_from_evt(csound, evt1->p[i],evt1); 
        char *str2 = get_arg_string_from_evt(csound, evt2->p[i],evt2);
        if(strcmp(str1, str2)) return 0;
      }
      else if(evt1->p[i] != evt2->p[i]) return 0;
    }
    return 1;
  }

  return 0;
}



static void remove_rt_event(CSOUND *csound, EVTBLK *evt, int32_t cont) {
  EVTNODE *e = csound->OrcTrigEvts;
  EVTBLK  *evtn = &(e->evt);
  while(e != NULL && e != csound->freeEvtNodes) {
    if(events_match(csound, evt, evtn)) {
      int i;
      csound->OrcTrigEvts = e->nxt;       
      e->nxt = csound->freeEvtNodes;
      csound->freeEvtNodes = e;
      if(csound->oparms->msglevel > 0) {
      csound->Message(csound, "unscheduled event: %c", evtn->opcod);
      for(i = 1; i < evtn->pcnt+1; i++) {
        if(IsStringCode(evtn->p[i]))
          csound->Message(csound, "%s ",
                          get_arg_string_from_evt(csound, evtn->p[i], evtn));
        else csound->Message(csound, "%.3f ", evtn->p[i]);
      }
      csound->Message(csound, "\n");
      }
      if(!cont) break;
    }
    e = e->nxt;
  }
}

void set_evt_strarg(CSOUND *csound, EVTBLK *e, int32_t pcnt, const
                    char *str);

int32_t remove_event_op(CSOUND *csound, RMEVT *p, int32_t cont) {
  EVTBLK evt;
  MYFLT pfields[VARGMAX] = {0};
  int i, pcnt = p->INOCOUNT;
  memset(&evt, 0, sizeof(EVTBLK));
  evt.p2orig = *p->arg[1];
  evt.p3orig = *p->arg[2];
  evt.pcnt = pcnt;
  evt.opcod = 'i';
  evt.p = pfields;
 
  // named instruments
  if(IS_STR_ARG(p->arg[0])) {
     evt.p[1] = named_instr_find(csound,
                                 ((STRINGDAT *)p->arg[0])->data);
  } else if(GetTypeForArg(p->arg[0]) == &CS_VAR_TYPE_INSTR){
    INSTREF *ref = (INSTREF *) p->arg[0];
    evt.p[1] =  instr_num(csound, ref->instr);
  }
  else evt.p[1] = *p->arg[0];
  
  for(i = 2; i < pcnt+1; i++) {
    // TODO: encode string args, ignored for now
    if(IS_STR_ARG(p->arg[i-1])) {
       STRINGDAT *str = (STRINGDAT *) p->arg[i-1];
       set_evt_strarg(csound, &evt, i, str->data);
      }
    else evt.p[i] = *p->arg[i-1];
  }
    
  remove_rt_event(csound, &evt, cont);
  if(evt.strarg != NULL) csound->Free(csound, evt.strarg);
  return OK;
}

int32_t remove_event(CSOUND *csound, RMEVT *p) {
  return remove_event_op(csound, p, 0);
}

int32_t remove_all_events(CSOUND *csound, RMEVT *p) {
  return remove_event_op(csound, p, 1);
}
