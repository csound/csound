/*
  Copyright (C) 2005 Victor Lazzarini
  MIDI functionality (c) 2008 Peter Brinkmann

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
  compatible with csound 5
*/

#include <stdio.h>
#include <m_pd.h>
#include <pthread.h>
#include "csound.h"

#define CS_MAX_CHANS 32
#define MAXMESSTRING 16384

#define MIDI_QUEUE_MAX 1024
#define MIDI_QUEUE_MASK 1023

static t_class *csoundapi_class = 0;

typedef struct _channelname {
  t_symbol *name;
  MYFLT   value;
  struct _channelname *next;
} channelname;

typedef struct _midi_queue {
  int writep;
  int readp;
  unsigned char values[MIDI_QUEUE_MAX];
} midi_queue;

typedef struct t_csoundapi_ {
  t_object  x_obj;
  t_float   f;
  t_sample *outs[CS_MAX_CHANS];
  t_sample *ins[CS_MAX_CHANS];
  t_int     vsize;
  t_int     chans;
  t_int     pksmps;
  t_int     pos;
  t_int     cleanup;
  t_int     end;
  t_int     numlets;
  t_int     result;
  t_int     run;
  t_int     ver;
  char    **cmdl;
  int       argnum;
  channelname *iochannels;
  t_outlet *ctlout;
  t_outlet *bangout;
  t_int     messon;
  CSOUND   *csound;
  char     *csmess;
  t_symbol *curdir;
  midi_queue *mq;
  char *orc;
} t_csoundapi;

static int set_channel_value(t_csoundapi *x, t_symbol *channel, MYFLT value);
static MYFLT get_channel_value(t_csoundapi *x, char *channel);
static channelname *create_channel(channelname *ch, char *channel);
static void destroy_channels(channelname *ch);
static void in_channel_value_callback(CSOUND *csound,
                                      const char *name,
                                      void *val, const void *channelType);
static void out_channel_value_callback(CSOUND *csound,
                                       const char *name,
                                       void *val, const void *channelType);
static void csoundapi_event(t_csoundapi *x, t_symbol *s,
                            int argc, t_atom *argv);
static void csoundapi_run(t_csoundapi *x, t_floatarg f);
static void csoundapi_offset(t_csoundapi *x, t_floatarg f);
static void csoundapi_reset(t_csoundapi *x);
static void csoundapi_rewind(t_csoundapi *x);
static void csoundapi_open(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv);
static void *csoundapi_new(t_symbol *s, int argc, t_atom *argv);
static void csoundapi_destroy(t_csoundapi *x);
static void csoundapi_dsp(t_csoundapi *x, t_signal **sp);
static t_int *csoundapi_perform(t_int *w);
static void csoundapi_channel(t_csoundapi *x, t_symbol *s,
                              int argc, t_atom *argv);
static void csoundapi_control(t_csoundapi *x, t_symbol *s,t_float f);
static void csoundapi_set_channel(t_csoundapi *x, t_symbol *s, int argc,
                                  t_atom *argv);
static void message_callback(CSOUND *,int attr, const char *format,va_list valist);
static void csoundapi_mess(t_csoundapi *x, t_floatarg f);


static void csoundapi_midi(t_csoundapi *, t_symbol *, int, t_atom *);
static void csoundapi_noteon(t_csoundapi *, t_floatarg, t_floatarg, t_floatarg);
static void csoundapi_noteoff(t_csoundapi *, t_floatarg, t_floatarg, t_floatarg);
static void csoundapi_ctlchg(t_csoundapi *, t_floatarg, t_floatarg, t_floatarg);
static void csoundapi_pgmchg(t_csoundapi *, t_floatarg, t_floatarg);
static void csoundapi_polytouch(t_csoundapi *, t_floatarg, t_floatarg, t_floatarg);
static void csoundapi_touch(t_csoundapi *, t_floatarg, t_floatarg);
static void csoundapi_bend(t_csoundapi *, t_floatarg, t_floatarg);

static int open_midi_callback(CSOUND *cs, void **userData, const char *dev);
static int read_midi_callback(CSOUND *cs, void *userData,
                              unsigned char *mbuf, int nbytes);
