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

#include "csound~.h"
#include "channel.h"
#include "csound.h"
#include "CsoundObject.h"
#include "CsoundTable.h"
#include "memory.h"
#include "midi.h"
#include "sequencer.h"
#include "util.h"

// a macro to mark exported symbols in the code without requiring an external file to define them
#ifdef WIN_VERSION
	// note that this the required syntax on windows regardless of whether the compiler is msvc or gcc
	#define T_EXPORT __declspec(dllexport)
#else // MAC_VERSION
	// the mac uses the standard gcc syntax, you should also set the -fvisibility=hidden flag to hide the non-marked symbols
	#define T_EXPORT __attribute__((visibility("default")))
#endif

t_class *csound_class;
int MaxChannelStringLength = 64; // Should init to something (probable limit is 256).

using namespace dvx;

int T_EXPORT main(void)
{
	t_class *c;

	c = class_new("csound~", (method)csound_new, (method)csound_free, (long)sizeof(t_csound), 0L, A_GIMME, 0);

	class_addmethod(c, (method)csound_assist,       "assist", A_CANT, 0);
	class_addmethod(c, (method)csound_start,        "bang", 0);
	class_addmethod(c, (method)csound_control,      "c", A_GIMME, 0);
	class_addmethod(c, (method)csound_csound,       "csound", A_GIMME, 0);
	class_addmethod(c, (method)csound_control,      "control", A_GIMME, 0);
	class_addmethod(c, (method)csound_dblclick,     "dblclick", A_CANT, 0);
	class_addmethod(c, (method)csound_dsp,          "dsp", A_CANT, 0);
   	class_addmethod(c, (method)csound_dsp64,          "dsp64", A_CANT, 0);
	class_addmethod(c, (method)csound_event,        "e", A_GIMME, 0);
	class_addmethod(c, (method)csound_event,        "event", A_GIMME, 0);
	class_addmethod(c, (method)csound_float,        "float", A_FLOAT, 0);
	class_addmethod(c, (method)csound_int,          "int", A_LONG, 0);
	class_addmethod(c, (method)csound_loadsamp,     "loadsamp", A_GIMME, 0);
	class_addmethod(c, (method)csound_midi,         "m", A_GIMME, 0);
	class_addmethod(c, (method)csound_midi,         "midi", A_GIMME, 0);
	class_addmethod(c, (method)csound_open,         "open", 0);
	class_addmethod(c, (method)csound_parse,        "parse", A_GIMME, 0);
	class_addmethod(c, (method)csound_path,         "path", A_SYM, 0);
	class_addmethod(c, (method)csound_playstart,    "playstart", 0);
	class_addmethod(c, (method)csound_playstop,     "playstop", 0);
	class_addmethod(c, (method)csound_message,      "printout", A_LONG, 0);
	class_addmethod(c, (method)csound_read,         "read", A_GIMME, 0);
	class_addmethod(c, (method)csound_readbuf,      "readbuf", A_GIMME, 0);
	class_addmethod(c, (method)csound_recordstart,  "recordstart", 0);
	class_addmethod(c, (method)csound_recordstop,   "recordstop", 0);
	class_addmethod(c, (method)csound_reset,        "reset", 0);
	class_addmethod(c, (method)csound_restartclocks,"restartclocks", 0);
	class_addmethod(c, (method)csound_rewind,       "rewind", 0);
	class_addmethod(c, (method)csound_rsidx,        "rsidx", A_GIMME, 0);
	class_addmethod(c, (method)csound_run,          "run", A_GIMME, 0);
	class_addmethod(c, (method)csound_sfdir,        "sfdir", A_SYM, 0);
	class_addmethod(c, (method)csound_start,        "start", 0);
	class_addmethod(c, (method)csound_stop,         "stop", 0);
	class_addmethod(c, (method)csound_tempo,        "tempo", A_FLOAT, 0);
	class_addmethod(c, (method)csound_write,        "write", A_GIMME, 0);
	class_addmethod(c, (method)csound_writebuf,     "writebuf", A_GIMME, 0);
	class_addmethod(c, (method)csound_wsidx,        "wsidx", A_GIMME, 0);
	
	CLASS_ATTR_SYM(c, "args", 0, t_csound, args);
	CLASS_ATTR_SAVE(c, "args", 0);
	CLASS_ATTR_STYLE_LABEL(c, "args", 0, "text", "\"csound\" Arguments");
	
	CLASS_ATTR_LONG(c, "autostart", 0, t_csound, autostart);
	CLASS_ATTR_SAVE(c, "autostart", 0);
	CLASS_ATTR_FILTER_CLIP(c, "autostart", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "autostart", 0, "onoff", "Auto Compile/Start Csound");
	CLASS_ATTR_ACCESSORS(c, "autostart", NULL, (method)csound_autostart_set);

	CLASS_ATTR_LONG(c, "bypass", 0, t_csound, bypass);
	CLASS_ATTR_SAVE(c, "bypass", 0);
	CLASS_ATTR_FILTER_CLIP(c, "bypass", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "bypass", 0, "onoff", "Disable Audio/MIDI Processing");
	CLASS_ATTR_ACCESSORS(c, "bypass", NULL, (method)csound_bypass_set);

	CLASS_ATTR_LONG(c, "i", 0, t_csound, numInSignals);
	CLASS_ATTR_FILTER_CLIP(c, "i", 1, 128);
	CLASS_ATTR_INVISIBLE(c, "i", 0);
	CLASS_ATTR_ACCESSORS(c, "i", NULL, (method)csound_i_set);

	CLASS_ATTR_LONG(c, "input", 0, t_csound, input);
	CLASS_ATTR_SAVE(c, "input", 0);
	CLASS_ATTR_FILTER_CLIP(c, "input", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "input", 0, "onoff", "Accept Input Control Messages");
	CLASS_ATTR_ACCESSORS(c, "input", NULL, (method)csound_input_set);

	CLASS_ATTR_LONG(c, "io", 0, t_csound, numInOutSignals);
	CLASS_ATTR_FILTER_CLIP(c, "io", 1, 128);
	CLASS_ATTR_INVISIBLE(c, "io", 0);
	CLASS_ATTR_ACCESSORS(c, "io", NULL, (method)csound_io_set);

	CLASS_ATTR_LONG(c, "interval", 0, t_csound, outputClockInterval);
	CLASS_ATTR_SAVE(c, "interval", 0);
	CLASS_ATTR_FILTER_CLIP(c, "interval", 1, 5000);
	CLASS_ATTR_STYLE_LABEL(c, "interval", 0, "text", "Output Control Message Clock Interval (ms)");
	
	CLASS_ATTR_LONG(c, "matchsr", 0, t_csound, matchMaxSR);
	CLASS_ATTR_SAVE(c, "matchsr", 0);
	CLASS_ATTR_FILTER_CLIP(c, "matchsr", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "matchsr", 0, "onoff", "Auto Recompile to Match Max SR");
	
	CLASS_ATTR_LONG(c, "message", 0, t_csound, messageOutputEnabled);
	CLASS_ATTR_SAVE(c, "message", 0);
	CLASS_ATTR_FILTER_CLIP(c, "message", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "message", 0, "onoff", "Allow Printing to Max Window");

	CLASS_ATTR_LONG(c, "nolatency", ATTR_SET_OPAQUE_USER, t_csound, evenlyDivisible);
	CLASS_ATTR_STYLE_LABEL(c, "nolatency", 0, "onoff", "Process Audio Without Latency");

	CLASS_ATTR_LONG(c, "o", 0, t_csound, numOutSignals);
	CLASS_ATTR_FILTER_CLIP(c, "o", 1, 128);
	CLASS_ATTR_INVISIBLE(c, "o", 0);
	CLASS_ATTR_ACCESSORS(c, "o", NULL, (method)csound_o_set);

	CLASS_ATTR_LONG(c, "output", 0, t_csound, output);
	CLASS_ATTR_SAVE(c, "output", 0);
	CLASS_ATTR_FILTER_CLIP(c, "output", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "output", 0, "onoff", "Allow Ouput Control Messages");
	CLASS_ATTR_ACCESSORS(c, "output", NULL, (method)csound_output_set);
	
	CLASS_ATTR_LONG(c, "overdrive", 0, t_csound, outputOverdrive);
	CLASS_ATTR_SAVE(c, "overdrive", 0);
	CLASS_ATTR_ACCESSORS(c, "overdrive", NULL, (method)csound_overdrive_set);
	CLASS_ATTR_FILTER_CLIP(c, "overdrive", 0, 1);
	CLASS_ATTR_STYLE_LABEL(c, "overdrive", 0, "onoff", "Bypass Output Control Message Clock");

	// This attribute is not used. It is included so that the user does not
	// see error messages concerning @scale.
	CLASS_ATTR_LONG(c, "scale", 0, t_csound, scale);
	CLASS_ATTR_INVISIBLE(c, "scale", 0);
	CLASS_ATTR_ACCESSORS(c, "scale", NULL, (method)csound_scale_set);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	csound_class = c;

	csoundInitialize(NULL);

	return 0;
}

