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

  commands.c tclcsound commands
*/

#include "tclcsound.h"
#ifdef MSVC
#include <windows.h>
#endif

#define CS_READY 0
#define CS_COMPILED 1
#define CS_RUNNING 2
#define CS_PAUSED 3

#define CHAN_NOT_FOUND 0
#define CHAN_FOUND 1
#define IN_CHAN 2
#define OUT_CHAN 3

/* performance thread */
uintptr_t csThread(void *clientData)
{
    int     result = 0, *status;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    status = &p->status;
    if (*status == CS_COMPILED) {
      *status = CS_RUNNING;
      if (csoundGetOutputBufferSize(cs)
          <= (csoundGetKsmps(cs) * csoundGetNchnls(cs))) {
        while (result == 0 && *status != -1)
          if (*status != CS_PAUSED)
            result = csoundPerformKsmps(cs);
          else
            result = 0;
      }
      else {
        while (result == 0 && *status != -1)
          if (*status != CS_PAUSED)
            result = csoundPerformBuffer(cs);
          else
            result = 0;
      }
      *status = CS_COMPILED;
      p->result = result;
    }
    return 0;
}

/* TCL commands  */

/* csCompile <arguments>
   compiles the given orc/score with given options
*/
int csCompile(ClientData clientData, Tcl_Interp * interp,
                int argc, char **argv)
{
    char    res[4];
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (p->status == CS_READY) {
      p->result = csoundCompile(cs, argc, argv);
      if (!p->result)
        p->status = CS_COMPILED;
      else
        csoundReset(cs);
      sprintf(res, "%d", p->result);
      Tcl_SetResult(interp, res, TCL_VOLATILE);
    }
    return (TCL_OK);
}

/* csCompileList arglist
   compiles the given orc/score with given options as a Tcl List
*/
int csCompileList(ClientData clientData, Tcl_Interp * interp,
                    int argc, char **argv)
{

    char   *cmd;
    char  **largv;
    int     largc;
    csdata *p = (csdata *) clientData;

    if (argc == 2) {
      cmd = (char *) malloc(16384);
      sprintf(cmd, "csound %s", argv[1]);
      Tcl_SplitList(interp, cmd, &largc, (CONST84 char ***) &largv);
      csCompile(p, interp, largc, largv);
      Tcl_Free((char *) largv);
      free(cmd);
    }
    return (TCL_OK);
}

/* csPerform
   performs the score, returning when finished
*/
int csPerform(ClientData clientData, Tcl_Interp * interp,
                int argc, char **argv)
{
    char    res[10];
    int     result;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (p->status == CS_COMPILED) {
      if (csoundGetOutputBufferSize(cs)
          <= (csoundGetKsmps(cs) * csoundGetNchnls(cs)))
        while (!(result = csoundPerformKsmps(cs)));
      else
        while (!(result = csoundPerformBuffer(cs)));
      sprintf(res, "%d", result);
      p->result = result;
    }
    else
      sprintf(res, "%d", -1);
    Tcl_SetResult(interp, res, TCL_VOLATILE);
    return (TCL_OK);
}

/* csPerformKsmps
 performs one ksmps block of audio output
 */
int csPerformKsmps(ClientData clientData, Tcl_Interp * interp,
                     int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    char    res[10];

    p->result = csoundPerformKsmps(cs);
    sprintf(res, "%d", p->result);
    Tcl_SetResult(interp, res, TCL_VOLATILE);
    return (TCL_OK);
}

/* csPerformBuffer
 performs one buffer-full of audio output
 */
int csPerformBuffer(ClientData clientData, Tcl_Interp * interp,
                      int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    char    res[10];

    p->result = csoundPerformBuffer(cs);
    sprintf(res, "%d", p->result);
    Tcl_SetResult(interp, res, TCL_VOLATILE);
    return (TCL_OK);
}

