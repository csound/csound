#include "vst4cs.h"
#include "vsthost.h"
#include <stdlib.h>

extern "C"
{
	std::vector<VSTPlugin *> pluginInstances;
	char vst4csver []= "0.2alpha";
	void path_convert(char* in);
	MYFLT SCALING_FACTOR = 32767.0;
	
	int vstinit (void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		char vstplugname[0x100];
		if (vsthandle == -1) {
            csound->Message(csound, "\nvst4cs version %s by Andres Cabrera and Michael Gogins,\n", vst4csver);
            csound->Message(csound, "using code from Hermann Seib and the vst~ object in pd.\n");
            csound->Message(csound, "VST is a trademark of Steinberg Media Technologies GmbH\n");
            csound->Message(csound, "VST Plug-In Technology by Steinberg.\n");
        }
		if (*p->iplugin == SSTRCOD) {
			strcpy(vstplugname, p->STRARG);
		}
		else 
            csound->Message(csound, "vstinit: Invalid VST plugin name.\n");
		path_convert (vstplugname);
		VSTPlugin *vstPlugin = new VSTPlugin();
		vsthandle = pluginInstances.size();
		*p->iVSThandle = vsthandle;
		pluginInstances.push_back(vstPlugin);
		csound->Message(csound, "vstinit: Plugin handle %d.\n", (size_t) *p->iVSThandle);
		if (vstPlugin->Instance(csound, vstplugname)) {
			csound->Message(csound, "vstinit: VST plugin could not be loaded.\n");
			return !OK; 
		}
		vstPlugin->Init(csound);
		if (*p->iverbose) 
		    vstPlugin->Info(csound);
		csound->Message(csound, "vstinit: Plugin  %x.\n", vstPlugin);
		return OK;
	}

	int vstinit_free(void* data)
	{
        for(size_t i = 0, n = pluginInstances.size(); i < n; ++i) {
            delete pluginInstances[i];
        }
        pluginInstances.clear();
		return OK;
	}

	int vstinfo (void *data)  //This opcode will probably disappear
	{
		VSTINFO *p = (VSTINFO *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		size_t iVSThandle = (size_t) *p->iVSThandle;
		pluginInstances[iVSThandle]->Info(csound);
		return OK;
	}
	
	int vstplug_init (void *data)
	{
		VST_PLUGIN *p = (VST_PLUGIN *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		p->nsamps = (long) csound->GetKsmps(csound);
		size_t iVSThandle = (size_t) *p->iVSThandle;
		csound->Message(csound, "vstplug_init: Plugin handle %d.\n", iVSThandle);
		VSTPlugin *plugin = pluginInstances[iVSThandle];
		csound->Message(csound, "vstplug_init: Plugin %x.\n", plugin);
		csound->Message(csound, "vstplug_init: nsmps %d.\n", p->nsamps);
		return OK;
	}
	
	int vstplug (void *data)
	{
		VST_PLUGIN *p = (VST_PLUGIN *)data;
        if(!p->h.insdshead->nxtact)
        {
            return OK;
        }
  		ENVIRON *csound = p->h.insdshead->csound;
		size_t iVSThandle = (size_t) *p->iVSThandle;
		VSTPlugin *plugin = pluginInstances[iVSThandle];
		//csound->Message(csound, "vstplug: Plugin handle %d.\n", iVSThandle);
		//csound->Message(csound, "vstplug: Plugin %x.\n", plugin);
		MYFLT *ain1 = p->ain1;
		MYFLT *ain2 = p->ain2;
		MYFLT *aout1 = p->aout1;
		MYFLT *aout2 = p->aout2;
		long nsamps = p->nsamps;
        plugin->inputs[0] = ain1;
        plugin->inputs[1] = ain2;
        plugin->outputs[0] = aout1;
        plugin->outputs[1] = aout2;
        int i;
  		for (i = 0; i < nsamps; i++) {
  		    ain1[i] = ain1[i] / SCALING_FACTOR;
  		    ain2[i] = ain2[i] / SCALING_FACTOR;
  		    aout1[i] = 0;
  		    aout2[i] = 0;
        }
		/* TODO (#4#): Agregar chequeo a si hay process replacing y 
		               ejecutar de acuerdo a eso */
		plugin->process(plugin->inputs, plugin->outputs, nsamps);
        for (i = 0; i < nsamps; i++) {
            aout1[i] = aout1[i] * SCALING_FACTOR;
            aout2[i] = aout2[i] * SCALING_FACTOR;
        }    
		return OK;
	}
	
	int vstinst_init (void *data)
	{
	    /* TODO (manta#1#): Cuando este listo vstplug copiarlo aqui modificado 
           o quitarlo y poner los parametros de entrada opcionales*/
		return OK;
	}
		
	int vstnote_init (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
		p->ktrigger = 1;
		p->kremaining = 0;
		return OK;
	}
	
	int vstnote (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
        ENVIRON *csound = p->h.insdshead->csound;
		size_t iVSThandle = *p->iVSThandle;
		MYFLT kchan = *p->kchan;
		MYFLT knote = *p->knote;
		MYFLT kveloc = *p->kveloc;
		MYFLT kdur = *p->kdur;
		if (p->ktrigger) {
		    csound->Message(csound, "vstnote: AddNoteOn(%f %f %f).\n", knote, kveloc, kchan);
			pluginInstances[iVSThandle]->AddNoteOn(knote, kveloc, kchan);
			p->ktrigger = 0;
			p->kremaining = kdur * csound->GetSr(csound);
		}
		if (p->kremaining) {
		    csound->Message(csound, "vstnote: AddNoteOff(%f %f).\n", knote, kchan);
            pluginInstances[iVSThandle]->AddNoteOff(knote, kchan);
            p->kremaining -= csound->GetKsmps(csound); 	
            if(p->kremaining < 0) {
                p->kremaining = 0;
            }
		}
		return OK;		
	}
	
	int outvst_init(void *data)
	{
		OUTVST *p = (OUTVST *)data;
		p->oldKstatus = 0.;
		p->oldKchan = 0.;
		p->oldKdata1 = 0.;
		p->oldKdata2 = 0.;
		return OK;
	}
	
	int outvst (void *data)  //pensar en agregar un parametro 'trigger' o solo enviar cuando cambia
	{
		OUTVST *p = (OUTVST *)data;
        ENVIRON *csound = p->h.insdshead->csound;
		size_t iVSThandle = (size_t) *p->iVSThandle;
		MYFLT kstatus = *p->kstatus;
		MYFLT kchan = *p->kchan;
		MYFLT kdata1 = *p->kdata1;
		MYFLT kdata2 = *p->kdata2;
		if (kstatus == p->oldKstatus && 
            kchan == p->oldKchan &&
            kdata1 == p->oldKdata1 && 
            kdata2 == p->oldKdata2)
		    return OK;
        p->oldKstatus = kstatus;
        p->oldKchan = kchan;
        p->oldKdata1 = kdata1;
        p->oldKdata2 = kdata2;
        csound->Message(csound, "outvst: %f %f %f %f.\n", kstatus, kchan, kdata1, kdata2);
		switch (int(kstatus)) {
		case 144: {  
				pluginInstances[iVSThandle]->AddNoteOn(kdata1, kdata2, kchan);
			}
			break;
		case 128: {  
				pluginInstances[iVSThandle]->AddNoteOff(kdata1, kchan);
			}
			break;
		/*case 160: 
			{
				pluginInstances[iVSThandle]->AddPolyAftertouch((int)data1, (int)kdata2, (int)kchan);
			}
		
		*/
			break;
		case 176: {
				pluginInstances[iVSThandle]->AddControlChange((int)kdata1, (int)kdata2);
				printf("Control %i = %i",int (kdata1), (int) kdata2);
			}
			break;
		case 192: {
				pluginInstances[iVSThandle]->AddProgramChange(int(kdata1));
				printf ("Program change");
			}
			break;
		case 208: {
				pluginInstances[iVSThandle]->AddAftertouch(int(kdata1));
				printf("Aftertouch");
			}
			break;
		case 224: {
				pluginInstances[iVSThandle]->AddPitchBend(int(kdata1));
			}
			break;
		default:
			printf("Warning: outvst: Invalid MIDI opcode");
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
		size_t iVSThandle = (size_t) *p->iVSThandle;
		*p->kvalue = pluginInstances[iVSThandle]->GetParamValue((int) *p->kparam);
		if ( *p->kvalue == (MYFLT) -1)
			csound->Message(csound, "Invalid parameter number %d.\n", 
               (int) *p->kparam);
		return OK;	
	}	
	
	int vstpsend_init (void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		p->oldparam = 0;
		p->oldvalue = 0;
  		return OK; 
	}

	int vstpsend(void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		size_t iVSThandle = *p->iVSThandle;
		MYFLT kparam = *p->kparam;
		MYFLT kvalue = *p->kvalue;
		if(p->oldparam == kparam && p->oldvalue == kvalue)
		     return OK;
        p->oldparam = kparam;
		p->oldvalue = kvalue;
		pluginInstances[iVSThandle]->SetParameter((int) kparam, (float) kvalue);
		return OK;	
	}
	
	void path_convert(char* in)
	{
		char inpath[strlen(in)];
		char outpath[0x100];
		int i= 0;
		int j = 0;
		strcpy(inpath, in);
		for (i= 0; i< 0x100-1; i++) {
		     if (inpath[i]) {
		         if (inpath[i]==47) {
		             outpath[j]= 92;
		         }
		         else
					outpath[j]= inpath[i];
			//printf("%i:%s|||",i++,inpath[i]);
			}
			else
			    outpath[j] = 0;
			j++;
		}
		in = outpath;
		//printf("%s\n", in);
		//return OK;
	}

    OENTRY vstOentry[] = { 
        {   "vstinit", sizeof (VSTINIT), 1, "i", "So", &vstinit, 0, 0, &vstinit_free },
		{	"vstinfo", sizeof (VSTINFO), 1, "", "i", &vstinfo, 0, 0, 0},
		{	"vstplug", sizeof (VST_PLUGIN), 5, "mm", "iaa", &vstplug_init, 0, &vstplug, 0},
		/* TODO (#1#): Cambiar por parametros opcionales */
		{	"vstnote", sizeof (VSTNOTE), 3, "", "ikkkk", &vstnote_init, &vstnote, 0, 0},
		{	"vstout", sizeof (OUTVST), 3, "", "ikkkk", &outvst_init, &outvst, 0, 0},
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