void csound_assist(t_csound *x, void *b, long msg, long arg, char *s)
{
	int i;
	
	if(msg == ASSIST_INLET) 
	{
		switch(arg)
		{
		case 0:  sprintf(s, "(signal) Audio In 0 / (int) MIDI Input"); break;
		default: sprintf(s, "(signal) Audio In %ld", arg); break;
		}
	}
	else if(msg == ASSIST_OUTLET)
	{
		if(arg < x->numOutSignals) sprintf(s, "(signal) Audio Out %ld", arg);
		else
		{
			i = arg - x->numOutSignals;
			switch(i)
			{
			case 0: sprintf(s,"(list) Messages"); break;
			case 1: sprintf(s,"(int) MIDI"); break;
			case 2: sprintf(s,"(bang) Orchestra Compiled Successfuly"); break;
			case 3: sprintf(s,"(bang) Csound Peformance Done"); break;
			}
		}
	}
}

void csound_dblclick(t_csound *x)
{
	csound_open(x);
}

void csound_open(t_csound *x)
{
	CsoundObject *cso = x->cso;

	if(!cso->m_args.ArgListValid()) return;
	if(cso->m_args.CsdInPath())
		openFile(cso->m_obj, cso->m_args.CsdPath().c_str());
	else
	{
		openFile(cso->m_obj, cso->m_args.OrcPath().c_str());
		openFile(cso->m_obj, cso->m_args.ScoPath().c_str());
	}	
}

void csound_message(t_csound *x, long n)
{
	x->messageOutputEnabled = (bool) n;
}

t_max_err csound_overdrive_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		long n = atom_getlong(av);
		
		if(n == 0)
		{
			x->outputOverdrive = false; 
			if(x->output) clock_fdelay(x->outputClock, x->outputClockInterval); // Restart the clock.
		}
		else
		{
			x->outputOverdrive = true;
			clock_unset(x->outputClock); // Stop the clock.
		}
	}
	return MAX_ERR_NONE;
}

t_max_err csound_scale_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	object_warn(x->m_obj, "The @scale attribute is deprecated. Audio is automatically scaled.");
	return MAX_ERR_NONE;
}

t_int *csound_perform(t_int *w)
{
	t_csound *x = (t_csound *)(w[1]);
	int i, chan, vectorSize = x->vectorSize;
	CsoundObject *cso = x->cso;
	
	for(i=0; i<x->numInSignals; i++) x->in[i] = (float *)(w[i+2]);
	for(i=0; i<x->numOutSignals; i++) 
	{
		x->out[i] = (float *)(w[i+2+x->numInSignals]);
		memset(x->out[i], 0, sizeof(t_float) * x->vectorSize);
	}

	if(x->l_obj.z_disabled) 
		return (w+1+x->numPerformArgs);
	
	if(x->bypass)
	{
		// Copy audio input to output.
		chan = (x->numInSignals < x->numOutSignals ? x->numInSignals : x->numOutSignals);
		for(i=0; i<chan; i++) memcpy(x->out[i], x->in[i], sizeof(float) * vectorSize);
		
		// Since we're bypassing the Csound performance, return early.
		// Must return w + 1 + the # of args to perform method (see csound_dsp()).
		return (w+1+x->numPerformArgs);  
	}
	
	cso->Perform();
	
	if(x->outputOverdrive && x->output)
	{
		cso->m_oChanGroup.ProcessDirtyChannels(ChannelGroup::AUDIO_THREAD);
		cso->m_oChanGroup.SendDirtyChannels(x->message_outlet, ChannelGroup::AUDIO_THREAD);
	}
	
	// Must return w + 1 + the # of args to perform method (see csound_dsp()).
	return (w+1+x->numPerformArgs);  
}