/* csPlay
   starts performance, returns immediately
*/
int csPlay(ClientData clientData, Tcl_Interp * interp, int argc, char **argv)
{

    char    res[10];
    csdata *p = (csdata *) clientData;

    if (p->status == CS_COMPILED) {
      p->threadID =
          csoundCreateThread((uintptr_t(*)(void *)) csThread, (void *) p);
      sprintf(res, "%d", p->result);
      Tcl_SetResult(interp, res, TCL_VOLATILE);
    }
    else if (p->status == CS_PAUSED) {
      p->status = CS_RUNNING;
      sprintf(res, "%d", 0);
      Tcl_SetResult(interp, res, TCL_VOLATILE);
    }
    return (TCL_OK);
}

/* csPause
   toggles on/off performance
*/
int csPause(ClientData clientData, Tcl_Interp * interp, int argc, char **argv)
{
    csdata *p = (csdata *) clientData;

    if (p->status == CS_PAUSED)
      p->status = CS_RUNNING;
    else if (p->status == CS_RUNNING)
      p->status = CS_PAUSED;
    return (TCL_OK);
}

/* csStop
   stops performance and puts csound ready for new compilation
*/
int csStop(ClientData clientData, Tcl_Interp * interp, int argc, char **argv)
{
    csdata *p = (csdata *) clientData;

    if (p->status == CS_RUNNING || p->status == CS_PAUSED) {
      p->status = -1;
      Tcl_Sleep(1000);
    }
    csoundReset(p->instance);
    p->status = CS_READY;
    return (TCL_OK);
}

/* csNote <arguments>
   starts a new i-statement according to arguments
*/
int csNote(ClientData clientData, Tcl_Interp * interp,
             int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    MYFLT   pFields[256];
    double  val;
    int     i;

    for (i = 1; i < argc; i++) {
      Tcl_GetDoubleFromObj(interp, argv[i], &val);
      pFields[i - 1] = (MYFLT) val;
    }
    if (p->status == CS_RUNNING || p->status == CS_COMPILED
        || p->status == CS_PAUSED) {
      p->result = csoundScoreEvent(cs, 'i', pFields, argc - 1);
      resp = Tcl_GetObjResult(interp);
      Tcl_SetIntObj(resp, p->result);
    }
    return (TCL_OK);
}

/* csNoteList arglist
   starts a new i-statement according to arglist (single Tcl list)
*/
int csNoteList(ClientData clientData, Tcl_Interp * interp,
                 int argc, char **argv)
{
    char  **largv;
    int     largc;
    MYFLT   pFields[256];
    int     i;
    char    res[10];
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (argc == 2) {
      Tcl_SplitList(interp, argv[1], &largc, (CONST84 char ***) &largv);
      for (i = 0; i < largc; i++)
        pFields[i] = (MYFLT) atof(largv[i]);
      if (p->status == CS_RUNNING || p->status == CS_COMPILED
          || p->status == CS_PAUSED) {
        p->result = csoundScoreEvent(cs, 'i', pFields, largc);
        sprintf(res, "%d", p->result);
        Tcl_SetResult(interp, res, TCL_VOLATILE);
      }
      Tcl_Free((char *) largv);
    }

    return (TCL_OK);
}

/* csTable
   starts a new f-statement
*/
int csTable(ClientData clientData, Tcl_Interp * interp,
              int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    MYFLT   pFields[256];
    double  val;
    int     i;

    for (i = 1; i < argc; i++) {
      Tcl_GetDoubleFromObj(interp, argv[i], &val);
      pFields[i - 1] = (MYFLT) val;
    }
    if (p->status == CS_RUNNING || p->status == CS_COMPILED
        || p->status == CS_PAUSED) {
      p->result = csoundScoreEvent(cs, 'f', pFields, argc - 1);
      resp = Tcl_GetObjResult(interp);
      Tcl_SetIntObj(resp, p->result);
    }
    return (TCL_OK);
}

