#include "vst4cs.h"
#include "vsthost.h"
#include <stdlib.h>

extern "C"
{
	std::vector<VSTPlugin *> pluginInstances;
	char vst4csver []= "0.1alpha";
	void path_convert(char* in);
	extern double SCALING_FACTOR = 32767.0;
	
	int vstinit (void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		MYFLT *localhandle;
		MYFLT *verbose;
		char vstplugname[64];
		MYFLT sr = csound->GetSr(csound);
		MYFLT kr = csound->GetKr(csound);	
		if (vsthandle == -1) {
            csound->Message(csound, "\nvst4cs version %s by Andres Cabrera,\n", vst4csver);
            csound->Message(csound, "using code from Hermann Seib and the vst~ object in pd.\n");
            csound->Message(csound, "VST is a trademark of Steinberg Media Technologies GmbH\n");
            csound->Message(csound, "VST Plug-In Technology by Steinberg.\n");
        }
		localhandle = p->iVSThandle;
		verbose = p->iverbose;
		if (*p->iplugin == SSTRCOD) {
			strcpy(vstplugname, p->STRARG);
			//printf("Parameter: %s\n",vstplugname);
		}
		else 
            csound->Message(csound, "Invalid VST plugin name.\n");
		path_convert (vstplugname);
		//csound->Message(csound, "Parameter : %s\n",vststring);
		pluginInstances.push_back(new VSTPlugin());
		vsthandle = pluginInstances.size() - 1;
		*localhandle = vsthandle;
		//printf("Loading: %s",vstplugname);
		//pluginInstances[vsthandle] = new VSTPlugin;
		if (pluginInstances[vsthandle]->Instance(csound, vstplugname)) {
			csound->Message(csound, "VST plugin could not be loaded.\n");
			return 0; 
		}
		pluginInstances[vsthandle]->Init(sr, kr);
		if (*verbose) 
		    pluginInstances[vsthandle]->Info(csound);
		//printf("Plugin address: %i\n",Effect[vsthandle]);
		return OK;
	}

	int vstinit_free(void*data)
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
		MYFLT *localhandle;
		localhandle = p->iVSThandle;
		pluginInstances[int(*localhandle)]->Info(csound);
		return OK;
	}
	
	int vstplug_init (void *data)
	{
		VST_PLUGIN *p = (VST_PLUGIN *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		*p->nsamps = csound->GetKsmps(csound);
  		//csound->Message(csound, "Number of ksmps: %i\n", int(p->nsamps));
		return OK;
	}
	
	int vstplug (void *data)
	{
		VST_PLUGIN *p = (VST_PLUGIN *)data;
		size_t iVSThandle = (size_t) *p->iVSThandle;
		MYFLT *aout1 = p->aout1;
		MYFLT *aout2 = p->aout2;
		MYFLT *ain1 = p->ain1;
		MYFLT *ain2 = p->ain2;
		size_t nsamps = (size_t) *p->nsamps;
		float inputs[2][nsamps];
		float outputs[2][nsamps];
		int i;
  		for (i = 0; i < nsamps; i++) {
  		    inputs[0][i] = ain1[i] / SCALING_FACTOR;
  		    inputs[1][i] = ain2[i] / SCALING_FACTOR;
  		    outputs[0][i] = 0;
  		    outputs[1][i] = 0;
        }
		/* TODO (#4#): Agregar chequeo a si hay process replacing y 
		               ejecutar de acuerdo a eso */
		pluginInstances[iVSThandle]->process((float **)inputs, (float **)outputs, (long) nsamps);
        for (i = 0; i <  nsamps; i++) {
            aout1[i] = outputs[0][i] * SCALING_FACTOR;
            aout2[i] = outputs[1][i] * SCALING_FACTOR;
        }    
		return OK;
	}
	
	int vstinst_init (void *data)
	{
		return OK;
	/* TODO (manta#1#): Cuando este listo vstplug copiarlo aqui modificado o quitarlo y poner los parametros de entrada opcionales*/
	}
	
	int vstinst (void *data)
	{
		return OK;
	}
	
	int vstnote_init (void * data)
	{
		/* TODO (#3#): Pensar en array para polifonia */
		VSTNOTE *p = (VSTNOTE *)data;
		MYFLT elapse = 0;
		MYFLT count = 0;
		p->counting = &elapse;
		p->elapsed = &count; 		
		return OK;
	}
	
	int vstnote (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
  		int controlrate = csound->GetKr(csound); 	
		MYFLT *ktrigger = p->ktrigger;
		MYFLT *iVSThandle = p->iVSThandle;
		MYFLT *kchan = p->kchan;
		MYFLT *knote = p->knote;
		MYFLT *kveloc = p->kveloc;
		MYFLT *kdur = p->kdur;
		MYFLT *elapsed = p->elapsed;
		MYFLT *counting = p->counting;
		int elapse;
		bool count;
		if (*ktrigger!= 0) {
			count = true;
			pluginInstances[int(*iVSThandle)]->AddNoteOn((int)(*knote), (int)(*kveloc), (int)(*kchan));
			printf ("%i",(int)*counting);
			elapse = 0;
		};
		if (count) {
		    elapse = elapse +1;
  		    if ( (elapse) > (*kdur *controlrate)) {
  			    pluginInstances[int(*iVSThandle)]->AddNoteOff((int)(*knote), (int)(*kchan));
  			    count = false;
  			    elapse = 0;
  			    //break;
		    };
		};
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
		int iVSThandle = (int) *p->iVSThandle;
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
		VSTPRET *p = (VSTPRET *)data;
		p->oldvalue = 0;
  		return OK; 
	}
		
	int vstpret(void *data)
	{
		VSTPRET *p = (VSTPRET *)data;
		MYFLT *kvalue = p->kvalue;
		MYFLT *iVSThandle = p->iVSThandle;
		MYFLT *kparam = p->kparam;
		MYFLT oldvalue = p->oldvalue; //pensar en quitar oldvalue
		*kvalue = pluginInstances[int(*iVSThandle)]->GetParamValue(int(*kparam));
		if ( (*kvalue) == -1)
			printf ("Invalid Parameter number.\n");
		//printf ("Value = %f", (float)(*kvalue));
		return OK;	
	}	
	
	int vstpsend_init (void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		p->oldvalue = 0;
  		return OK; 
	}

	int vstpsend(void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
		MYFLT *iVSThandle = p->iVSThandle;
		MYFLT *kparam = p->kparam;
		MYFLT *kvalue = p->kvalue;
		MYFLT oldvalue = p->oldvalue; //pensar en quitar oldvalue
		pluginInstances[int(*iVSThandle)]->SetParameter(int(*kparam), float (*kvalue));
		//printf ("Value = %f", (float)(*kvalue));
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
		{	"vstplug", sizeof (VST_PLUGIN), 3, "mm", "iaa", &vstplug_init, 0, &vstplug, 0},
		/* TODO (#1#): Cambiar por parametros opcionales */
		{	"vstnote", sizeof (VSTNOTE), 3, "", "kikkkk", &vstnote_init, &vstnote, 0, 0},
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
