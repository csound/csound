/*
  Copyright (C) 2005 Victor Lazzarini

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

  csoundapi~: a PD class using the csound API
  compatible with csound 4.23 and 5

*/

#include <stdio.h>
#include <m_pd.h>
#include "csoundCore.h"
#include "csound.h"

#define CS_MAX_CHANS 32
#define CS_VERSION_  csoundGetVersion()
static t_class *csoundapi_class = 0;
static t_int lockcs = 0;

typedef struct _channelname {
  t_symbol *name;
  MYFLT     value;
  struct   _channelname *next;
} channelname;

typedef struct t_csoundapi_ {
  t_object x_obj;
  t_float f;
  t_sample *outs[CS_MAX_CHANS];
  t_sample *ins[CS_MAX_CHANS];
  t_int vsize;
  t_int chans;
  t_int pksmps;
  t_int pos;
  t_int cleanup;
  t_int end;
  t_int numlets;
  t_int result;
  t_int run;
  t_int ver;
  char** cmdl;
  int argnum;
  channelname *iochannels;
  t_outlet *ctlout;
  t_outlet *bangout;
  t_int padding1;
  ENVIRON *csound;
  t_int padding2;
} t_csoundapi;

PUBLIC int   set_channel_value(t_csoundapi *x, t_symbol *channel, MYFLT value);
PUBLIC MYFLT get_channel_value(t_csoundapi *x, char *channel);
PUBLIC channelname *create_channel(channelname *ch, char *channel);
PUBLIC void  destroy_channels(channelname *ch);
PUBLIC void in_channel_value_callback(void *csound, char *name, MYFLT *val);
PUBLIC void out_channel_value_callback(void *csound, char *name, MYFLT val);
PUBLIC void csoundapi_event(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv);
PUBLIC void csoundapi_run(t_csoundapi *x, t_floatarg f);
PUBLIC void csoundapi_offset(t_csoundapi *x, t_floatarg f);
PUBLIC void csoundapi_reset(t_csoundapi *x);
PUBLIC void csoundapi_rewind(t_csoundapi *x);
PUBLIC void csoundapi_open(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv);
PUBLIC void csoundapi_tilde_setup();
PUBLIC void *csoundapi_new(t_symbol *s, int argc, t_atom *argv);
PUBLIC void csoundapi_destroy(t_csoundapi *x);
PUBLIC void csoundapi_dsp(t_csoundapi *x, t_signal **sp);
PUBLIC t_int *csoundapi_perform(int *w);
PUBLIC void csoundapi_channel(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv);
PUBLIC void csoundapi_control(t_csoundapi *x, t_symbol *s, float f);

PUBLIC void csoundapi_tilde_setup(void) {

  csoundapi_class = class_new(gensym("csoundapi~"),(t_newmethod)csoundapi_new,
                              (t_method)csoundapi_destroy, sizeof(t_csoundapi),
                              CLASS_DEFAULT,A_GIMME, 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_dsp,
                  gensym("dsp"), (t_atomtype) 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_event,
                  gensym("event"), A_GIMME, 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_reset,
                  gensym("reset"), 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_rewind,
                  gensym("rewind"), 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_open,
                  gensym("open"), A_GIMME, 0);
  class_addmethod(csoundapi_class, (t_method)csoundapi_run,
                  gensym("run"), A_DEFFLOAT, 0);
  class_addmethod(csoundapi_class, (t_method)csoundapi_offset,
                  gensym("offset"), A_DEFFLOAT, 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_channel,
                  gensym("set"), A_GIMME, 0);
  class_addmethod(csoundapi_class, (t_method) csoundapi_control,
                  gensym("control"), A_DEFSYMBOL, A_DEFFLOAT, 0);

  CLASS_MAINSIGNALIN(csoundapi_class,t_csoundapi,f);

  post("\ncsoundapi~ 1.0\n A PD csound class using the Csound %2.2f API"
       "\n(c) V Lazzarini, 2005\n", CS_VERSION_/100.f);
  lockcs = 0;
}