void csound_perform64(t_object *_x, t_object *dsp64, double **ins,
                           long numins, double **outs, long numouts, long sampleframes, long flags, void *
                           userparam) {
    
	t_csound *x = (t_csound *)_x;
	int i, chan, vectorSize = x->vectorSize;
	CsoundObject *cso = x->cso;
	x->in64 = ins, x->out64 = outs;
    
//	for(i=0; i<x->numInSignals; i++) x->in[i] = (float *)(w[i+2]);
	for(i=0; i < numouts; i++)
	{
		memset(outs[i], 0, sizeof(t_double) * sampleframes);
	}
    
	if(x->l_obj.z_disabled) return;
	
	if(x->bypass)
	{
		// Copy audio input to output.
		chan = (x->numInSignals < x->numOutSignals ? x->numInSignals : x->numOutSignals);
		for(i=0; i<chan; i++) memcpy(x->out64[i], x->in64[i], sizeof(double) * vectorSize);
		
		// Since we're bypassing the Csound performance, return early.
		// Must return w + 1 + the # of args to perform method (see csound_dsp()).
//		return (w+1+x->numPerformArgs);
	}
	
	cso->Perform64();
	
	if(x->outputOverdrive && x->output)
	{
		cso->m_oChanGroup.ProcessDirtyChannels(ChannelGroup::AUDIO_THREAD);
		cso->m_oChanGroup.SendDirtyChannels(x->message_outlet, ChannelGroup::AUDIO_THREAD);
	}
	
	// Must return w + 1 + the # of args to perform method (see csound_dsp()).
//	return (w+1+x->numPerformArgs);
}


void csound_dsp(t_csound *x, t_signal **sp, short *count)
{
	CsoundObject *cso = x->cso;
	int i=0, totalVectors=0;
	void **perform_args;
	
	totalVectors = x->numInSignals + x->numOutSignals;
	x->numPerformArgs = totalVectors + 1;

	perform_args = (void **) MemoryNew(sizeof(void*) * (totalVectors + 1));
	perform_args[0] = (void *) x;  // first argument is a pointer to the t_csound struct
	for(i=1; i<=totalVectors; i++) perform_args[i] = (void*) sp[i-1]->s_vec;
	
	x->sr = sp[0]->s_sr;  // store current sampling rate
	x->vectorSize = sp[0]->s_n; // store vector size
	x->one_div_sr = 1.0f / (float)x->sr; // store 1 / sr

	dsp_addv(csound_perform, x->numPerformArgs, perform_args);
	MemoryFree(perform_args);

	if(cso->m_compiled && x->sr != cso->m_sr)
	{
		if(!x->matchMaxSR && x->messageOutputEnabled)
			object_error(x->m_obj, "Max sr (%d) != Csound sr (%d)", x->sr, x->cso->m_sr);
		else if(x->matchMaxSR)
		{
			cso->Compile();
		}
	}

	x->evenlyDivisible = (x->vectorSize % x->cso->m_ksmps == 0);
}

void csound_dsp64(t_csound *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	CsoundObject *cso = x->cso;
//	int i=0, totalVectors=0;
    int totalVectors=0;
//	void **perform_args;
	
	totalVectors = x->numInSignals + x->numOutSignals;
	x->numPerformArgs = totalVectors + 1;
    
//	perform_args = (void **) MemoryNew(sizeof(void*) * (totalVectors + 1));
//	perform_args[0] = (void *) x;  // first argument is a pointer to the t_csound struct
//	for(i=1; i<=totalVectors; i++) perform_args[i] = (void*) sp[i-1]->s_vec;
	
//	x->sr = sp[0]->s_sr;  // store current sampling rate
    x->sr = (int)samplerate;  // store current sampling rate
//	x->vectorSize = sp[0]->s_n; // store vector size
    x->vectorSize = maxvectorsize;
	x->one_div_sr = 1.0f / (float)x->sr; // store 1 / sr
   
//	dsp_addv(csound_perform, x->numPerformArgs, perform_args);
	dsp_add64(dsp64, (t_object*)x, (t_perfroutine64)csound_perform64, 0, NULL);
//	MemoryFree(perform_args);
    
	if(cso->m_compiled && x->sr != cso->m_sr)
	{
		if(!x->matchMaxSR && x->messageOutputEnabled)
			object_error(x->m_obj, "Max sr (%d) != Csound sr (%d)", x->sr, x->cso->m_sr);
		else if(x->matchMaxSR)
		{
			cso->Compile();
		}
	}
    
	x->evenlyDivisible = (x->vectorSize % x->cso->m_ksmps == 0);
}
 
void csound_int(t_csound *x, long n)
{
	if(x->bypass || !x->cso->m_compiled || x->cso->m_performanceFinished) return;
	x->cso->m_midiBuffer.Enqueue((byte) n);
}

void csound_float(t_csound *x, double f){}

void csound_control(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	Sequencer &seq = cso->m_sequencer;
	int type;
	MYFLT f_val = FL(0.0);
	char *s_val = NULL;
	char *name = NULL;

	if(!x->input) return;

	if(argc != 2 || argv[0].a_type != A_SYM) 
	{
		char buf[128];
		PrintAtoms(s, argc, argv, buf, 128);
		object_error(x->m_obj, "Incorrect control message format: %s", buf);
		return;	
	}

	name = argv[0].a_w.w_sym->s_name;
	switch(argv[1].a_type)
	{
	case A_FLOAT: f_val = (MYFLT) atom_getfloat(&argv[1]); break;
	case A_LONG:  f_val = (MYFLT) atom_getlong(&argv[1]); break;
	case A_SYM:   s_val = argv[1].a_w.w_sym->s_name; break;
	}

	if(NULL == s_val) 
	{
		type = CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL;
		{
			ScopedLock k(cso->m_lock);
			
			if(cso->m_compiled && !cso->m_performanceFinished)
				cso->m_iChanGroup.SetValAndSync(name, type, f_val);
			else
				cso->m_iChanGroup.SetVal(name, type, f_val);
		}
		if(seq.Recording())
			seq.AddControlEvent(name, f_val, true);
	}
	else 
	{
		type = CSOUND_INPUT_CHANNEL | CSOUND_STRING_CHANNEL;
		{	
			ScopedLock k(cso->m_lock); 

			if(cso->m_compiled && !cso->m_performanceFinished)
				cso->m_iChanGroup.SetValAndSync(name, type, s_val);
			else
				cso->m_iChanGroup.SetVal(name, type, s_val);
		}
		if(seq.Recording())
			seq.AddStringEvent(name, s_val, true);
	}
}