/* csTableList arglist
   starts a new i-statement according to arglist (single Tcl list)
*/
int csTableList(ClientData clientData, Tcl_Interp * interp,
                  int argc, char **argv)
{
    char  **largv;
    int     largc;
    MYFLT   pFields[256];
    int     i;
    char    res[10];
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (argc == 2) {
      Tcl_SplitList(interp, argv[1], &largc, (CONST84 char ***) &largv);
      for (i = 0; i < largc; i++)
        pFields[i] = (MYFLT) atof(largv[i]);
      if (p->status == CS_RUNNING || p->status == CS_COMPILED
          || p->status == CS_PAUSED) {
        p->result = csoundScoreEvent(cs, 'f', pFields, largc);
        sprintf(res, "%d", p->result);
        Tcl_SetResult(interp, res, TCL_VOLATILE);
      }
      Tcl_Free((char *) largv);
    }

    return (TCL_OK);
}

int csEvent(ClientData clientData, Tcl_Interp * interp,
              int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    MYFLT   pFields[256];
    double  val;
    int     i;
    char    type;

    type = *(Tcl_GetStringFromObj(argv[1], NULL));
    for (i = 2; i < argc; i++) {
      Tcl_GetDoubleFromObj(interp, argv[i], &val);
      pFields[i - 2] = (MYFLT) val;
    }
    if (p->status == CS_RUNNING || p->status == CS_COMPILED
        || p->status == CS_PAUSED) {
      p->result = csoundScoreEvent(cs, type, pFields, argc - 2);
      resp = Tcl_GetObjResult(interp);
      Tcl_SetIntObj(resp, p->result);
    }
    return (TCL_OK);
}

int csEventList(ClientData clientData, Tcl_Interp * interp,
                  int argc, char **argv)
{
    char  **largv;
    int     largc;
    MYFLT   pFields[256];
    int     i;
    char    res[4], type;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (argc == 2) {
      Tcl_SplitList(interp, argv[1], &largc, (CONST84 char ***) &largv);
      type = *largv[1];
      for (i = 1; i < largc; i++)
        pFields[i - 1] = (MYFLT) atof(largv[i]);
      if (p->status == CS_RUNNING || p->status == CS_COMPILED
          || p->status == CS_PAUSED) {
        p->result = csoundScoreEvent(cs, type, pFields, largc - 1);
        sprintf(res, "%d", p->result);
        Tcl_SetResult(interp, res, TCL_VOLATILE);
      }
      Tcl_Free((char *) largv);
    }

    return (TCL_OK);
}

/* csRewind
   rewinds the score
*/
int csRewind(ClientData clientData, Tcl_Interp * interp,
               int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (p->status == CS_PAUSED || p->status == CS_COMPILED
        || p->status == CS_RUNNING)
      csoundRewindScore(cs);

    return (TCL_OK);
}

/* csOffset secs
   moves the score performance to start at the position
   at secs seconds
*/
int csOffset(ClientData clientData, Tcl_Interp * interp,
               int argc, Tcl_Obj ** argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    double  val;

    if (argc == 2 &&
        (p->status == CS_PAUSED || p->status == CS_COMPILED
         || p->status == CS_RUNNING)) {
      Tcl_GetDoubleFromObj(interp, argv[1], &val);
      csoundSetScoreOffsetSeconds(cs, (MYFLT) val);
    }
    return (TCL_OK);
}

int csGetScoreTime(ClientData clientData, Tcl_Interp * interp,
                     int argc, Tcl_Obj ** argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    Tcl_Obj *resp;

    resp = Tcl_GetObjResult(interp);
    Tcl_SetDoubleObj(resp, (double) csoundGetScoreTime(cs));
    return (TCL_OK);
}

int csGetOffset(ClientData clientData, Tcl_Interp * interp,
                  int argc, Tcl_Obj ** argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    Tcl_Obj *resp;

    resp = Tcl_GetObjResult(interp);
    Tcl_SetDoubleObj(resp, (double) csoundGetScoreOffsetSeconds(cs));
    return (TCL_OK);
}

int csOpcodedir(ClientData clientData, Tcl_Interp * interp,
                  int argc, char **argv)
{
    if (argc >= 2) {
#ifndef MSVC
      setenv("OPCODEDIR", argv[1], 1);
#else
      SetEnvironmentVariable("OPCODEDIR", argv[1]);
#endif
      Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
    }
    return (TCL_OK);
}

