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

  tclcs.c: Tcl/Tk csound-aware interpreter

*/

#include <stdio.h>
#include <string.h>
#include <csound.h>
#include <envvar.h>
#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include <tk.h>

/* Csound performance status

CS_READY: ready for compilation, effectively stopped
CS_COMPILED: ready for performance, but not yet running
CS_RUNNING: running, producing audio
CS_PAUSED: paused, but ready for performance

Csound will switch from CS_RUNNING to CS_COMPILED once
the loaded score has finished playing.
*/

enum {CS_READY=0, CS_COMPILED, CS_RUNNING, CS_PAUSED} 
cs_status;

enum {CHAN_NOT_FOUND=0,CHAN_FOUND,IN_CHAN,OUT_CHAN}
cs_ctlchns;

typedef struct __ctlchn {
   char *name;
   double value;
   struct __ctlchn *next;
} ctlchn;

typedef struct __csdata {
    CSOUND *instance; /* csound object */
	int result;       /* action result */
	void *threadID;   /* processing thread ID */
	int status;      /* perf status */
	ctlchn *inchan;
	ctlchn *outchan;
	Tcl_Interp *interp;
	
} csdata;

/* performance thread */
uintptr_t  
csThread(void *clientData) {
	
	int result=0, *status;
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	status = &p->status;
	if(*status == CS_COMPILED) {
		*status = CS_RUNNING;
		if (csoundGetOutputBufferSize(cs)
			<= (csoundGetKsmps(cs) * csoundGetNchnls(cs))) {
			while(result == 0 && *status != -1)
				   if(*status != CS_PAUSED) 
					  result = csoundPerformKsmps(cs);
				   else result = 0;
		}
		else {
			while(result == 0 && *status != -1)
				   if(*status != CS_PAUSED) 
					  result = csoundPerformBuffer(cs);
				  else result = 0;
		}
		*status = CS_COMPILED;
		p->result = result;
	}
	return 0;
}

/* TCL commands  */

/* csCompile <argument list> 
   compiles the given orc/score with given options 
*/
int csCompile(ClientData clientData, Tcl_Interp* interp,
			  int argc, char **argv) {
	char res[4];
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	if(p->status == CS_READY){
		p->result = csoundCompile(cs,argc,argv);
		if(!p->result) p->status = CS_COMPILED;
		else csoundReset(cs);
		sprintf(res, "%d", p->result); 
		Tcl_SetResult(interp, res, TCL_VOLATILE);
	}
	return(TCL_OK);
}

/* csPlay
   starts performance
*/
int csPlay(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	
	char res[4];
	csdata *p = (csdata *)clientData;
	if(p->status == CS_COMPILED){
		p->threadID = csoundCreateThread((uintptr_t (*)(void *))csThread, (void *)p);
		sprintf(res, "%d", p->result); 
		Tcl_SetResult(interp, res, TCL_VOLATILE);
	} else if(p->status == CS_PAUSED){
	p->status = CS_RUNNING;
	sprintf(res, "%d", 0); 
	Tcl_SetResult(interp, res, TCL_VOLATILE);
	}
	return(TCL_OK);
}

/* csPause
   toggles on/off performance
*/
int csPause(ClientData clientData, Tcl_Interp* interp,
			int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	if(p->status == CS_PAUSED) p->status = CS_RUNNING;
	else if(p->status == CS_RUNNING) p->status = CS_PAUSED;
	return (TCL_OK);
}

/* csStop
   stops performance and puts csound ready for new compilation
*/
int csStop(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	if(p->status == CS_RUNNING || 
	   p->status == CS_PAUSED) {
	   p->status = -1;
	   Tcl_Sleep(1000);
	   }
	csoundReset(p->instance);
	p->status = CS_READY;
	return (TCL_OK);
}

/* csNote
   starts a new i-statement
*/
int csNote(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	float pFields[256];
	int i;
	char res[4];
	for(i=1; i < argc; i++) pFields[i-1] = (float) atof(argv[i]);
	if(p->status == CS_RUNNING || p->status == CS_COMPILED 
	  || p->status == CS_PAUSED) {
	   p->result = csoundScoreEvent(cs,'i',pFields,argc-1);
	   sprintf(res, "%d", p->result); 
	   Tcl_SetResult(interp, res, TCL_VOLATILE);
	   }
	return (TCL_OK);
}

/* csTable
   starts a new f-statement
*/
int csTable(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	float pFields[256];
	int i;
	char res[4];
	for(i=1; i < argc; i++) pFields[i-1] = (float) atof(argv[i]);
	if(p->status == CS_RUNNING || p->status == CS_COMPILED 
	  || p->status == CS_PAUSED) {
	   p->result = csoundScoreEvent(cs,'f',pFields,argc-1);
	   sprintf(res, "%d", p->result); 
	   Tcl_SetResult(interp, res, TCL_VOLATILE);
	   }
	return (TCL_OK);
}

