#include "vst4cs.h"
#include "vsthost.h"
#include <stdlib.h>
#include <math.h>
#include <vector>

#ifdef MAKEDLL
#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
#define PUBLIC
#define DIR_SEP '/'
#endif

const static MYFLT SCALING_FACTOR = FL(32767.0);

extern "C"
{
	std::vector<VSTPlugin *> vstPlugins;
	char vst4csver []= "0.1alpha";
	void path_convert(char* in);
	
	int vstinit (void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		MYFLT *verbose;
		char vstplugname[0x100];
		float sr = csound->GetSr(csound);
		float kr = csound->GetKr(csound);	
		if (vstPlugins.size() == 0) {
           csound->Message(csound, "=======================================================\n");
           csound->Message(csound, "vst4cs version %s by Andres Cabrera and Michael Gogins\n", vst4csver);
           csound->Message(csound, "Using code from Hermann Seib and the vst~ object in pd\n");
           csound->Message(csound, "VST is a trademark of Steinberg Media Technologies GmbH\n");
           csound->Message(csound, "VST Plug-In Technology by Steinberg\n");
           csound->Message(csound, "=======================================================\n");
		}
		verbose = p->iverbose;
		if (*p->iplugin == SSTRCOD) {
			strcpy(vstplugname, p->STRARG);          
		}
		else 
           csound->Message(csound,"Invalid plugin name.\n");
		path_convert (vstplugname);
		VSTPlugin *plugin = new VSTPlugin(csound);
		if (plugin->Instance(vstplugname)) {
			csound->Message(csound, "Error loading effect.\n");
			return NOTOK;
		}
		plugin->Init(sr, kr);
		if (*verbose) 
            plugin->Info();
        *p->iVSThandle = vstPlugins.size();
        vstPlugins.push_back(plugin);
		return OK;
	}
	
	int vstinit_free(void*data)
	{
		return OK;
	}
	
	int vstinfo (void *data)  //This opcode will probably disappear
	{
		VSTINFO *p = (VSTINFO *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) p->iVSThandle];
		plugin->Info();
		return OK;
	}
	
	int vstplug_init (void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		csound->Message(csound, "vstplug_init.\n");
		VSTPlugin *plugin = vstPlugins[(size_t) p->iVSThandle];
  		p->framesPerBlock = csound->GetKsmps(csound);
  		p->channels = csound->GetNchnls(csound);
		return OK;
	}
	
	int vstplug (void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
		// Only transmit audio for final active instance.
        if(p->h.insdshead->nxtact) {
            return OK;
        }
  		//ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		//csound->Message(csound, "vstplug: plugin %x.\n", plugin);
		for(size_t i = 0; i < p->framesPerBlock; i++) {
          plugin->inputs[0][i] = p->ain1[i] / SCALING_FACTOR;
          plugin->inputs[1][i] = p->ain2[i] / SCALING_FACTOR;
          plugin->outputs[0][i] = 0;
          plugin->outputs[1][i] = 0;
        }		
		plugin->process(plugin->inputs, plugin->outputs, p->framesPerBlock);
		for(size_t i = 0; i < p->framesPerBlock; i++) {
          p->aout1[i] = plugin->outputs[0][i] * SCALING_FACTOR;
          p->aout2[i] = plugin->outputs[1][i] * SCALING_FACTOR;
        }		
		return OK;
	}
	
	int vstnote_init (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		csound->Message(csound, "vstnote_init.\n");
		VSTPlugin *vstPlugin = vstPlugins[(size_t) *p->iVSThandle];
		p->framesRemaining = *p->kdur * vstPlugin->sample_rate;
		vstPlugin->AddNoteOn(*p->kchan, *p->knote, *p->kveloc);
		return OK;
	}
		
	int vstnote (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
        if(p->framesRemaining >= 0) {
            ENVIRON *csound = p->h.insdshead->csound;
            p->framesRemaining -= (size_t) csound->GetKsmps(csound);
            if(p->framesRemaining <= 0) {
                csound->Message(csound, "vstnote.\n");
                VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
                plugin->AddNoteOff(*p->kchan, *p->knote);
            }
        }
 		return OK;		
	}
	
	int outvst_init(void *data)
	{
		OUTVST_ *p = (OUTVST_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		csound->Message(csound, "outvst_init.\n");
		p->oldkstatus = 0;
		p->oldkchan = 0;
		p->oldkvalue = 0;
		return OK;
	}
	
	int outvst (void *data)  //pensar en agregar un parametro 'trigger' o solo enviar cuando cambia
	{
		OUTVST_ *p = (OUTVST_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		MYFLT *iVSThandle = p->iVSThandle;
		MYFLT *kstatus=p->kstatus;
		MYFLT *kchan=p->kchan;
		MYFLT *kdata1=p->kdata1;
		MYFLT *kdata2=p->kdata2;
		if(round(p->oldkstatus) == round(*kstatus) &&
		   round(p->oldkchan) == round(*kchan) &&
		   round(p->oldkvalue) == round(*kdata1))
		   return OK;
  		csound->Message(csound, "outvst.\n");
    	p->oldkstatus = *kstatus;
    	p->oldkchan = *kchan;
     	p->oldkvalue = *kdata1;		
		switch (int(*kstatus))
		{
		case 144:  //noteon
			{
				plugin->AddNoteOn((int)(*kchan), *kdata1, *kdata2);
			}
			break;
		case 128:   //noteoff
			{
				plugin->AddNoteOff((int)(*kchan), *kdata1);
			}
		
		
			break;
		/*case 160: //poly aftertouch
			{
				plugin->AddPolyAftertouch((int)*kdata1, (int)*kdata2, (int)*kchan);
			}
		
		*/
			break;
		case 176:  //control change
			{
				plugin->AddControlChange((int)*kdata1, (int)*kdata2);
			}
		
			break;
		case 192:   //program change
			{
				plugin->AddProgramChange(int(*kdata1));
			}
		
			break;
		case 208: //aftertouch
			{
				plugin->AddAftertouch(int(*kdata1));
			}
		
			break;
		case 224:   //pitch bend
			{
				plugin->AddPitchBend(int(*kdata1));
			}
		
			break;
		default:
			break;
		}
		return OK;
	}
	
	int vstpret_init (void *data)
	{
  		return OK; 
	}
		
	int vstpret(void *data)
	{
		VSTPRET *p = (VSTPRET *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		csound->Message(csound, "vstpret(%d).\n", int(*p->kparam));
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		*p->kvalue = plugin->GetParamValue(int(*p->kparam));
		if ((*p->kvalue) == -1)
			csound->Message(csound, "Invalid parameter number %d.\n", int(*p->kparam));
		return OK;	
	}	
	
	int vstpsend_init (void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		csound->Message(csound, "vstpsend_init.\n");
		p->oldkparam = 0;
		p->oldkvalue = 0;
  		return OK; 
	}
	
	int vstpsend(void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		csound->Message(csound, "vstpsend.\n");
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		if(*p->kparam == p->oldkparam &&
           *p->kvalue == p->oldkvalue)
            return OK;
        p->oldkparam = *p->kparam;
        p->oldkvalue = *p->kvalue;
		plugin->SetParameter(int(*p->kparam), float (*p->kvalue));
		return OK;	
	}
	
	void path_convert(char* in)
	{
		char inpath[strlen(in)];
		char outpath[64];
		int i= 0;
		int j = 0;
		strcpy(inpath, in);
		for (i= 0; i<= 63; i++) {
				if (inpath[i]) {
					if (inpath[i]==47) {
						outpath[j]= 92;
					}
					else
						outpath[j]= inpath[i];
				}
				else
					outpath[j] = 0;
			j++;
		}
		in = outpath;
	}

    OENTRY vstOentry[] = { 
        {"vstinit",  sizeof(VSTINIT),  1, "i",  "So",    &vstinit,       0, 0, &vstinit_free },
		{"vstinfo",  sizeof(VSTINFO),  1, "",   "i",     &vstinfo,       0, 0, 0},
		{"vstplug",  sizeof(VSTPLUG_), 5, "mm", "iaa",   &vstplug_init,  0, &vstplug, 0},
		{"vstnote",  sizeof(VSTNOTE),  3, "",   "ikkkk", &vstnote_init,  &vstnote, 0, 0},
		{"vstout",   sizeof(OUTVST_),  3, "",   "ikkkk", &outvst_init,   &outvst, 0, 0},
  		{"vstpret",  sizeof(VSTPRET),  3, "k",  "ik",    &vstpret_init,  &vstpret, 0, 0 },
  		{"vstpsend", sizeof(VSTPSEND), 3, "","  ikk",    &vstpsend_init, &vstpsend, 0, 0 }

    };
   
    /**
    * Called by Csound to obtain the size of
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC int opcode_size()
    {
        return sizeof(OENTRY)*7;
    }

    /**
    * Called by Csound to obtain a pointer to
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC OENTRY *opcode_init(ENVIRON *csound)
    {
        return vstOentry;
    }
};