int csSetenv(ClientData clientData, Tcl_Interp * interp,
               int argc, char **argv)
{
    if (argc >= 3) {
#ifndef MSVC
      setenv(argv[1], argv[2], 1);
#else
      SetEnvironmentVariable(argv[1], argv[2]);
#endif
      Tcl_SetResult(interp, argv[2], TCL_VOLATILE);
    }
    return (TCL_OK);
}

/* channel interface */

int FindChannel(csdata * p, char *name)
{
    ctlchn *chan = p->inchan;

    while (chan != NULL) {
      if (!strcmp(chan->name, name))
        return IN_CHAN;
      chan = chan->next;
    }
    chan = p->outchan;
    while (chan != NULL) {
      if (!strcmp(chan->name, name))
        return OUT_CHAN;
      chan = chan->next;
    }
    return CHAN_NOT_FOUND;
}

int SetChannelValue(ctlchn * chan, char *name, MYFLT val)
{

    while (chan != NULL) {
      if (!strcmp(chan->name, name)) {
        chan->value = (double) val;
        return CHAN_FOUND;
      }
      chan = chan->next;
    }
    return CHAN_NOT_FOUND;
}

int GetChannelValue(ctlchn * chan, char *name, MYFLT * val)
{

    while (chan != NULL) {
      if (!strcmp(chan->name, name)) {
        *val = (MYFLT) chan->value;
        return CHAN_FOUND;
      }
      chan = chan->next;
    }
    *val = (MYFLT) 0.0;
    return CHAN_NOT_FOUND;
}

void FreeChannels(csdata * p)
{
    ctlchn *chan = p->inchan, *tmp;

    while (chan != NULL) {
      tmp = chan;
      chan = chan->next;
      free(tmp->name);
      free(tmp);
    }
    chan = p->outchan;
    while (chan != NULL) {
      tmp = chan;
      chan = chan->next;
      free(tmp->name);
      free(tmp);
    }
}

/*
csInChannel channel
Register new input channel for use with invalue opcode
*/
int csInChannel(ClientData clientData, Tcl_Interp * interp,
                  int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    ctlchn *newch, *tmp;

    if (argc >= 2) {
      if (FindChannel(p, argv[1]) != IN_CHAN) {
        newch = (ctlchn *) malloc(sizeof(ctlchn));
        tmp = p->inchan;
        p->inchan = newch;
        p->inchan->next = tmp;
        p->inchan->name = (char *) malloc(strlen(argv[1]));
        strcpy(p->inchan->name, argv[1]);
        p->inchan->value = 0.0;
        Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
      }
      else
        Tcl_SetResult(interp, "0", TCL_VOLATILE);
    }
    return (TCL_OK);
}

/*
csOutChannel channel
Register new output channel for use with outvalue opcode
*/
int csOutChannel(ClientData clientData, Tcl_Interp * interp,
                   int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    ctlchn *newch, *tmp;

    if (argc >= 2) {
      if (FindChannel(p, argv[1]) != OUT_CHAN) {
        newch = (ctlchn *) malloc(sizeof(ctlchn));
        tmp = p->outchan;
        p->outchan = newch;
        p->outchan->next = tmp;
        p->outchan->name = (char *)malloc(strlen(argv[1]));
        strcpy(p->outchan->name, argv[1]);
        p->outchan->value = 0.0;
        Tcl_LinkVar(interp, p->outchan->name, (char *) &p->outchan->value,
                    TCL_LINK_DOUBLE | TCL_LINK_READ_ONLY);
        Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
      }
      else
        Tcl_SetResult(interp, "0", TCL_VOLATILE);
    }
    return (TCL_OK);
}

