#include "vst4cs.h"
#include "vsthost.h"
#include <stdlib.h>

#ifdef MAKEDLL
#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
#define PUBLIC
#define DIR_SEP '/'
#endif

#define CS(p) (p->h.insdshead->csound)
#define MAX_VST_PLUGS 10
#define SCALING_FACTOR 32767

extern "C"
{
	VSTPlugin *pEffect[MAX_VST_PLUGS];
	char vst4csver []= "0.1alpha";
	void path_convert(char* in);
	
	int vstinit (void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		MYFLT *localhandle;
		MYFLT *verbose;
		char vstplugname[64];
		float sr = CS(p)->GetSr(CS(p));
		float kr = CS(p)->GetKr(CS(p));	
		if (vsthandle == -1) {
           CS(p)->Message(CS(p), "vst4cs version %s by Andres Cabrera and Michael Gogins\n", vst4csver);
           CS(p)->Message(CS(p), "Using code from Hermann Seib and the vst~ object in pd\n");
           CS(p)->Message(CS(p), "VST is a trademark of Steinberg Media Technologies GmbH\n");
           CS(p)->Message(CS(p), "VST Plug-In Technology by Steinberg\n");
		}
		localhandle = p->iVSThandle;
		verbose = p->iverbose;
		if (*p->iplugin == SSTRCOD)                    /* if strg name given */
		{
			strcpy(vstplugname, p->STRARG);          /*   use that         */
			//CS(p)->Message(CS(p), "Parameter: %s\n",vstplugname);
		}
		/*else if ((long)*p->iplugin <= strsmax && strsets != NULL &&
             strsets[(long)*p->iplugin])
		strcpy(pvfilnam, strsets[(long)*p->ifilno]);*/
		else CS(p)->Message(CS(p),"Invalid plugin name.\n");
		path_convert (vstplugname);
		//CS(p)->Message(CS(p), "Parameter : %s\n",vststring);
		vsthandle = vsthandle + 1;
		if (vsthandle >= MAX_VST_PLUGS) {
			CS(p)->Message(CS(p), 
                    "Cannot create more than %i VST plugins.\n", MAX_VST_PLUGS);
			*localhandle = -1;
			return OK;
		}
		else 
          *localhandle = vsthandle;
		pEffect[vsthandle] = new VSTPlugin(csound);
		if (pEffect[vsthandle]->Instance(vstplugname)) {
			CS(p)->Message(CS(p), "Error loading effect.\n");
			return NOTOK;
		}
		pEffect[vsthandle]->Init(sr, kr);
		if (*verbose) 
            pEffect[vsthandle]->Info();
		return OK;
	}
	
	int vstinit_free(void*data)
	{
		delete [] pEffect;
		return OK;
	}
	
	int vstinfo (void *data)  //This opcode will probably disappear
	{
		VSTINFO *p = (VSTINFO *)data;
		size_t localhandle = (size_t) p->iVSThandle;
		pEffect[localhandle]->Info();
		return OK;
	}
	
	int vstplug_init (void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
		VSTPlugin *plugin = pEffect[(size_t) p->iVSThandle];
  		p->framesPerBlock = CS(p)->GetKsmps(CS(p));
  		p->channels = CS(p)->GetNchnls(CS(p));
		return OK;
	}
	
	int vstplug (void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
		// Only transmit audio for final active instance.
        if(p->h.insdshead->nxtact) {
            return OK;
        }
		VSTPlugin *plugin = pEffect[(size_t) *p->iVSThandle];
  		//ENVIRON *csound = p->h.insdshead->csound;
		//csound->Message(csound, "vstplug: Plugin handle %d.\n", iVSThandle);
		//csound->Message(csound, "vstplug: Plugin %x.\n", plugin);
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
	
	int vstinst_init (void *data)
	{
	/* TODO (manta#1#): Cuando este listo vstplug copiarlo aqui modificado o quitarlo y poner los parametros de entrada opcionales*/
		return OK;
	}
	
	int vstinst (void *data)
	{
		return OK;
	}
	
	int vstnote_init (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
        ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *vstPlugin = pEffect[(size_t) *p->iVSThandle];
		p->framesRemaining = *p->kdur * vstPlugin->sample_rate;
		vstPlugin->AddNoteOn(*p->knote, *p->kveloc, *p->kchan);
		return OK;
	}
		
	int vstnote (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
        if(p->framesRemaining >= 0) {
            ENVIRON *csound = p->h.insdshead->csound;
            p->framesRemaining -= (size_t) csound->GetKsmps(csound);
            if(p->framesRemaining <= 0) {
                VSTPlugin *vstPlugin = pEffect[(size_t) *p->iVSThandle];
                vstPlugin->AddNoteOff(*p->knote, *p->kchan);
            }
        }
 		return OK;		
	}
	
	int outvst_init(void *data)
	{
		OUTVST_ *p = (OUTVST_ *)data;
		p->oldkstatus = 0;
		p->oldkchan = 0;
		p->oldkvalue = 0;
		return OK;
	}
	
	int outvst (void *data)  //pensar en agregar un parametro 'trigger' o solo enviar cuando cambia
	{
		OUTVST_ *p = (OUTVST_ *)data;
		MYFLT *iVSThandle = p->iVSThandle;
		MYFLT *kstatus=p->kstatus;
		MYFLT *kchan=p->kchan;
		MYFLT *kdata1=p->kdata1;
		MYFLT *kdata2=p->kdata2;
		if(p->oldkstatus == *kstatus &&
		   p->oldkchan == *kchan &&
		   p->oldkvalue == *kdata1)
		   return OK;
    	p->oldkstatus = *kstatus;
    	p->oldkchan = *kchan;
     	p->oldkvalue = *kdata1;		
		switch (int(*kstatus))
		{
		case 144:  //noteon
			{
				pEffect[int(*iVSThandle)]->AddNoteOn((int)(*kdata1), (int)(*kdata2), (int)(*kchan));
			}
			break;
		case 128:   //noteoff
			{
				pEffect[int(*iVSThandle)]->AddNoteOff((int)(*kdata1), (int)(*kchan));
			}
		
		
			break;
		/*case 160: //poly aftertouch
			{
				pEffect[int(*iVSThandle)]->AddPolyAftertouch((int)*kdata1, (int)*kdata2, (int)*kchan);
			}
		
		*/
			break;
		case 176:  //control change
			{
				pEffect[int(*iVSThandle)]->AddControlChange((int)*kdata1, (int)*kdata2);
			}
		
			break;
		case 192:   //program change
			{
				pEffect[int(*iVSThandle)]->AddProgramChange(int(*kdata1));
			}
		
			break;
		case 208: //aftertouch
			{
				pEffect[int(*iVSThandle)]->AddAftertouch(int(*kdata1));
			}
		
			break;
		case 224:   //pitch bend
			{
				pEffect[int(*iVSThandle)]->AddPitchBend(int(*kdata1));
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
		MYFLT *iVSThandle = p->iVSThandle;
		*p->kvalue = pEffect[int(*iVSThandle)]->GetParamValue(int(*p->kparam));
		if ( (*p->kvalue) == -1)
			CS(p)->Message(CS(p), "Invalid parameter number %d.\n", int(*p->kparam));
		return OK;	
	}	
	
	int vstpsend_init (void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		p->oldkparam = 0;
		p->oldkvalue = 0;
  		return OK; 
	}
	
	int vstpsend(void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		MYFLT *iVSThandle = p->iVSThandle;
		if(*p->kparam == p->oldkparam &&
           *p->kvalue == p->oldkvalue)
            return OK;
        p->oldkparam = *p->kparam;
        p->oldkvalue = *p->kvalue;
		pEffect[int(*iVSThandle)]->SetParameter(int(*p->kparam), float (*p->kvalue));
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
        {   "vstinit", sizeof (VSTINIT), 1, "i", "So", &vstinit, 0, 0, &vstinit_free },
		{	"vstinfo", sizeof (VSTINFO), 1, "", "i", &vstinfo, 0, 0, 0},
		{	"vstplug", sizeof (VSTPLUG_), 5, "mm", "iaa", &vstplug_init, 0, &vstplug, 0},
		/* TODO (#1#): Cambiar por parametros opcionales */
		{	"vstnote", sizeof (VSTNOTE), 3, "", "ikkkk", &vstnote_init, &vstnote, 0, 0},
		{	"vstout", sizeof (OUTVST_), 3, "", "ikkkk", &outvst_init, &outvst, 0, 0},
  		{	"vstpret",sizeof(VSTPRET),3,"k","ik", &vstpret_init, &vstpret, 0, 0 },
  		{	"vstpsend",sizeof(VSTPSEND),3,"","ikk", &vstpsend_init, &vstpsend, 0, 0 }

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