/* csRewind
   rewinds the score
*/
int csRewind(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	if(p->status == CS_PAUSED || p->status == CS_COMPILED
	   || p->status == CS_RUNNING) 
		 csoundRewindScore(cs);
	return (TCL_OK);
}

/* csOffset secs
   moves the score performance to start at the position 
   at secs seconds
*/
int csOffset(ClientData clientData, Tcl_Interp* interp,
		   int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	if(argc == 2 && 
	    (p->status == CS_PAUSED || p->status == CS_COMPILED
		 || p->status == CS_RUNNING)) 
	     csoundSetScoreOffsetSeconds(cs, (float) atof(argv[1]));
	return (TCL_OK);
}


int csOpcodedir(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv) {
	if(argc >= 2) {
	setenv("OPCODEDIR", argv[1], 1);
	Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
	}
	return (TCL_OK);
}

/* channel interface */

int FindChannel(csdata *p, char *name){
ctlchn *chan = p->inchan;
while(chan != NULL) {
 if(!strcmp(chan->name, name)) return IN_CHAN;
 chan = chan->next;
 }
chan = p->outchan;
while(chan != NULL) {
 if(!strcmp(chan->name, name)) return OUT_CHAN;
 chan = chan->next;
 }
return CHAN_NOT_FOUND;
}

int SetChannelValue(ctlchn *chan, char *name, MYFLT val){

while(chan != NULL) {
 if(!strcmp(chan->name, name)) {
 chan->value = (double) val;
 return CHAN_FOUND;
 }
 chan = chan->next;
 }
 return CHAN_NOT_FOUND;
}

int GetChannelValue(ctlchn *chan, char *name, MYFLT *val){

while(chan != NULL) {
 if(!strcmp(chan->name, name)) {
 *val = (MYFLT) chan->value;
 return CHAN_FOUND;
 }
 chan = chan->next;
 }
 *val = (MYFLT)0.0;
 return CHAN_NOT_FOUND;
}