/*
csInValue channel value
sets the value of a control channel
[to be used with invalue opcode]
*/
int csInValue(ClientData clientData, Tcl_Interp * interp,
                int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    double  val;

    if (argc == 3) {
      Tcl_GetDoubleFromObj(interp, argv[2], &val);
      resp = Tcl_GetObjResult(interp);
      if (SetChannelValue
          (p->inchan, Tcl_GetStringFromObj(argv[1], NULL), (MYFLT) val)
          != CHAN_NOT_FOUND)
        Tcl_SetObjResult(interp, argv[1]);
      else
        Tcl_SetStringObj(resp, "channel not found", -1);
    }
    return (TCL_OK);
}

/*
csOutValue channel
returns the value of a control channel
[to be used with outvalue opcode]
*/
int csOutValue(ClientData clientData, Tcl_Interp * interp,
                 int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    MYFLT   val;

    if (argc == 2) {
      resp = Tcl_GetObjResult(interp);
      if (GetChannelValue(p->inchan, Tcl_GetStringFromObj(argv[1], NULL), &val)
          != CHAN_NOT_FOUND) {
        Tcl_SetDoubleObj(resp, (double) val);
      }
      else
        Tcl_SetStringObj(resp, "channel not found", -1);
    }
    return (TCL_OK);
}

void in_channel_value_callback(CSOUND * csound, const char *name, MYFLT * val)
{
    csdata *p = (csdata *) csoundGetHostData(csound);

    GetChannelValue(p->inchan, (char *) name, val);
}

void out_channel_value_callback(CSOUND * csound, const char *name, MYFLT val)
{
    csdata *p = (csdata *) csoundGetHostData(csound);
    MYFLT   oldval;

    if (GetChannelValue(p->outchan, (char *) name, &oldval) == CHAN_FOUND) {
      SetChannelValue(p->outchan, (char *) name, val);
      if (oldval != val)
        Tcl_UpdateLinkedVar(p->interp, name);
    }
}

int csSetTable(ClientData clientData, Tcl_Interp * interp,
                 int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    int     ndx, ftn, size;
    double  value;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    resp = Tcl_GetObjResult(interp);
    if (argc >= 4) {
      Tcl_GetIntFromObj(interp, argv[1], &ftn);
      Tcl_GetIntFromObj(interp, argv[2], &ndx);
      Tcl_GetDoubleFromObj(interp, argv[3], &value);
      size = csoundTableLength(cs, ftn);
      if (ndx >= 0 && ndx <= size) {
        csoundTableSet(cs, ftn, ndx, (MYFLT) value);
        Tcl_SetDoubleObj(resp, value);
      }
      else if (size < 0)
        Tcl_SetStringObj(resp, "table not found", -1);
      else if (ndx > size)
        Tcl_SetStringObj(resp, "out-of-range index", -1);
    }
    else
      Tcl_SetStringObj(resp, "unsufficient parameters", -1);

    return TCL_OK;
}

int csGetTable(ClientData clientData, Tcl_Interp * interp,
                 int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    int     ndx, ftn, size;
    double  value;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    resp = Tcl_GetObjResult(interp);
    if (argc >= 3) {
      Tcl_GetIntFromObj(interp, argv[1], &ftn);
      Tcl_GetIntFromObj(interp, argv[2], &ndx);
      size = csoundTableLength(cs, ftn);
      if (ndx >= 0 && ndx <= size) {
        value = (double) csoundTableGet(cs, ftn, ndx);
        Tcl_SetDoubleObj(resp, value);
      }
      else if (ndx < 0 || ndx > size)
        Tcl_SetDoubleObj(resp, 0.0);
    }
    else
      Tcl_SetDoubleObj(resp, 0.0);

    return TCL_OK;
}

int csGetTableSize(ClientData clientData, Tcl_Interp * interp,
                     int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    int     ftn;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    if (argc >= 2) {
      Tcl_GetIntFromObj(interp, argv[1], &ftn);
      resp = Tcl_GetObjResult(interp);
      Tcl_SetIntObj(resp, csoundTableLength(cs, ftn));
    }
    return TCL_OK;
}