void csound_midi(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	int i;
	byte buffer[MAX_MIDI_MESSAGE_SIZE];
	
	if(x->bypass || sys_getdspstate()) return;
	if(argc == 0 || argc > MAX_MIDI_MESSAGE_SIZE) return;
	for(i=0; i<argc; i++)
	{
		switch(argv[i].a_type)
		{
		case A_LONG:
			buffer[i] = (byte) atom_getlong(&argv[i]);
			break;
		default: 
			object_error(x->m_obj, "Only integers are allowed in midi messages.");
			return;
		}
	}
	x->cso->m_midiBuffer.EnqueueBuffer(buffer, i);
}

void csound_parse(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	bool tosub = false;

	for(int i=0; i<argc; i++)
	{
		if(argv[i].a_type == A_SYM && strcmp("tosub", argv[i].a_w.w_sym->s_name) == 0)
			tosub = true;
	}

	x->cso->m_scripter.ParseCSD(x->cso->m_args.CsdPath().c_str(), tosub);
}

void csound_path(t_csound *x, t_symbol *s)
{
	char path[MAX_STRING_LENGTH];
	
	if(strlen(s->s_name) >= MAX_STRING_LENGTH)
	{
		object_error(x->m_obj, "Pathname is too long; maximum string length is %d.", MAX_STRING_LENGTH);
		return;
	}
	
	strncpy(path, s->s_name, MAX_STRING_LENGTH-1);
	if(isQuoted(path)) removeQuotes(path);
	convertMaxPathToPosixPath(path, path, MAX_STRING_LENGTH);

#ifdef _WINDOWS
	if(-1 != _chdir(path))
#elif MACOSX
	if(-1 != chdir(path))
#endif
	{
		object_post(x->m_obj, "Current Directory set to: %s", path);
		x->cso->m_path = path;
	}
	else
		object_error(x->m_obj, "%s not found, or you don't have exec permission on folder or an ancestor folder.", 
			 path);
}

void csound_event(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int i, totalSize = 0;
	char buffer[MAX_EVENT_MESSAGE_SIZE], tmp[MAX_STRING_LENGTH];
	
	if(argc == 0 || argv[0].a_type != A_SYM) return;
	
	sprintf(buffer, "%s", argv[0].a_w.w_sym->s_name);
	totalSize += strlen(buffer);

	for(i=1; i<argc; i++)
	{
		switch(argv[i].a_type)
		{
		case A_LONG:
			sprintf(tmp, " %lld", (long long)atom_getlong(&argv[i]));
			totalSize += strlen(tmp);
			strncat(buffer, tmp, MAX_EVENT_MESSAGE_SIZE - strlen(buffer) - 1);
			break;
		case A_FLOAT:
			sprintf(tmp, " %f", atom_getfloat(&argv[i]));
			totalSize += strlen(tmp);
			strncat(buffer, tmp, MAX_EVENT_MESSAGE_SIZE - strlen(buffer) - 1);
			break;
		case A_SYM:
			// Add double quotes if a space or slash is present and it's not quoted.
			if(!isQuoted(argv[i].a_w.w_sym->s_name) && 
				(strchr(argv[i].a_w.w_sym->s_name, '/') ||
				 strchr(argv[i].a_w.w_sym->s_name, '\\') ||
				 strchr(argv[i].a_w.w_sym->s_name, ' ')))
			{
				char tmp2[MAX_STRING_LENGTH];
				
				if(isAbsoluteMaxPath(argv[i].a_w.w_sym->s_name))
				{
					convertMaxPathToPosixPath(argv[i].a_w.w_sym->s_name, tmp2, MAX_STRING_LENGTH);
					snprintf(tmp, MAX_STRING_LENGTH-1, " \"%s\"", tmp2); 
				}
				else
					snprintf(tmp, MAX_STRING_LENGTH-1, " \"%s\"", argv[i].a_w.w_sym->s_name); 
			}
			else
				snprintf(tmp, MAX_STRING_LENGTH-1, " %s", argv[i].a_w.w_sym->s_name);
			totalSize += strlen(tmp);
			strncat(buffer, tmp, MAX_EVENT_MESSAGE_SIZE - strlen(buffer) - 1);
			break;
		default: 
			object_error(x->m_obj, "Unrecognized element in event string.");
			return;
		}
	}

	// If the event string is too long, don't pass the truncated string to Csound.
	if(totalSize >= MAX_EVENT_MESSAGE_SIZE)
	{
		object_error(x->m_obj, "Event string size %d too large.  Max size is %d.", 
		     totalSize, MAX_EVENT_MESSAGE_SIZE - 1);
	}
	else if(!cso->m_renderingToFile)
	{
		Sequencer &seq = cso->m_sequencer;

		if(seq.Recording()) seq.AddCsoundEvent(buffer, true);
		ScopedLock k(x->cso->m_lock);
		if(cso->m_compiled && !cso->m_performanceFinished)
			csoundInputMessage(x->cso->m_csound, buffer);
	}
}