PUBLIC void *csoundapi_new(t_symbol *s, int argc, t_atom *argv){

  char  **cmdl;
  int i;
  if(!lockcs) {
    t_csoundapi *x = (t_csoundapi *) pd_new(csoundapi_class);
    x->csound = (ENVIRON *)csoundCreate(x);
    if(CS_VERSION_ < 500) lockcs = 1;
    else lockcs = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    x->numlets = 1;
    x->result = 1;
    x->run = 1;
    x->chans = 1;
    x->cleanup = 0;
    x->cmdl = NULL;
    x->iochannels = NULL;
    csoundSetInputValueCallback(x->csound,in_channel_value_callback);
    csoundSetOutputValueCallback(x->csound,out_channel_value_callback);

    if(argc==1 && argv[0].a_type == A_FLOAT){
      x->numlets = (t_int) atom_getfloat(&argv[0]);
      for(i=1; i< x->numlets && i < CS_MAX_CHANS; i++){
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
      }
    }
    else if(argc>0) {
      cmdl = (char **) malloc(sizeof(char*)*(argc+3));
      cmdl[0]= (char *) strdup("csound");
      for(i=1; i < argc+1; i++){
        cmdl[i] = (char *)malloc(64);
        atom_string(&argv[i-1],cmdl[i],64);
        post(cmdl[i]);
      }
      cmdl[i]=  (char *)strdup("-d");
      cmdl[i+1]=  (char *)strdup("-n");

      x->argnum = argc+3;
      x->cmdl = cmdl;
      x->result = csoundCompile(x->csound, x->argnum, cmdl);

      if(!x->result){
        x->end = 0;
        x->cleanup = 1;
        x->chans = csoundGetNchnls(x->csound);
        x->pksmps = csoundGetKsmps(x->csound);
        x->numlets = x->chans;
        for(i=1; i< x->numlets && i < CS_MAX_CHANS; i++)
          inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"), gensym("signal"));
        for(i=1; i< x->numlets && i < CS_MAX_CHANS; i++)
          outlet_new(&x->x_obj, gensym("signal"));
        x->pos = 0;
      }
      else post("csoundapi~ warning: could not compile");
    }
    x->ctlout = outlet_new(&x->x_obj, gensym("list"));
    x->bangout = outlet_new(&x->x_obj, gensym("bang"));
    return (void *) x;
  }
  post("csoundapi~ warning: using API v.%1.2f multiple instances only with v.5.00",
       CS_VERSION_/100.f);
  return 0;

}

PUBLIC void csoundapi_destroy(t_csoundapi *x){
  if(x->cmdl!=NULL) free(x->cmdl);
  if(x->iochannels!=NULL) destroy_channels(x->iochannels);
  csoundDestroy(x->csound);
  lockcs=0;
}

PUBLIC void csoundapi_dsp(t_csoundapi *x, t_signal **sp){

  int i, numlets = x->numlets;
  x->vsize = sp[0]->s_n;

  for(i=0; i < numlets; i++){
    x->ins[i] = (t_sample *) sp[i]->s_vec;
    x->outs[i] = (t_sample *) sp[i+numlets]->s_vec;
  }

  if(!x->result){
    dsp_add((t_perfroutine) csoundapi_perform, 1, x);
  } else post("csoundapi~ warning: orchestra not compiled");

}

PUBLIC t_int *csoundapi_perform(int *w){

  t_csoundapi *x = (t_csoundapi *) w[1];
  t_int  size = x->vsize;
  t_int pos = x->pos;
  t_int posn = pos;
  t_float scal  = x->csound->e0dbfs;
  t_int pksmps = x->pksmps;
  t_int numlets = x->numlets;
  t_int chans = x->chans, samps;
  t_sample *out[CS_MAX_CHANS], *in[CS_MAX_CHANS];
  int i,n,end= x->end, run = x->run;
  MYFLT *csout, *csin;

  csout = csoundGetSpout(x->csound);
  csin = csoundGetSpin(x->csound);

  for(i=0; i < numlets; i++) {
    in[i] = x->ins[i];
    out[i] = x->outs[i];
  }

  samps = pksmps*chans;
  for(i=0; i<size; i++){
    if(run && end==0){
      if(pos==samps){
        if((end = csoundPerformKsmps(x->csound))!=0)
          outlet_bang(x->bangout);
        pos = 0;
        posn = 0;
      }
      for(n=0; n < numlets; n++){
        if(n < chans) csin[posn] = (MYFLT)in[n][i]*scal;
        posn++;
      }
      for(n=0; n < numlets; n++){
        out[n][i] =  (t_float)(n < chans ? csout[pos]/scal : 0.0);
        pos++;
      }
    } else for(n=0; n < numlets; n++) out[n][i] = 0.f;
  }

  x->end = end;
  x->pos = pos;
  return (t_int *)(w+2);

}

PUBLIC void csoundapi_event(t_csoundapi *x, t_symbol *s,
                            int argc, t_atom *argv){
  char type[10];
  MYFLT pFields[64];
  int num = argc-1, i;
  if(!x->result){
    atom_string(&argv[0],type,10);
    if(type[0] == 'i' || type[0] == 'f' || type[0] == 'e'){
      for(i=1; i <argc; i++) pFields[i-1] = atom_getfloat(&argv[i]);
      csoundScoreEvent(x->csound, type[0], pFields, num);
      x->cleanup = 1;
      x->end=0;
    }else post("csoundapi~ warning: invalid realtime score event");
  }else post("csoundapi~ warning: not compiled");
}

PUBLIC void csoundapi_reset(t_csoundapi *x){
  if(CS_VERSION_ >= 500){
    if(x->cmdl!=NULL){

      if(x->end && x->cleanup) {
        csoundCleanup(x->csound);
        x->cleanup = 0;
      }

      csoundReset(x->csound);
      x->result = csoundCompile(x->csound, x->argnum, x->cmdl);

      if(!x->result){
        x->end=0;
        x->pos=0;
        x->cleanup=1;
      }
    }
  }
  else post("not implemented in v.%1.2f\n", CS_VERSION_/100.f);
}