static int close_midi_callback(CSOUND *cs, void *userData);

static void csoundapi_tabset(t_csoundapi *x, t_symbol *tab, t_float f);
static void csoundapi_tabget(t_csoundapi *x, t_symbol *tab, t_float f);

static void csoundapi_compile(t_csoundapi *x,  t_symbol *orc);


PUBLIC void csound6_tilde_setup(void)
{
    csoundapi_class =
      class_new(gensym("csound6~"), (t_newmethod) csoundapi_new,
                (t_method) csoundapi_destroy, sizeof(t_csoundapi),
                CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_dsp, gensym("dsp"),
                    (t_atomtype) 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_event,
                    gensym("event"), A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_reset,
                    gensym("reset"), 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_rewind,
                    gensym("rewind"), 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_open, gensym("open"),
                    A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_run, gensym("run"),
                    A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_offset,
                    gensym("offset"), A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_channel,
                    gensym("set"), A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_control,
                    gensym("control"), A_DEFSYMBOL, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_set_channel,
                    gensym("chnset"), A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_mess,
                    gensym("messages"), A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_tabset,
                    gensym("tabset"), A_DEFSYMBOL, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_tabget,
                    gensym("tabget"),  A_DEFSYMBOL, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_midi,
                    gensym("midi"), A_GIMME, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_noteon,
                    gensym("noteon"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_noteoff,
                    gensym("noteoff"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_ctlchg,
                    gensym("ctlchg"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_pgmchg,
                    gensym("pgmchg"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_polytouch,
                    gensym("polytouch"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_touch,
                    gensym("touch"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_bend,
                    gensym("bend"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(csoundapi_class, (t_method) csoundapi_compile,
                    gensym("compile"), A_DEFSYMBOL, 0);

    CLASS_MAINSIGNALIN(csoundapi_class, t_csoundapi, f);

    {
      int v1, v2, v3;
      v1 = csoundGetVersion();
      v3 = v1 % 10;
      v2 = (v1 / 10) % 100;
      v1 = v1 / 1000;
      post("\ncsound6~ 1.01\n"
           " A PD csound class using the Csound %d.%02d.%d API\n"
           "(c) V Lazzarini, 2005-2007\n", v1, v2, v3);
    }
    /* csoundInitialize(NULL,NULL,0); */

}


int isCsoundFile(char *in)
{
    int     len;
    int     i;
    const char *extensions[6] = {".csd", ".orc", ".sco",".CSD",".ORC",".SCO"};

    len = strlen(in);
    for (i = 0; i < len; i++, in++) {
      if (*in == '.')
        break;
    }
    if (*in == '.') {
      for(i=0; i<6;i++)
        if (strcmp(in,extensions[i])==0) return 1;
    }
    return 0;
}

static void *csoundapi_new(t_symbol *s, int argc, t_atom *argv)
{
    char  **cmdl;
    int     i;

    t_csoundapi *x = (t_csoundapi *) pd_new(csoundapi_class);

    x->csound = (CSOUND *) csoundCreate(x);
    outlet_new(&x->x_obj, gensym("signal"));
    x->orc = NULL;
    x->numlets = 1;
    x->result = 1;
    x->run = 1;
    x->chans = 1;
    x->cleanup = 0;
    x->cmdl = NULL;
    x->iochannels = NULL;
    x->csmess = malloc(MAXMESSTRING);
    x->messon = 1;
    x->curdir = canvas_getcurrentdir();
    if (argc == 1 && argv[0].a_type == A_FLOAT) {
      x->numlets = (t_int) atom_getfloat(&argv[0]);
      for (i = 1; i < x->numlets && i < CS_MAX_CHANS; i++) {
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("signal"),
                  gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
      }
    }
    else if (argc > 0) {
      cmdl = (char **) malloc(sizeof(char *) * (argc + 3));
      cmdl[0] = "csound";
      for (i = 1; i < argc + 1; i++) {
        cmdl[i] = (char *) malloc(64);
        atom_string(&argv[i - 1], cmdl[i], 64);
        if (*cmdl[i] != '-' && isCsoundFile(cmdl[i])) {
#ifndef WIN32
          if(*cmdl[i] != '/')
#else
          if(cmdl[i][1] != ':')
#endif
            {
              char *tmp = cmdl[i];
              cmdl[i] =
                (char *)  malloc(strlen(tmp) + strlen(x->curdir->s_name) + 2);
              strcpy(cmdl[i], x->curdir->s_name);
              strcat(cmdl[i],"/");
              strcat(cmdl[i],tmp);
              post(cmdl[i]);
              free(tmp);
            }
        }
        post(cmdl[i]);
      }
      cmdl[i] = "-d";
      x->argnum = argc + 2;
      x->cmdl = cmdl;

      csoundSetHostImplementedAudioIO(x->csound, 1, 0);
      csoundSetInputChannelCallback(x->csound, in_channel_value_callback);
      csoundSetOutputChannelCallback(x->csound, out_channel_value_callback);

      csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
      csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
      csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);

      csoundSetMessageCallback(x->csound, message_callback);
      x->result = csoundCompile(x->csound, x->argnum, cmdl);


      if (!x->result) {
        x->end = 0;
        x->cleanup = 1;
        x->chans = csoundGetNchnls(x->csound);
        x->pksmps = csoundGetKsmps(x->csound);
        x->numlets = x->chans;
        for (i = 1; i < x->numlets && i < CS_MAX_CHANS; i++)
          inlet_new(&x->x_obj, &x->x_obj.ob_pd,
                    gensym("signal"), gensym("signal"));
        for (i = 1; i < x->numlets && i < CS_MAX_CHANS; i++)
          outlet_new(&x->x_obj, gensym("signal"));
        x->pos = 0;
      }
      else
        post("csound6~ warning: could not compile");
    }
    x->ctlout = outlet_new(&x->x_obj, gensym("list"));
    x->bangout = outlet_new(&x->x_obj, gensym("bang"));
    return (void *) x;
}

static void csoundapi_destroy(t_csoundapi *x)
{
    if (x->cmdl != NULL) free(x->cmdl);
    if (x->iochannels != NULL)
      destroy_channels(x->iochannels);
    csoundDestroy(x->csound);
    free(x->csmess);

}

static void csoundapi_dsp(t_csoundapi *x, t_signal **sp)
{
    int     i, numlets = x->numlets;

    x->vsize = sp[0]->s_n;

    for (i = 0; i < numlets; i++) {
      x->ins[i] = (t_sample *) sp[i]->s_vec;
      x->outs[i] = (t_sample *) sp[i + numlets]->s_vec;
    }

    if (!x->result) {
      dsp_add((t_perfroutine) csoundapi_perform, 1, x);
    }
    else
      post("csound6~ warning: orchestra not compiled");
}

static t_int *csoundapi_perform(t_int *w)
{
    t_csoundapi *x = (t_csoundapi *) w[1];
    t_int   size = x->vsize;
    t_int   pos = x->pos;
    t_int   posn = pos;
    t_float scal = csoundGet0dBFS(x->csound);
    t_int   pksmps = x->pksmps;
    t_int   numlets = x->numlets;
    t_int   chans = x->chans, samps;
    t_sample *out[CS_MAX_CHANS], *in[CS_MAX_CHANS];
    t_int   i, n, end = x->end, run = x->run;
    MYFLT  *csout, *csin;

    csout = csoundGetSpout(x->csound);
    csin = csoundGetSpin(x->csound);

    for (i = 0; i < numlets; i++) {
      in[i] = x->ins[i];
      out[i] = x->outs[i];
    }

    samps = pksmps * chans;
    for (i = 0; i < size; i++) {
      if (run && end == 0) {
        if (pos == samps) {
          if ((end = csoundPerformKsmps(x->csound)) != 0)
            outlet_bang(x->bangout);
          pos = 0;
          posn = 0;
        }
        for (n = 0; n < numlets; n++) {
          if (n < chans)
            csin[posn] = (MYFLT) in[n][i] * scal;
          posn++;
        }
        for (n = 0; n < numlets; n++) {
          out[n][i] = (t_float) (n < chans ? csout[pos] / scal : 0.0);
          pos++;
        }
      }
      else
        for (n = 0; n < numlets; n++)
          out[n][i] = 0.f;
    }

    x->end = end;
    x->pos = pos;
    return (t_int *) (w + 2);
}

static void csoundapi_event(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv)
{
    char    type[10];
    MYFLT   *pFields;
    int     num = argc - 1, i;
    pFields  = (MYFLT *) malloc(num*sizeof(MYFLT));

    if (!x->result) {
      atom_string(&argv[0], type, 10);
      if (type[0] == 'i' || type[0] == 'f' || type[0] == 'e') {
        for (i = 1; i < argc; i++)
          pFields[i - 1] = atom_getfloat(&argv[i]);
        csoundScoreEvent(x->csound, type[0], pFields, num);
        x->cleanup = 1;
        x->end = 0;
      }
      else
        post("csound6~ warning: invalid realtime score event");
    }
    else
      post("csound6~ warning: not compiled");
    free(pFields);
}

static void csoundapi_reset(t_csoundapi *x)
{
    if (x->cmdl != NULL) {

      if (x->end && x->cleanup) {
        csoundCleanup(x->csound);
        x->cleanup = 0;
      }

      csoundReset(x->csound);
      csoundSetHostImplementedAudioIO(x->csound, 1, 0);
      csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
      csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
      csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);
      x->result = csoundCompile(x->csound, x->argnum, x->cmdl);

      if (!x->result) {
        x->end = 0;
        x->pos = 0;
        x->cleanup = 1;
      }
    }
}

static void csoundapi_rewind(t_csoundapi *x)
{
    if (!x->result) {
      csoundSetScoreOffsetSeconds(x->csound, (MYFLT) 0);
      csoundRewindScore(x->csound);
      csoundSetScorePending(x->csound, 1);
      x->end = 0;
      x->pos = 0;
      x->cleanup = 1;
    }
    else
      post("csound6~ warning: not compiled");
}

static void csoundapi_open(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv)
{
    char  **cmdl;
    int     i;

    if (x->end && x->cleanup) {
      csoundCleanup(x->csound);
      x->cleanup = 0;
    }

    if (x->cmdl != NULL)
      free(x->cmdl);
    csoundReset(x->csound);
    x->result = 1;
    cmdl = (char **) malloc(sizeof(char *) * (argc + 3));
    cmdl[0] = "csound";
    for (i = 1; i < argc + 1; i++) {
      cmdl[i] = (char *) malloc(64);
      atom_string(&argv[i - 1], cmdl[i], 64);
      if (*cmdl[i] != '-' && isCsoundFile(cmdl[i])) {
#ifndef WIN32
        if(*cmdl[i] != '/')
#else
        if(cmdl[i][1] != ':')
#endif
          {
            char *tmp = cmdl[i];
            cmdl[i] =
              (char *)  malloc(strlen(tmp) + strlen(x->curdir->s_name) + 2);
            strcpy(cmdl[i], x->curdir->s_name);
            strcat(cmdl[i],"/");
            strcat(cmdl[i],tmp);
            post(cmdl[i]);
            free(tmp);
          }
      }
      post(cmdl[i]);
    }
    cmdl[i] = "-d";
    x->argnum = argc + 2;
    x->cmdl = cmdl;
    csoundSetHostImplementedAudioIO(x->csound, 1, 0);

    csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
    csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
    csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);
    x->run = 0;
    x->result = csoundCompile(x->csound, x->argnum, cmdl);


    if (!x->result) {
      x->end = 0;
      x->cleanup = 1;
      x->chans = csoundGetNchnls(x->csound);
      x->pksmps = csoundGetKsmps(x->csound);
      x->pos = 0;
      csoundSetHostData(x->csound, x);
      if (x->chans != x->numlets)
        post("csound6~ warning: number of orchestra channels (%d)\n"
             "does not match number of PD in/outlets (%d)\n"
             "some channels will be muted", x->chans, x->numlets);
      x->run = 1;
    }
    else
      post("csound6~ warning: could not compile");
}

static void csoundapi_run(t_csoundapi *x, t_floatarg f)
{
    x->run = (int) f;
    post("csoundapi~ run: %d", x->run);
}

static void csoundapi_tabset(t_csoundapi *x, t_symbol *tab, t_float f)
{
    t_garray *pdarray;
    MYFLT    *cstable;
    t_word *pdarray_vec;
    int   cstabsize, i, size;
    int   pdarraysize;
    cstabsize = csoundGetTable(x->csound, &cstable, (int) f);
    if(cstabsize != -1) {
      pdarray =  (t_garray *) pd_findbyclass(tab, garray_class);
      if( pdarray != NULL) {
        garray_getfloatwords(pdarray, &pdarraysize, &pdarray_vec);
        size = cstabsize <= pdarraysize ? cstabsize : pdarraysize;
        for(i = 0; i < size; i++) {
          cstable[i] = (MYFLT) pdarray_vec[i].w_float;
        }
      }
      else {
        post ("csound6~: could not find array\n");
        return;
      }
    } else post("csound6~: csound table %d not found \n", (int) f);
}

static void csoundapi_tabget(t_csoundapi *x,  t_symbol *tab, t_float f)
{
    t_garray *pdarray;
    MYFLT    *cstable;
    t_word *pdarray_vec;
    int   cstabsize, i, size;
    int   pdarraysize;
    cstabsize = csoundGetTable(x->csound, &cstable, (int) f);
    if(cstabsize != -1) {
      pdarray =  (t_garray *) pd_findbyclass(tab, garray_class);
      if( pdarray != NULL) {
        garray_getfloatwords(pdarray, &pdarraysize, &pdarray_vec);
        size = cstabsize <= pdarraysize ? cstabsize : pdarraysize;
        for(i = 0; i < size; i++)
          pdarray_vec[i].w_float = (t_float) cstable[i];
        garray_redraw(pdarray);
      }
      else {
        post ("csound6~: could not find array\n");
        return;
      }
    } else post("csound6~: csound table %d not found \n", (int) f);
}

void *thread_func(void *p){
    t_csoundapi *pp = (t_csoundapi *) p;
    int size = 0;
    char c;
    char *orc, *orcfile;
    FILE *fp;

#ifndef WIN32
    if(*(pp->orc) != '/')
#else
      if(*(pp->orc) != ':')
#endif
        {
          orcfile =
            (char *)  malloc(strlen(pp->orc) + strlen(pp->curdir->s_name) + 2);
          strcpy(orcfile, pp->curdir->s_name);
          strcat(orcfile,"/");
          strcat(orcfile,pp->orc);
        }
      else orcfile = pp->orc;
    post(orcfile);

    fp = fopen(orcfile, "rb");
    if(fp != NULL) {
      while(!feof(fp))
        size += fread(&c,1,1,fp);

      if(size==0) {
        fclose(fp);
        return NULL;
      }

      orc = (char *) malloc(size+1);
      fseek(fp, 0, SEEK_SET);
      if(fread(orc,1,size,fp) > 0)
        csoundCompileOrc(pp->csound, orc);
      fclose(fp);
      free(orc);
    }
    else post("csound6~: could not open %s \n", pp->orc);

    free(orcfile);
    return NULL;
}


static void csoundapi_compile(t_csoundapi *x,  t_symbol *orc) {
    pthread_t thread;
    if(x->orc != NULL) free(x->orc);
    x->orc = strdup(orc->s_name);
    pthread_create(&thread, NULL, thread_func, x);
}


static void csoundapi_offset(t_csoundapi *x, t_floatarg f)
{
    csoundSetScoreOffsetSeconds(x->csound, (MYFLT) f);
}

static int set_channel_value(t_csoundapi *x, t_symbol *channel, MYFLT value)
{
    channelname *ch = x->iochannels;

    if (ch != NULL)
      while (strcmp(ch->name->s_name, channel->s_name)) {
        ch = ch->next;
        if (ch == NULL) {
          return 0;
        }
      }
    else
      return 0;
    ch->value = value;
    return 1;
}

static MYFLT get_channel_value(t_csoundapi *x, char *channel)
{
    channelname *ch;

    ch = x->iochannels;
    if (ch != NULL)
      while (strcmp(ch->name->s_name, channel)) {
        ch = ch->next;
        if (ch == NULL) {
          return (MYFLT) 0;
        }
      }
    else
      return (MYFLT) 0;
    return ch->value;
}

static channelname *create_channel(channelname *ch, char *channel)
{
    channelname *tmp = ch, *newch = (channelname *) malloc(sizeof(channelname));

    newch->name = gensym(channel);
    newch->value = 0.f;
    newch->next = tmp;
    ch = newch;
    return ch;
}

static void destroy_channels(channelname *ch)
{
    channelname *tmp = ch;

    while (ch != NULL) {
      tmp = ch->next;
      free(ch);
      ch = tmp;
    }
}

static void csoundapi_channel(t_csoundapi *x, t_symbol *s,
                              int argc, t_atom *argv)
{
    int     i;
    char    chs[64];

    for (i = 0; i < argc; i++) {
      atom_string(&argv[i], chs, 64);
      x->iochannels = create_channel(x->iochannels, chs);
    }
}

static void csoundapi_control(t_csoundapi *x, t_symbol *s, t_float f)
{
    if (!set_channel_value(x, s, f))
      post("channel not found");
}

static void in_channel_value_callback(CSOUND *csound,
                                      const char *name, void *valp,
                                      const void *channelType)
{
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(csound);
    MYFLT *vp = (MYFLT *) valp;
    *vp = get_channel_value(x, (char *) name);

}

static void out_channel_value_callback(CSOUND *csound,
                                       const char *name, void *valp,
                                       const void *channelType)
{
    t_atom  at[2];
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(csound);
    MYFLT val = *((MYFLT *) valp);
    SETFLOAT(&at[1], (t_float) val);
    SETSYMBOL(&at[0], gensym((char *) name));
    outlet_list(x->ctlout, gensym("list"), 2, at);
}

static void csoundapi_set_channel(t_csoundapi *x, t_symbol *s,
                                  int argc, t_atom *argv){
    CSOUND *csound = x->csound;
    int i;
    char chn[64];

    for(i=0; i < argc; i+=2){
      atom_string(&argv[i],chn,64);
      if(i+1 < argc){
        if(argv[i+1].a_type == A_SYMBOL) {
          t_symbol *mess = atom_getsymbol(&argv[i+1]);
          csoundSetStringChannel(csound, chn, mess->s_name);
        }
        else if (argv[i+1].a_type == A_FLOAT) {
          csoundSetControlChannel(csound, chn, atom_getfloat(&argv[i+1]));
        }
      }
    }
}


static void csoundapi_mess(t_csoundapi *x, t_floatarg f)
{
    x->messon = (int) f;
}

static void message_callback(CSOUND *csound,
                             int attr, const char *format,va_list valist){
    int i;
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(csound);

    if(x->csmess != NULL)
      vsnprintf(x->csmess, MAXMESSTRING, format, valist);
    for(i=0;i<MAXMESSTRING;i++)
      if(x->csmess != NULL && x->csmess[i] == '\0'){
        x->csmess[i-1]= ' ';
        break;
      }
    if(x->csmess != NULL && x->messon) post(x->csmess);
}


static int open_midi_callback(CSOUND *cs, void **userData, const char *dev)
{
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(cs);
    midi_queue *mq = (midi_queue *) malloc(sizeof(midi_queue));
    if (mq == NULL) {
      error("unable to allocate memory for midi queue");
      return -1;
    }
    mq->writep = mq->readp = 0;
    x->mq = mq;

    post("midi in opened");
    return 0;
}

static int close_midi_callback(CSOUND *cs, void *userData)
{
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(cs);
    free(x->mq);
    x->mq=NULL;

    post("midi in closed");
    return 0;
}

static int read_midi_callback(CSOUND *cs, void *userData,
                              unsigned char *mbuf, int nbytes)
{
    t_csoundapi *x = (t_csoundapi *) csoundGetHostData(cs);
    midi_queue *mq = x->mq;
    if (mq == NULL) {
      return -1;
    }

    int wp = mq->writep;
    int rp = mq->readp;
    int nread = 0;
    unsigned char *val = mq->values;

    while ((rp != wp) && (nread < nbytes)) {
      *mbuf++ = val[rp++];
      rp &= MIDI_QUEUE_MASK;
      nread++;
    }

    mq->readp = rp;
    return nread;
}

#define MIDI_COMMON                                                     \
  midi_queue *mq = x->mq;                                               \
  if (mq == NULL) {                                                     \
    post("WARNING: midi disabled (launch with"                          \
         " '-+rtmidi=null -M0' to enable)");                            \
    return;                                                             \
  }                                                                     \
  if (!canvas_dspstate) {                                               \
    post("WARNING: dsp off, midi event ignored");                       \
    return;                                                             \
  }                                                                     \
  int wp = mq->writep;                                                  \
  unsigned char *val = mq->values;                                      \
  int mm;

#define MIDI_COMMAND(N, M)                              \
  mm = ((int) N) - 1;                                   \
  if ((mm & 0x0f) != mm) {                              \
    error("midi channel out of range: %d", mm + 1);     \
    return;                                             \
  }                                                     \
  val[wp++] = (mm | M);

#define MIDI_DATA(N, M, S)                      \
  mm = (int) N;                                 \
  if ((mm & M) != mm) {                         \
    error("midi " S " out of range: %d", mm);   \
    return;                                     \
  }                                             \
  val[wp++ & MIDI_QUEUE_MASK] = mm;

#define MIDI_FINISH                             \
  mq->writep = wp & MIDI_QUEUE_MASK;

static void csoundapi_midi(t_csoundapi *x, t_symbol *s, int argc, t_atom *argv)
{
    MIDI_COMMON;

    int i;
    for(i = 0; i<argc; i++) {
      if (argv[i].a_type != A_FLOAT) {
        error("midi parameter of wrong type");
        return;
      }
      MIDI_DATA(atom_getfloat(&argv[i]), 0xff, "value");
    }

    MIDI_FINISH;
}

static void csoundapi_noteon(t_csoundapi *x, t_floatarg note,
                             t_floatarg vel, t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0x90);
    MIDI_DATA(note, 0x7f, "note");
    MIDI_DATA(vel, 0x7f, "velocity");

    MIDI_FINISH;
}

static void csoundapi_noteoff(t_csoundapi *x, t_floatarg note,
                              t_floatarg vel, t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0x80);
    MIDI_DATA(note, 0x7f, "note");
    MIDI_DATA(vel, 0x7f, "velocity");

    MIDI_FINISH;
}

static void csoundapi_ctlchg(t_csoundapi *x, t_floatarg value,
                             t_floatarg ctrl, t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0xb0);
    MIDI_DATA(ctrl, 0x7f, "controller");
    MIDI_DATA(value, 0x7f, "value");

    MIDI_FINISH;
}

static void csoundapi_pgmchg(t_csoundapi *x, t_floatarg pgm,
                             t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0xc0);
    MIDI_DATA(pgm, 0x7f, "program");

    MIDI_FINISH;
}

static void csoundapi_polytouch(t_csoundapi *x, t_floatarg note,
                                t_floatarg vel, t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0xa0);
    MIDI_DATA(note, 0x7f, "note");
    MIDI_DATA(vel, 0x7f, "velocity");

    MIDI_FINISH;
}

static void csoundapi_touch(t_csoundapi *x, t_floatarg vel, t_floatarg chan)
{
    MIDI_COMMON;

    MIDI_COMMAND(chan, 0xd0);
    MIDI_DATA(vel, 0x7f, "velocity");

    MIDI_FINISH;
}

static void csoundapi_bend(t_csoundapi *x, t_floatarg bend, t_floatarg chan)
{
    MIDI_COMMON;

    int b = (int) bend;
    MIDI_COMMAND(chan, 0xe0);
    MIDI_DATA(b & 0x7f, 0x7f, "pitch bend lsb");
    MIDI_DATA(b >> 7, 0x7f, "pitch bend msb");

    MIDI_FINISH;
}