void *csound_new(t_symbol *s, short argc, t_atom *argv)
{
	int i=0, numArgCount=0, iarg;
	char tmpStr[MAX_STRING_LENGTH];
	char *str = NULL, *lastAttr = NULL;
	static bool firstTime = true;
    t_csound *x = (t_csound *)object_alloc(csound_class);

	if(NULL == x) return x;

	try
	{
		x->m_obj = (t_object*) x;
		x->args = NULL;
		x->autostart = false;
		x->bypass = false;
		x->input = true;
		x->output = true;
		x->matchMaxSR = true;
		x->outputClockInterval = DEFAULT_CLOCK_INTERVAL;
		x->messageOutputEnabled = true;
		x->numInSignals = DEFAULT_NUM_SIGNALS;
		x->numOutSignals = DEFAULT_NUM_SIGNALS;
		x->numInOutSignals = DEFAULT_NUM_SIGNALS;
		x->evenlyDivisible = false;
		x->outputOverdrive = false;

		x->cso = new CsoundObject(x);                // Must construct CsoundObject before following code!
		std::auto_ptr<CsoundObject> auto_p(x->cso);  // Just in case exception happens before end of try.

		x->outputClock = clock_new(x, (method)csound_outputClockCallback);
		clock_fdelay(x->outputClock, x->outputClockInterval);

		x->msgClock = clock_new(x, (method)csound_msgClockCallback);
		clock_fdelay(x->msgClock, DEFAULT_MESSAGE_CLOCK_INTERVAL);

		// Process "@" csound~ arguments. Must process after creating CsoundObject because @autostart may == 1.
		attr_args_process(x, argc, argv);	
	
		if(firstTime)
		{
			// Post the version number.
			object_post(x->m_obj, "csound~ v1.1.3");
			firstTime = false;
		}
	
		// Initialize variables with the arguments to our object.
		for(i = 0; i < argc; i++)
		{
			switch(argv[i].a_type)
			{
				case A_LONG:
					iarg = atom_getlong(&argv[i]);

					if(lastAttr != NULL)
					{
						// Ignore the current atom (it's an attribute value).
						if(0 == strcmp(lastAttr,"@start"))
						{
							object_warn(x->m_obj, "@start is deprecated. Please use @autostart.");
							x->autostart = true;
						}
						lastAttr = NULL;
					}
					else
					{
						if(numArgCount == 0)
							x->numInSignals = x->numOutSignals = atom_getlong(&argv[i]);
						else if(numArgCount == 1)
							x->numOutSignals = atom_getlong(&argv[i]);
						else
							object_error(x->m_obj, "Too many integer arguments.");
						++numArgCount;
					}
					break;
				case A_FLOAT:
					object_error(x->m_obj, "Float arguments not accepted.");
					lastAttr = NULL;
					break;
				case A_SYM:
					str = argv[i].a_w.w_sym->s_name;

					if(str[0] == '@') 
						lastAttr = str; 
					else 
					if(strstr(str, ".csd") || strstr(str, ".orc") || strstr(str, ".sco") || 
					   strstr(str, ".CSD") || strstr(str, ".ORC") || strstr(str, ".SCO"))
					{
						// The current string contains a csd/orc/sco file.
						// Treat it as an argument list for Csound.
						if(NULL == x->args)
							x->args = gensym(str);
					}
					else
					{
						if(strcmp(str, "noscale") == 0)
							object_warn(x->m_obj, "\"noscale\" is depecrated. Scaling is automatic now.", str);
					}
					break;
			}
		}
	
		// Add signal inlets.				
		dsp_setup((t_pxobject *)x, x->numInSignals);  
		((t_pxobject *)x)->z_misc = Z_NO_INPLACE;
	
		// Add non-signal outlets (right to left).
		x->done_bang_outlet = bangout(x);
		x->compiled_bang_outlet = bangout(x);
		x->midi_outlet = intout(x);
		x->message_outlet = listout(x);
	
		// Add signal outlets.
		for(i=0; i<x->numOutSignals; i++) outlet_new((t_object *)x, "signal"); 
	
		x->sr = sys_getsr();
		x->cso->m_inChans = x->numInSignals;
		x->cso->m_outChans = x->numOutSignals;
		x->in = (float **) MemoryNew(sizeof(float *) * x->numInSignals);
		x->out = (float **) MemoryNew(sizeof(float *) * x->numOutSignals);
	
		x->cso->m_defaultPathID = path_getdefault();
	
		if(0 == path_topotentialname(x->cso->m_defaultPathID, "", tmpStr, false))
		{
		#ifdef _WINDOWS
			x->cso->m_defaultPath = tmpStr;
		#elif MACOSX
			// Copy tmpStr to x->cso->m_defaultPath.  In the process, use the volume name
			// (e.g. "Macintosh HD:") to form an absolute path.   
			string &s = x->cso->m_defaultPath;
			s = "/Volumes/";
			s += tmpStr;
			s.erase(s.find_first_of(':'), 1);
		#endif
			SetEnvironment(x, x->cso->m_defaultPath.c_str());
		}
	
		x->atomList[0].a_type = A_SYM;
		x->atomList[0].a_w.w_sym = gensym("rsidx");
		x->atomList[1].a_type = A_LONG;
		x->atomList[2].a_type = A_LONG;
		x->atomList[3].a_type = A_FLOAT;

		// If the user provided csound argument list with "@args",
		// process the argument list with csound_csound() and start Csound.
		if(x->args)
		{
			t_atom temp_list[MAX_ATOM_LIST_SIZE];
			int size = CreateAtomListFromString(x->m_obj, x->args->s_name, temp_list, MAX_ATOM_LIST_SIZE);
			if(size) 
			{
				csound_csound(x, NULL, size, temp_list);
				if(x->autostart)
					csound_startDeferred(x);
			}
		}

		auto_p.release(); // Everything went OK, don't delete the CsoundObject.
	}
	catch(std::exception & ex)
	{
		object_error(x->m_obj, "%s", ex.what());
	}

    return (x);
}

void csound_free(t_csound *x)
{
	dsp_free(&x->l_obj); // Must call this first to avoid crashes when audio processing is on.
	csound_stopDeferred(x);
	clock_unset(x->outputClock);
	freeobject((t_object *) x->outputClock);
	clock_unset(x->msgClock);
	freeobject((t_object *) x->msgClock);
	MemoryFree(x->in);
	MemoryFree(x->out);
	delete x->cso;
}

void csound_csound(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	x->cso->SetCsoundArguments(argc,argv);
}

void csound_start(t_csound *x)
{ 
	defer_low(x->m_obj, (method)csound_startDeferred, NULL, 0, NULL);
}

void csound_startDeferred(t_csound *x)
{
	x->cso->Compile();
}

void csound_stop(t_csound *x)
{ 
	defer_low(x->m_obj, (method)csound_stopDeferred, NULL, 0, NULL);
}

void csound_stopDeferred(t_csound *x)
{
	x->cso->Stop();
}

void csound_recordstart(t_csound *x)
{
	CsoundObject *cso = x->cso;

	if(!cso->m_renderingToFile && !cso->m_renderThreadExists)
		cso->m_sequencer.StartRecording();
}

void csound_recordstop(t_csound *x)
{
	x->cso->m_sequencer.StopRecording();
}