PUBLIC void csoundapi_rewind(t_csoundapi *x){
  if(!x->result){
    csoundSetScoreOffsetSeconds(x->csound, (MYFLT) 0);
    csoundRewindScore(x->csound);
    csoundSetScorePending(x->csound, 1);
    x->end=0;
    x->pos=0;
    x->cleanup=1;
  }else post("csoundapi~ warning: not compiled");
}

PUBLIC void csoundapi_open(t_csoundapi *x, t_symbol *s,
                           int argc, t_atom *argv){
  char  **cmdl;
  int i;
  if(CS_VERSION_ >= 500 || x->cmdl==NULL) {

    if(x->end && x->cleanup) {
      csoundCleanup(x->csound);
      x->cleanup = 0;
    }

    if(x->cmdl!=NULL) free(x->cmdl);
    csoundReset(x->csound);
    x->result = 1;
    cmdl = (char **) malloc(sizeof(char*)*(argc+3));
    cmdl[0]=  (char *)strdup("csound");
    for(i=1; i < argc+1; i++){
      cmdl[i] = (char *)malloc(64);
      atom_string(&argv[i-1],cmdl[i],64);
      post(cmdl[i]);
    }
    cmdl[i]=  (char *)strdup("-d");
    cmdl[i+1]=  (char *)strdup("-n");

    x->argnum = argc+3;
    x->cmdl = cmdl;
    x->result = csoundCompile(x->csound, x->argnum, cmdl);

    if(!x->result){
      x->end = 0;
      x->cleanup = 1;
      x->chans = csoundGetNchnls(x->csound);
      x->pksmps = csoundGetKsmps(x->csound);
      x->pos = 0;
      csoundSetHostData(x->csound, x);
      if(x->chans!=x->numlets)
        post("csoundapi~ warning: number of orchestra channels (%d)\n"
             "does not match number of PD in/outlets (%d)\n" "some channels will be muted",
             x->chans,x->numlets);
    }
    else post("csoundapi~ warning: could not compile");
  } else post("score re-opening not implemented in v.%1.2f", (float)CS_VERSION_/100.f);

}

PUBLIC void csoundapi_run(t_csoundapi *x, t_floatarg f){
  x->run = (int) f;
  post("csoundapi~ run: %d", x->run);
}

PUBLIC void csoundapi_offset(t_csoundapi *x, t_floatarg f){
  csoundSetScoreOffsetSeconds(x->csound, (MYFLT) f);
}

PUBLIC int
set_channel_value(t_csoundapi *x, t_symbol *channel, MYFLT value){
  channelname *ch = x->iochannels;
  if(ch != NULL)
    while(strcmp(ch->name->s_name, channel->s_name)){
      ch = ch->next;
      if(ch == NULL){
        return 0;
      }
    }
  else return 0;
  ch->value = value;
  return 1;
}

PUBLIC MYFLT
get_channel_value(t_csoundapi *x, char *channel){
  channelname *ch;
  ch = x->iochannels;
  if(ch != NULL)
    while(strcmp(ch->name->s_name, channel)){
      ch = ch->next;
      if(ch == NULL){
        return (MYFLT) 0;
      }
    }
  else return (MYFLT) 0;
  return ch->value;

}

PUBLIC channelname
*create_channel(channelname *ch, char *channel){

  channelname *tmp = ch, *newch = (channelname *) malloc(sizeof(channelname));
  newch->name = gensym(channel);
  newch->value = 0.f;
  newch->next = tmp;
  ch = newch;
  return ch;
}

PUBLIC void destroy_channels(channelname *ch){
  channelname *tmp  = ch;
  while(ch != NULL){
    tmp = ch->next;
    free(ch);
    ch = tmp;
  }
}

PUBLIC void csoundapi_channel(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv){
  int i;
  char chs[64];
  for(i=0; i < argc; i++){
    atom_string(&argv[i], chs, 64);
    x->iochannels = create_channel(x->iochannels, chs);
  }
}

PUBLIC void csoundapi_control(t_csoundapi *x, t_symbol *s, float f){

  if(!set_channel_value(x,s,f)) post("channel not found");

}

PUBLIC void in_channel_value_callback(void *csound, char *name, MYFLT *val){
  t_csoundapi *x = (t_csoundapi *) csoundGetHostData(csound);
  *val = get_channel_value(x,name);
}

PUBLIC void out_channel_value_callback(void *csound, char *name, MYFLT val){
  t_atom at[2];
  t_csoundapi *x = (t_csoundapi *) csoundGetHostData(csound);
  SETFLOAT(&at[1], (t_float) val);
  SETSYMBOL(&at[0], gensym(name));
  outlet_list(x->ctlout,gensym("list"), 2, at);
}