int csStatus(ClientData clientData, Tcl_Interp * interp,
               int argc, char **argv)
{
    char    res[20];
    csdata *p = (csdata *) clientData;

    switch (p->status) {
    case CS_READY:
      strcpy(res, "CS_READY");
      break;
    case CS_COMPILED:
      strcpy(res, "CS_COMPILED");
      break;
    case CS_PAUSED:
      strcpy(res, "CS_PAUSED");
      break;
    case CS_RUNNING:
      strcpy(res, "CS_RUNNING");
      break;
    default:
      strcpy(res, "CS_UNDEFINED");
    }
    Tcl_SetResult(interp, res, TCL_VOLATILE);
    return (TCL_OK);
}

/* EXIT function */
void QuitCsTcl(ClientData clientData)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;

    while (p->status == CS_RUNNING || p->status == CS_PAUSED) {
      p->status = -1;
      Tcl_Sleep(1000);
    }
    FreeChannels(p);
    csoundDestroy(cs);
    free(clientData);
    printf("Ta-ra, me duck!!\n");
}

/* bus channels */
int
csSetControlChannel(ClientData clientData, Tcl_Interp * interp,
                      int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    int     result;
    double  val;
    MYFLT  *pvalue;

    if (argc >= 3) {
      resp = Tcl_GetObjResult(interp);
      result =
          csoundGetChannelPtr(cs, &pvalue, Tcl_GetStringFromObj(argv[1], NULL),
                              CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL);
      if (!result) {
        Tcl_GetDoubleFromObj(interp, argv[2], &val);
        *pvalue = val;
        Tcl_SetObjResult(interp, argv[1]);
      }
      else if (result == CSOUND_ERROR)
        Tcl_SetStringObj(resp, "invalid channel or value", -1);
      else if (result == CSOUND_MEMORY)
        Tcl_SetStringObj(resp, "not enough memory", -1);
    }
    return (TCL_OK);
}

int
csGetControlChannel(ClientData clientData, Tcl_Interp * interp,
                      int argc, Tcl_Obj ** argv)
{
    Tcl_Obj *resp;
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    int     result;
    MYFLT  *pvalue;

    if (argc >= 2) {
      resp = Tcl_GetObjResult(interp);
      result =
          csoundGetChannelPtr(cs, &pvalue, Tcl_GetStringFromObj(argv[1], NULL),
                              CSOUND_OUTPUT_CHANNEL | CSOUND_CONTROL_CHANNEL);
      if (!result) {
        Tcl_SetDoubleObj(resp, (double) *pvalue);
      }
      else
        Tcl_SetDoubleObj(resp, 0);
    }
    return (TCL_OK);
}

int
csSetStringChannel(ClientData clientData, Tcl_Interp * interp,
                     int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    int     result;
    MYFLT  *pvalue;

    if (argc >= 3) {
      result = csoundGetChannelPtr(cs, &pvalue, argv[1],
                                   CSOUND_INPUT_CHANNEL |
                                   CSOUND_STRING_CHANNEL);
      if (!result) {
        if (strlen(argv[2]) < csoundGetStrVarMaxLen(cs)) {
          strcpy((char *) pvalue, argv[2]);
          Tcl_SetResult(interp, argv[2], TCL_VOLATILE);
        }
        else
          Tcl_SetResult(interp, "string is too long", TCL_VOLATILE);
      }
      else if (result == CSOUND_ERROR)
        Tcl_SetResult(interp, "invalid channel or value", TCL_VOLATILE);
      else if (result == CSOUND_MEMORY)
        Tcl_SetResult(interp, "not enough memory", TCL_VOLATILE);
    }
    else
      Tcl_SetResult(interp, "argc not 3", TCL_VOLATILE);
    return (TCL_OK);
}

int
csGetStringChannel(ClientData clientData, Tcl_Interp * interp,
                     int argc, char **argv)
{
    csdata *p = (csdata *) clientData;
    CSOUND *cs = p->instance;
    int     result;
    MYFLT  *pvalue;

    if (argc >= 2) {
      result = csoundGetChannelPtr(cs, &pvalue, argv[1],
                                   CSOUND_OUTPUT_CHANNEL |
                                   CSOUND_STRING_CHANNEL);
      if (!result)
        Tcl_SetResult(interp, (char *) pvalue, TCL_VOLATILE);
    }
    return (TCL_OK);
}