void csound_playstart(t_csound *x)
{
	CsoundObject *cso = x->cso;

	if(!cso->m_renderingToFile && !cso->m_renderThreadExists)
		cso->m_sequencer.StartPlaying();
}

void csound_playstop(t_csound *x)
{
	x->cso->m_sequencer.StopPlaying();
}

// To reset, just call the csound_start() function.  csound_start() calls
// csoundPreCompile(), which calls csoundReset() automatically.
void csound_reset(t_csound *x)
{
	defer_low(x->m_obj, (method)csound_startDeferred, NULL, 0, NULL);
}

void csound_restartclocks(t_csound *x)
{
	if(x->output && x->outputOverdrive)
		clock_fdelay(x->outputClock, x->outputClockInterval); // Restart output control message clock.

	clock_fdelay(x->msgClock, DEFAULT_MESSAGE_CLOCK_INTERVAL); // Restart output message clock.
}

void csound_rewind(t_csound *x) { defer_low(x->m_obj, (method)csound_rewindDeferred, NULL, 0, NULL); }
void csound_rewindDeferred(t_csound *x)
{
	x->cso->Rewind();
}

void csound_sfdir(t_csound *x, t_symbol *s) { SetEnvironment(x, s->s_name); }
void SetEnvironment(t_csound *x, const char *path)
{
	int result;
	char tmp[MAX_STRING_LENGTH];
	
	if(isAbsoluteMaxPath(path))	
		convertMaxPathToPosixPath(path, tmp, MAX_STRING_LENGTH);
	else
		strncpy(tmp, path, MAX_STRING_LENGTH-1);
	
	if(strlen(tmp))
	{
		result = csoundSetGlobalEnv("SFDIR", tmp);
		if(result != CSOUND_SUCCESS) error("Unable to set SFDIR to %s", tmp);
		result = csoundSetGlobalEnv("SSDIR", tmp);
		if(result != CSOUND_SUCCESS) error("Unable to set SSDIR to %s", tmp);
		result = csoundSetGlobalEnv("SADIR", tmp);
		if(result != CSOUND_SUCCESS) error("Unable to set SADIR to %s", tmp);
		result = csoundSetGlobalEnv("INCDIR", tmp);
		if(result != CSOUND_SUCCESS) error("Unable to set INCDIR to %s", tmp);
		result = csoundSetGlobalEnv("CSSTRNGS", tmp);
		if(result != CSOUND_SUCCESS) error("Unable to set CSSTRNGS to %s", tmp);
	}
}

void csound_sendPerfDoneBang(t_csound *x, t_symbol *s, long argc, t_atom *argv) 
{ 
	outlet_bang(x->done_bang_outlet);
}

void csound_outputClockCallback(t_csound *x)
{	
	if(!x->outputOverdrive && x->output)
	{
		{
			ScopedLock k(x->cso->m_lock); // Prevent Csound from modifying output channels while we access them.
			x->cso->m_oChanGroup.ProcessDirtyChannels(ChannelGroup::CLOCK_THREAD);
		}
		// Send the atom tuples that were buffered by ProcessDirtyChannels() above.
		x->cso->m_oChanGroup.SendDirtyChannels(x->message_outlet, ChannelGroup::CLOCK_THREAD);

		// Reschedule csound_outputClockCallback (i.e. continue the clock).
		clock_fdelay(x->outputClock, x->outputClockInterval);
	}
	else
		clock_unset(x->outputClock); // Stop the clock.
}

void csound_msgClockCallback(t_csound *x)
{
	x->cso->m_msg_buf.post();
	clock_fdelay(x->msgClock, DEFAULT_MESSAGE_CLOCK_INTERVAL);
}

void csound_write(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	defer_low(x->m_obj, (method)csound_writeDeferred, s, argc, argv);
}

void csound_writeDeferred(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	Sequencer &seq = cso->m_sequencer;
	string file, absPath;
	
	if(argc < 1 || argv[0].a_type != A_SYM) return;

	try
	{
		file = argv[0].a_w.w_sym->s_name;
		if(isQuoted(file)) removeQuotes(file);
		if(isAbsoluteMaxPath(file))
		{
			convertMaxPathToPosixPath(file);
			absPath = file;
		}
		else 
			absPath = cso->m_defaultPath + "/" + file;

		if(seq.m_read_write_thread_exists)
		{
			object_error(x->m_obj, "Can't write sequence to file; previous writing thread still working.");
			return;
		}

		Sequencer::ParamObject *spo = new Sequencer::ParamObject(&seq, absPath);
		void* result = NULL;

		result = csoundCreateThread(Sequencer_WriteThreadFunc, (void*)spo);
		if(result == NULL)
			object_error(x->m_obj, "Could not create writing thread.");
    else {
      seq.m_read_write_thread = result;
			seq.m_read_write_thread_exists = true;
    }
	}
	catch(std::exception & ex)
	{
		object_error(x->m_obj, "csound_writeDeferred() : %s", ex.what());
	}
}

void csound_read(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	defer_low(x->m_obj, (method)csound_readDeferred, s, argc, argv);
}

void csound_readDeferred(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	Sequencer &seq = cso->m_sequencer;
	string file, absPath;
	
	if(argc < 1 || argv[0].a_type != A_SYM) return;

	try
	{
		file = argv[0].a_w.w_sym->s_name;
		if(isQuoted(file)) removeQuotes(file);
		/*
		if(isAbsoluteMaxPath(file)) convertMaxPathToPosixPath(file);
		cso->SetCurDir();
		seq.Read(file);
		*/

		if(isAbsoluteMaxPath(file))
		{
			convertMaxPathToPosixPath(file);
			absPath = file;
		}
		else 
			absPath = cso->m_defaultPath + "/" + file;

		if(seq.m_read_write_thread_exists)
		{
			object_error(x->m_obj, "Can't read sequence from file; previous reading/writing thread still working.");
			return;
		}

		Sequencer::ParamObject *spo = new Sequencer::ParamObject(&seq, absPath);


		void *result = csoundCreateThread(Sequencer_ReadThreadFunc, (void*)spo);
		if(result == NULL)
			object_error(x->m_obj, "Could not create reading thread.");
    else {
      seq.m_read_write_thread = result;
			seq.m_read_write_thread_exists = true;
    }
	}
	catch(std::exception & ex)
	{
		object_error(x->m_obj, "csound_readDeferred() : %s", ex.what());
	}
}

