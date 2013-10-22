/*
    csound~ : A MaxMSP external interface for the Csound API.
    
    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.
    
    LICENSE AGREEMENT
    
    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "includes.h"
#include "definitions.h"

#ifndef _CSOUND_TILDE_H
#define _CSOUND_TILDE_H

using namespace std;
namespace dvx { class CsoundObject; }

typedef struct _csound
{
	t_pxobject l_obj;
	t_object*  m_obj;         // Pointer to this (cast as t_object*).

	dvx::CsoundObject * cso; // Pointer to CsoundObject partner (think of it as an extension to t_csound).

	int sr;					  // MSP sample rate.
	float one_div_sr;		  // 1 / current MSP sample rate.
	int vectorSize;			  // The current MSP vector size.
	int numPerformArgs;		  // Used to keep track of # of args sent to our csound_perform() method.
	void *compiled_bang_outlet; // When a csd or orc/sco is successfully compiled, a bang is sent out this outlet.
	void *done_bang_outlet;   // Whenever a Csound performances finishes, a bang is sent out this outlet.
	void *midi_outlet;		  // An outlet for MIDI data output from Csound.
	void *message_outlet;	  // An outlet for Csound outvalue lists.
	float **in, **out;		  // Array of pointers to MSP input/output vectors.
   	double **in64, **out64;		  // Array of pointers to MSP input/output vectors.
	void *outputClock;		  // When set, calls csound_outputClockCallback().
	void *msgClock;           // When set, calls csound_msgClockCallback().
	t_atom atomList[4];		  // Pre-allocated atomList[] for sending rsidx messages.

	// Max5 Attributes:

	t_symbol *args;           // The arguments to a "csound" command.
	int autostart;            // If true, compile Csound upon csound~ instance creation.
	int bypass;			      // If true, then copy audio input to output without performing Csound.
	int evenlyDivisible;	  // True if ksmps evenly divides Max vector size.
	int input;                // If true, control messages will be processed.
	int matchMaxSR;           // If true, Csound orchestra will be auto-recompiled to match Max sr.
	int outputClockInterval;     // The ms interval for outputClock.
	int messageOutputEnabled; // If true, then Csound messages will be output to the Max window.
	int output;               // If true, output control messages will be sent.
	int numInSignals;         // The # of input vectors for our external.
	int numOutSignals;	      // The # of output vectors for our external.
	int numInOutSignals;	  // The # of input/output vectors for our external. May not be consistent with numInSignals and numOutSignals.
	int outputOverdrive;      // If true, then output values from Csound will be output immediately
							  // (rather than at CSOUND_CLOCK_INTERVAL ms intervals).
	int scale;                // Not used. This is here so that we can add the @scale attribute (to prevent error messages).
	
} t_csound;

t_int *csound_perform(t_int *w);
void csound_dsp(t_csound *x, t_signal **sp, short *count);
void csound_dsp64(t_csound *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void csound_int(t_csound *x, long n);
void csound_float(t_csound *x, double f);
void csound_control(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void csound_midi(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void csound_parse(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void csound_path(t_csound *x, t_symbol *s);
void csound_event(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void *csound_new(t_symbol *s, short argc, t_atom *argv);
void csound_free(t_csound *x);
void csound_assist(t_csound *x, void *b, long m, long a, char *s);
void csound_dblclick(t_csound *x); // Call csound_open().
void csound_open(t_csound *x); // Open the current csd or orc/sco file(s).
void csound_csound(t_csound *x, t_symbol *s, short ac, t_atom *av); // Assemble csound argument list.
void csound_start(t_csound *x);
void csound_startDeferred(t_csound *x);
void csound_stop(t_csound *x);
void csound_stopDeferred(t_csound *x);
void csound_recordstart(t_csound *x);
void csound_recordstop(t_csound *x);
void csound_playstart(t_csound *x);
void csound_playstop(t_csound *x);
void csound_reset(t_csound *x);
void csound_restartclocks(t_csound *x);
void csound_rewind(t_csound *x);
void csound_rewindDeferred(t_csound *x);
void csound_sendPerfDoneBang(t_csound *x, t_symbol *s, long argc, t_atom *argv);
void csound_outputClockCallback(t_csound *x);
void csound_msgClockCallback(t_csound *x);
void csound_message(t_csound *x, long n);

void csound_sfdir(t_csound *x, t_symbol *s);
void SetEnvironment(t_csound *x, const char *path);

// Write a recorded sequence to file.
void csound_write(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void csound_writeDeferred(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Read a recorded sequence from file.
void csound_read(t_csound *x, t_symbol *s, short argc, t_atom *argv);
void csound_readDeferred(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Change playback tempo of recorded sequence.
void csound_tempo(t_csound *x, double f);

// Load an audio file into a Csound table.
void csound_loadsamp(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Read a single sample from a Csound table.
void csound_rsidx(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Write a single sample into a Csound table.
void csound_wsidx(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Read a Csound table into an MSP [buffer~].
void csound_readbuf(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Load an MSP [buffer~] into a Csound table.
void csound_writebuf(t_csound *x, t_symbol *s, short argc, t_atom *argv);

// Run a command line command.
void csound_run(t_csound *x, t_symbol *s, short argc, t_atom *argv);

t_max_err csound_i_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_o_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_io_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_autostart_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_bypass_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_input_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_output_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_overdrive_set(t_csound *x, void *attr, long ac, t_atom *av);

t_max_err csound_scale_set(t_csound *x, void *attr, long ac, t_atom *av);

#endif // _CSOUND_TILDE_H