/* initialize Tcl Tk commands */

int tclcsound_initialise(Tcl_Interp * interp)
{
    csdata *pdata = (csdata *) malloc(sizeof(csdata));

    csoundInitialize(NULL, NULL, 0);
    pdata->instance = csoundCreate(pdata);
    pdata->status = CS_READY;
    pdata->result = 0;
    pdata->inchan = NULL;
    pdata->outchan = NULL;
    pdata->interp = interp;
    csoundSetInputValueCallback(pdata->instance, in_channel_value_callback);
    csoundSetOutputValueCallback(pdata->instance, out_channel_value_callback);

    Tcl_CreateCommand(interp, "csCompile", (Tcl_CmdProc *) csCompile,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csCompileList", (Tcl_CmdProc *) csCompileList,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csPlay", (Tcl_CmdProc *) csPlay,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csPerform", (Tcl_CmdProc *) csPerform,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csPerformKsmps", (Tcl_CmdProc *) csPerformKsmps,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csPerformBuffer",
                      (Tcl_CmdProc *) csPerformBuffer, (ClientData) pdata,
                      NULL);
    Tcl_CreateCommand(interp, "csPause", (Tcl_CmdProc *) csPause,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csStop", (Tcl_CmdProc *) csStop,
                      (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csNote", (Tcl_ObjCmdProc *) csNote,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csEvent", (Tcl_ObjCmdProc *) csEvent,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csTable", (Tcl_ObjCmdProc *) csTable,
                         (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csNoteList", (Tcl_CmdProc *) csNoteList,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csTableList", (Tcl_CmdProc *) csTableList,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csEventList", (Tcl_CmdProc *) csEventList,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csRewind", (Tcl_CmdProc *) csRewind,
                      (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csOffset", (Tcl_ObjCmdProc *) csOffset,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csGetOffset", (Tcl_ObjCmdProc *) csGetOffset,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csGetScoreTime",
                         (Tcl_ObjCmdProc *) csGetScoreTime, (ClientData) pdata,
                         NULL);
    Tcl_CreateCommand(interp, "csStatus", (Tcl_CmdProc *) csStatus,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csOutChannel", (Tcl_CmdProc *) csOutChannel,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csInChannel", (Tcl_CmdProc *) csInChannel,
                      (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csOutValue", (Tcl_ObjCmdProc *) csOutValue,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csInValue", (Tcl_ObjCmdProc *) csInValue,
                         (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csOpcodedir", (Tcl_CmdProc *) csOpcodedir,
                      (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csSetenv", (Tcl_CmdProc *) csSetenv,
                      (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csSetTable", (Tcl_ObjCmdProc *) csSetTable,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csGetTable", (Tcl_ObjCmdProc *) csGetTable,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csGetTableSize",
                         (Tcl_ObjCmdProc *) csGetTableSize, (ClientData) pdata,
                         NULL);
    Tcl_CreateObjCommand(interp, "csGetControlChannel",
                         (Tcl_ObjCmdProc *) csGetControlChannel,
                         (ClientData) pdata, NULL);
    Tcl_CreateObjCommand(interp, "csSetControlChannel",
                         (Tcl_ObjCmdProc *) csSetControlChannel,
                         (ClientData) pdata, NULL);
    Tcl_CreateCommand(interp, "csGetStringChannel",
                      (Tcl_CmdProc *) csGetStringChannel, (ClientData) pdata,
                      NULL);
    Tcl_CreateCommand(interp, "csSetStringChannel",
                      (Tcl_CmdProc *) csSetStringChannel, (ClientData) pdata,
                      NULL);
    Tcl_CreateExitHandler(QuitCsTcl, (ClientData) pdata);

    return TCL_OK;
}