void csound_tempo(t_csound *x, double f)
{
	// Don't allow changes to bpm while recording.
	if(!x->cso->m_sequencer.Recording()) 
		x->cso->m_sequencer.SetBPM((float) (f * DEFAULT_BPM));
}

void csound_loadsamp(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int tableNum, channel;
	char filename[MAX_STRING_LENGTH];
	float offsetSeconds = 0.0f;
	float sizeSeconds = 0.0f; 
	
	if(argc < 3 || argv[0].a_type != A_LONG || argv[1].a_type != A_LONG || argv[2].a_type != A_SYM) return;
	tableNum = atom_getlong(&argv[0]);
	channel = atom_getlong(&argv[1]);
	strncpy(filename, argv[2].a_w.w_sym->s_name, MAX_STRING_LENGTH-1);
	
	// If 4'th argument exists, it specifies offset time in seconds.
	if(argc > 3)
	{
		switch(argv[3].a_type) 
		{
		case A_FLOAT: offsetSeconds = (float) atom_getfloat(&argv[3]); break;
		case A_LONG:  offsetSeconds = (float) atom_getlong(&argv[3]); break;
		}
	}
	
	// If 5'th argument exists, it specifies time to read in seconds.
	if(argc > 4)
	{
		switch(argv[4].a_type) 
		{
		case A_FLOAT: sizeSeconds = atom_getfloat(&argv[4]); break;
		case A_LONG:  sizeSeconds = (float) atom_getlong(&argv[4]); break;
		}
	}
	
	ScopedLock k(cso->m_lock);
	
	if(cso->m_compiled && !cso->m_performanceFinished)
	{
		if(isQuoted(filename)) removeQuotes(filename);
		if(isAbsoluteMaxPath(filename)) convertMaxPathToPosixPath(filename, filename, MAX_STRING_LENGTH);
		if(!isAbsolutePath(filename))
		{
			// Set the current directory again (in case it was changed).
			if(cso->m_path.size()) change_directory(x->m_obj, cso->m_path.c_str());
			else if(cso->m_defaultPath.size()) change_directory(x->m_obj, cso->m_defaultPath.c_str());
		}
		
		CsoundTable ct(x->m_obj, x->cso->m_csound);

		// If tableNum < 0, then increase the size of the target table if necessary;
		// otherwise, just fill it up as much as possible.
		ct.LoadAudioFile(filename,abs(tableNum),channel,(tableNum < 0 ? 1 : 0),offsetSeconds,sizeSeconds);
	}
}

void csound_readbuf(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int tableNum, channel;
	float offsetSeconds = 0.0f;
	float sizeSeconds = 0.0f;
	
	if(argc < 3 || argv[0].a_type != A_LONG || argv[1].a_type != A_LONG || argv[2].a_type != A_SYM) return;
	tableNum = atom_getlong(&argv[0]);
	channel = atom_getlong(&argv[1]);
	
	// If 4'th argument exists, it specifies offset time in seconds.
	if(argc > 3)
	{
		switch(argv[3].a_type) 
		{
		case A_FLOAT: offsetSeconds = (float) atom_getfloat(&argv[3]);  break;
		case A_LONG:  offsetSeconds = (float) atom_getlong(&argv[3]);  break;
		}
	}
	
	// If 5'th argument exists, it specifies time to read in seconds.
	if(argc > 4)
	{
		switch(argv[4].a_type) 
		{
		case A_FLOAT: sizeSeconds = (float) atom_getfloat(&argv[4]);  break;
		case A_LONG:  sizeSeconds = (float) atom_getlong(&argv[4]); break;
		}
	}

	ScopedLock(cso->m_lock);

	if(cso->m_compiled && !cso->m_performanceFinished)
	{
		CsoundTable ct(x->m_obj, cso->m_csound);
		ct.ReadBufferTilde(argv[2].a_w.w_sym,abs(tableNum),channel,(tableNum < 0 ? 1 : 0),offsetSeconds,sizeSeconds);
	}
}

void csound_writebuf(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int tableNum, channel;
	float offsetSeconds = 0.0f;
	float sizeSeconds = 0.0f; 
	
	if(argc < 3 || argv[0].a_type != A_LONG || argv[1].a_type != A_LONG || argv[2].a_type != A_SYM) return;
	tableNum = atom_getlong(&argv[0]);
	channel = atom_getlong(&argv[1]);
	
	// If 4'th argument exists, it specifies offset time in seconds.
	if(argc > 3)
	{
		switch(argv[3].a_type) 
		{
		case A_FLOAT: offsetSeconds = (float) atom_getfloat(&argv[3]);  break;
		case A_LONG:  offsetSeconds = (float) atom_getlong(&argv[3]);  break;
		}
	}
	
	// If 5'th argument exists, it specifies time to read in seconds.
	if(argc > 4)
	{
		switch(argv[4].a_type) 
		{
		case A_FLOAT: sizeSeconds = (float) atom_getfloat(&argv[4]);  break;
		case A_LONG:  sizeSeconds = (float) atom_getlong(&argv[4]);  break;
		}
	}

	ScopedLock(cso->m_lock);
	
	if(cso->m_compiled && !cso->m_performanceFinished)
	{
		CsoundTable ct(x->m_obj, cso->m_csound);
		ct.WriteBufferTilde(argv[2].a_w.w_sym,abs(tableNum),channel,offsetSeconds,sizeSeconds);
	}
}

void csound_rsidx(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int tableNum, index, result = -1;
	MYFLT val;
	
	if(argc != 2 || argv[0].a_type != A_LONG || argv[1].a_type != A_LONG) return;
	tableNum = atom_getlong(&argv[0]);
	index = atom_getlong(&argv[1]);
	if(index < 0) return;
	
	{
		ScopedLock k(cso->m_lock);
		if(cso->m_compiled && !cso->m_performanceFinished)
		{
			CsoundTable ct(x->m_obj, cso->m_csound);
			result = ct.Get(tableNum, index, &val);
		}
	}
	
	switch(result)
	{
	case 0:
        atom_setlong(&x->atomList[1], (t_atom_long)tableNum);
        atom_setlong(&x->atomList[2], (t_atom_long)index);
        atom_setfloat(&x->atomList[3], (t_atom_float)val);
		outlet_list(x->message_outlet, 0L, 4, x->atomList);
		break;
	case 1:
		object_error(x->m_obj, "\"rsidx\" failed because table #%d doesn't exist.", tableNum);
		break;
	default:
		break;
	}
}