void FreeChannels(csdata *p){
ctlchn *chan = p->inchan, *tmp;
while(chan != NULL) {
 tmp = chan;
 chan = chan->next;
 free(tmp->name);
 free(tmp);
 }
chan = p->outchan;
while(chan != NULL) {
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
int csInChannel(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	ctlchn *newch, *tmp;
	if(argc >= 2) {
	if(FindChannel(p, argv[1]) != IN_CHAN){
	newch = (ctlchn *) malloc(sizeof(ctlchn));
	tmp = p->inchan;
	p->inchan = newch;
	p->inchan->next = tmp;
	p->inchan->name = strdup(argv[1]);
	p->inchan->value = 0.0;
	Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
	}
	else Tcl_SetResult(interp, "0", TCL_VOLATILE);
	}
	return (TCL_OK);
}

/*
csOutChannel channel
Register new output channel for use with outvalue opcode
*/
int csOutChannel(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv) {
	csdata *p = (csdata *)clientData;
	ctlchn *newch, *tmp;
	if(argc >= 2) {
	if(FindChannel(p, argv[1]) != OUT_CHAN){
	newch = (ctlchn *) malloc(sizeof(ctlchn));
	tmp = p->outchan;
	p->outchan = newch;
	p->outchan->next = tmp;
	p->outchan->name = strdup(argv[1]);
	p->outchan->value = 0.0;
	Tcl_LinkVar(interp,p->outchan->name, (char *)&p->outchan->value, 
	TCL_LINK_DOUBLE | TCL_LINK_READ_ONLY);
	Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
	}
	else Tcl_SetResult(interp, "0", TCL_VOLATILE);
	}
	return (TCL_OK);
}

/*
csInValue channel value
sets the value of a control channel
[to be used with invalue opcode]
*/
int csInValue(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv){
	csdata *p = (csdata *)clientData;
	MYFLT val;
	if(argc == 3){
	val = (MYFLT) atof(argv[2]);
	if(SetChannelValue(p->inchan, argv[1], val) 
	  != CHAN_NOT_FOUND)
		Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
	 else
		Tcl_SetResult(interp, "channel not found", TCL_VOLATILE);
	}
	return (TCL_OK);			
}

/*
csOutValue channel
returns the value of a control channel
[to be used with outvalue opcode]
*/
int csOutValue(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv){
				
	csdata *p = (csdata *)clientData;
	char res[10];
	MYFLT val;
	if(argc == 2){
	if(GetChannelValue(p->inchan, argv[1], &val) 
	  != CHAN_NOT_FOUND){
	    sprintf(res, "%f", val);
		Tcl_SetResult(interp, argv[1], TCL_VOLATILE);
		}
	 else
		Tcl_SetResult(interp, "channel not found", TCL_VOLATILE);
	}
	return (TCL_OK);			
}

void in_channel_value_callback(CSOUND *csound,
                                      const char *name, MYFLT *val)
{
  csdata *p = (csdata *) csoundGetHostData(csound);
  GetChannelValue(p->inchan, (char*) name, val);
}

void out_channel_value_callback(CSOUND *csound,
                                       const char *name, MYFLT val)
{
  csdata *p = (csdata *) csoundGetHostData(csound);
  if(SetChannelValue(p->outchan, (char*) name, val)==CHAN_FOUND){
   Tcl_UpdateLinkedVar(p->interp, name);
   }
}


int csStatus(ClientData clientData, Tcl_Interp* interp,
				int argc, char **argv) {
	char res[10];
	csdata *p = (csdata *)clientData;
	switch (p->status){
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
void QuitCsTcl(ClientData clientData) {
	csdata *p = (csdata *)clientData;
	CSOUND *cs = p->instance;
	while(p->status == CS_RUNNING || 
		  p->status == CS_PAUSED){ 
		p->status = -1;
		Tcl_Sleep(1000);
	}
	FreeChannels(p);
	csoundDestroy(cs);
	free(clientData);
	printf("\n Bye-Bye !! \n");
}


/* initialize Tcl Tk Interpreter */
int Tcl_ApInit(Tcl_Interp* interp) {
	
	int status;
	csdata *pdata;
	
	status = Tcl_Init(interp);
	if (status != TCL_OK) {
		return TCL_ERROR;
	}
#ifdef TCLSH
	printf("\ncstclsh\n(c) Victor Lazzarini\n Music Technology Lab \n NUI Maynooth, 2005 \n");
#endif
#ifdef WISH
	/* Initialize Tk values. */
	status = Tk_Init(interp);
	if (status != TCL_OK) {
		return TCL_ERROR; 
	}
    printf("\ncswish\n(c) Victor Lazzarini\n Music Technology Lab \n NUI Maynooth, 2005 \n");
#endif

	pdata = (csdata *) malloc(sizeof(csdata));
	csoundInitialize(NULL,NULL,0);
	pdata->instance = csoundCreate(pdata);  
	pdata->status = CS_READY;
	pdata->result = 0;
	pdata->inchan = NULL;
	pdata->outchan = NULL;
	pdata->interp = interp;
	csoundSetInputValueCallback(pdata->instance,in_channel_value_callback);
    csoundSetOutputValueCallback(pdata->instance,out_channel_value_callback);
	
	Tcl_CreateCommand(interp, "csCompile", (Tcl_CmdProc *)csCompile, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csPlay", (Tcl_CmdProc *)csPlay, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csPause", (Tcl_CmdProc *)csPause, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csStop", (Tcl_CmdProc *)csStop, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csNote", (Tcl_CmdProc *)csNote, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csTable", (Tcl_CmdProc *)csTable, (ClientData) pdata, NULL);	
	Tcl_CreateCommand(interp, "csRewind", (Tcl_CmdProc *)csRewind, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csOffset", (Tcl_CmdProc *)csOffset, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csStatus", (Tcl_CmdProc *)csStatus, (ClientData) pdata, NULL);
	Tcl_CreateCommand(interp, "csOutChannel", (Tcl_CmdProc *)csOutChannel, (ClientData) pdata, NULL);	
	Tcl_CreateCommand(interp, "csInChannel", (Tcl_CmdProc *)csInChannel, (ClientData) pdata, NULL);	
	Tcl_CreateCommand(interp, "csOutValue", (Tcl_CmdProc *)csOutValue, (ClientData) pdata, NULL);	
	Tcl_CreateCommand(interp, "csInValue", (Tcl_CmdProc *)csInValue, (ClientData) pdata, NULL);	
	Tcl_CreateCommand(interp, "csOpcodedir", (Tcl_CmdProc *)csOpcodedir, (ClientData) pdata, NULL);
	Tcl_CreateExitHandler(QuitCsTcl, (ClientData) pdata);
	
	return TCL_OK;
}


int main(int argc, char *argv[])
{	
#ifdef TCLSH
	Tcl_Main(argc, argv, Tcl_ApInit);
#endif
#ifdef WISH  
	Tk_Main(argc, argv, Tcl_ApInit);
#endif
    return 0;
}