void csound_wsidx(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int tableNum, index, result = 0;
	float val;
	
	if(argc != 3 || argv[0].a_type != A_LONG || argv[1].a_type != A_LONG || 
	   (argv[2].a_type != A_FLOAT && argv[2].a_type != A_LONG)) return;
	tableNum = atom_getlong(&argv[0]);
	index = atom_getlong(&argv[1]);
	val = (float)(argv[2].a_type == A_FLOAT ? atom_getfloat(&argv[2]) : atom_getlong(&argv[2]));
	if(index < 0) return;
	
	{
		ScopedLock k(cso->m_lock);
		if(cso->m_compiled && !cso->m_performanceFinished)
		{
			CsoundTable ct(x->m_obj, cso->m_csound);
			result = ct.Set(tableNum, index, val);
		}
	}
	
	switch(result)
	{
	case 1:	object_error(x->m_obj, "\"wsidx\" failed because table #%d doesn't exist.", tableNum);
		break;
	case 2:	//object_error(x->m_obj, "\"wsidx\" failed because index %d is out of bounds.", index);
		break;
	default:
		break;
	}
}

#ifdef MACOSX
void csound_run(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int i, result;
	char tmp[MAX_STRING_LENGTH];
	char *args[MAX_ARGS];
	
	if(argc >= MAX_ARGS)
	{
		object_error(x->m_obj, "Too many arguments for \"run\". MAX_ARGS = %d", MAX_ARGS);
		return;
	}

	for(i=0; i<argc; i++)
	{
		switch(argv[i].a_type)
		{
		case A_FLOAT:
			snprintf(tmp, MAX_STRING_LENGTH-1, "%f", (float)atom_getfloat(&argv[i]));
			args[i] = strdup(tmp);
			break;
		case A_LONG:
			snprintf(tmp, MAX_STRING_LENGTH-1, "%lld", (long long)atom_getlong(&argv[i]));
			args[i] = strdup(tmp);
			break;
		case A_SYM:
			strncpy(tmp, argv[i].a_w.w_sym->s_name, MAX_STRING_LENGTH-1);
			if(isQuoted(tmp)) removeQuotes(tmp);
			if(isAbsoluteMaxPath(tmp)) convertMaxPathToPosixPath(tmp, tmp, MAX_STRING_LENGTH);
			args[i] = strdup(tmp);
			break;
		}
	}
	
	args[argc] = NULL;
	
	cso->SetCurDir();

	result = csoundRunCommand((const char * const *)args, 1);
	if(result < 0) object_error(x->m_obj, "'run' command error: %d", result);
	
	// Free the arguments.
	for(i=0; i<argc; i++) free(args[i]);
}
#endif

#ifdef _WINDOWS
void csound_run(t_csound *x, t_symbol *s, short argc, t_atom *argv)
{
	CsoundObject *cso = x->cso;
	int i, result;
	char tmp[MAX_STRING_LENGTH];
	char tmp2[MAX_STRING_LENGTH];
	char command[MAX_STRING_LENGTH];
	char args[MAX_STRING_LENGTH];
	args[0] = '\0';

	for(i=0; i<argc; i++)
	{
		switch(argv[i].a_type)
		{
		case A_FLOAT:
			snprintf(tmp, MAX_STRING_LENGTH-1, "%f", (float)atom_getfloat(&argv[i]));
			break;
		case A_LONG:
			snprintf(tmp, MAX_STRING_LENGTH-1, "%lld", (long long)atom_getlong(&argv[i]));
			break;
		case A_SYM:
			strncpy(tmp, argv[i].a_w.w_sym->s_name, MAX_STRING_LENGTH-1);
			if(hasSpace(tmp)) 
			{
				addQuotes(tmp, tmp2, MAX_STRING_LENGTH);
				strcpy(tmp, tmp2);
			}
			break;
		}

		if(i==0) strcpy(command, tmp);
		else
		{
			if(i>1) strncat(args, " ", MAX_STRING_LENGTH-strlen(args)-1);
			strncat(args, tmp, MAX_STRING_LENGTH-strlen(args)-1);
		}
	}
	
	cso->SetCurDir();
	ShellExecute(NULL, "open", command, args, NULL, SW_SHOWNORMAL);
}
#endif

// t_max_err csound_i_get(t_csound *x, void *attr, long *ac, t_atom **av) {}

t_max_err csound_i_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n > 0 && n < 128)
			x->numInSignals = n;
	}
	return MAX_ERR_NONE;
}

// t_max_err csound_o_get(t_csound *x, void *attr, long *ac, t_atom **av) {}

t_max_err csound_o_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n > 0 && n < 128)
			x->numOutSignals = n;
	}
	return MAX_ERR_NONE;
}

// t_max_err csound_io_get(t_csound *x, void *attr, long *ac, t_atom **av) {}

t_max_err csound_io_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n > 0 && n < 128)
		{
			x->numInSignals = n;
			x->numOutSignals = n;
		}
	}
	return MAX_ERR_NONE;
}

t_max_err csound_autostart_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n != 0)
		{
			x->autostart = true;
			if(!x->cso->m_compiled && x->cso->m_performanceFinished)
				defer_low(x->m_obj, (method)csound_startDeferred, NULL, 0, NULL);
		}
		else
			x->autostart = false;

	}
	return MAX_ERR_NONE;
}

t_max_err csound_bypass_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n != 0)
		{
			x->bypass = true;
			x->cso->m_midiBuffer.Flush();
		}
		else
			x->bypass = false;

	}
	return MAX_ERR_NONE;
}

t_max_err csound_input_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		ScopedLock k(x->cso->m_lock);
		if(n == 0)
			x->input = false;
		else
			x->input = true;
	}
	return MAX_ERR_NONE;
}

t_max_err csound_output_set(t_csound *x, void *attr, long ac, t_atom *av)
{
	if(ac && av)
	{
		long n = atom_getlong(av);
		if(n == 0)
		{
			x->output = false;
			clock_unset(x->outputClock);
		}
		else
		{
			x->output = true;
			if(!x->outputOverdrive) clock_fdelay(x->outputClock, x->outputClockInterval);
		}
	}
	return MAX_ERR_NONE;
}
