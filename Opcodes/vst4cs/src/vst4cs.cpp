//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from his Vst Host program
//  and from the vst~ object by Thomas Grill, 
//  which in turn borrows from the Psycle tracker.
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Andres Cabrera, Michael Gogins
//
//  The vst4cs library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The vst4cs library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with The vst4cs library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA

#include "vst4cs.h"
#include "vsthost.h"
#include "fxbank.h"

#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

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
	std::string version = "0.1alpha";
#ifdef WIN32
	void path_convert(char* in);
#endif
		
	int vstinit (void *csound_, void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = new VSTPlugin(csound);
        *p->iVSThandle = vstPlugins.size();
        vstPlugins.push_back(plugin);
		if (vstPlugins.size() == 1) {
           plugin->Log("=============================================================\n");
           plugin->Log("vst4cs version %s by Andres Cabrera and Michael Gogins\n", version.c_str());
           plugin->Log("Using code from Hermann Seib's VstHost and Thomas Grill's vst~ object\n");
           plugin->Log("VST is a trademark of Steinberg Media Technologies GmbH\n");
           plugin->Log("VST Plug-In Technology by Steinberg\n");
           plugin->Log("=============================================================\n");
		}
		char vstplugname[0x100];
		if (*p->iplugin == SSTRCOD) {
			strcpy(vstplugname, p->STRARG);          
		}
		else 
           plugin->Log("Invalid plugin name.\n");
#if WIN32
		path_convert (vstplugname);
#endif
		if (plugin->Instantiate(vstplugname)) {
			plugin->Log("Error loading effect.\n");
			return NOTOK;
		}
		plugin->Init();
		if (*p->iverbose) 
            plugin->Info();
		return OK;
	}
	
	int vstdeinit(void *csound_, void *data)
	{
  		//VSTINIT *p = (VSTINIT *)data;
		//ENVIRON *csound = p->h.insdshead->csound;
		//VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		//delete plugin;
		return OK;
	}
	
	int vstinfo(void *csound_, void *data)
	{
		VSTINFO *p = (VSTINFO *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Info();
		return OK;
	}
	
	int vstaudio_init(void *csound_, void *data)
	{
		VSTAUDIO *p = (VSTAUDIO *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstaudio_init.\n");
		p->framesPerBlock = csound->GetKsmps(csound);
  		p->channels = csound->GetNchnls(csound);
		return OK;
	}
	
	int vstaudio(void *csound_, void *data)
	{
		VSTAUDIO *p = (VSTAUDIO *)data;
  		//ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		//plugin->Debug("vstaudio: plugin %x.\n", plugin);
        if(!p->h.insdshead->nxtact) {
            for(size_t i = 0; i < p->framesPerBlock; i++) {
                plugin->inputs_[0][i] = p->ain1[i] / SCALING_FACTOR;
                plugin->inputs_[1][i] = p->ain2[i] / SCALING_FACTOR;
                plugin->outputs_[0][i] = FL(0.0);
                plugin->outputs_[1][i] = FL(0.0);
            }
            plugin->process(&plugin->inputs.front(), &plugin->outputs.front(), p->framesPerBlock);
            for(size_t i = 0; i < p->framesPerBlock; i++) {
                p->aout1[i] = plugin->outputs_[0][i] * SCALING_FACTOR;
                p->aout2[i] = plugin->outputs_[1][i] * SCALING_FACTOR;
            }
        } else {
            for(size_t i = 0; i < p->framesPerBlock; i++) {
               p->aout1[i] = p->ain1[i];
               p->aout2[i] = p->ain2[i];
            }	
        }
		return OK;
	}
	
	int vstaudiog(void *csound_, void *data)
	{
		VSTAUDIO *p = (VSTAUDIO *)data;
  		//ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		//plugin->Debug("vstaudio: plugin %x.\n", plugin);
        for(size_t i = 0; i < p->framesPerBlock; i++) {
            plugin->inputs_[0][i] = p->ain1[i] / SCALING_FACTOR;
            plugin->inputs_[1][i] = p->ain2[i] / SCALING_FACTOR;
            plugin->outputs_[0][i] = FL(0.0);
            plugin->outputs_[1][i] = FL(0.0);
        }
        plugin->process(&plugin->inputs.front(), &plugin->outputs.front(), p->framesPerBlock);
        for(size_t i = 0; i < p->framesPerBlock; i++) {
            p->aout1[i] = plugin->outputs_[0][i] * SCALING_FACTOR;
            p->aout2[i] = plugin->outputs_[1][i] * SCALING_FACTOR;
        }
		return OK;
	}
	
	int vstnote_init (void *csound_, void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
		VSTPlugin *vstPlugin = vstPlugins[(size_t) *p->iVSThandle];
		vstPlugin->Debug("vstnote_init\n");
		p->framesRemaining = *p->kdur * vstPlugin->framesPerSecond;
		vstPlugin->AddNoteOn(p->h.insdshead->p2, int(*p->kchan), *p->knote, *p->kveloc);
		return OK;
	}
		
	int vstnote (void *csound_, void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
        if(p->framesRemaining >= 0) {
            ENVIRON *csound = p->h.insdshead->csound;
            p->framesRemaining -= (size_t) csound->GetKsmps(csound);
            if(p->framesRemaining <= 0) {
                VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
                plugin->Debug("vstnote.\n");
                plugin->AddNoteOff(p->h.insdshead->p2, int(*p->kchan), *p->knote);
            }
        }
 		return OK;		
	}
	
	int vstmidiout_init(void *csound_, void *data)
	{
		VSTMIDIOUT *p = (VSTMIDIOUT *)data;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
  		plugin->Debug("vstmidiout_init.\n");
		p->oldkstatus = 0;
		p->oldkchan = 0;
		p->oldkvalue = 0;
		return OK;
	}
	
	int vstmidiout (void *csound_, void *data)
	{
		VSTMIDIOUT *p = (VSTMIDIOUT *)data;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		if(round(p->oldkstatus) == round(*p->kstatus) &&
		   round(p->oldkchan) == round(*p->kchan) &&
		   round(p->oldkvalue) == round(*p->kdata1))
		   return OK;
  		plugin->Debug("vstmidiout.\n");
    	p->oldkstatus = *p->kstatus;
    	p->oldkchan = *p->kchan;
     	p->oldkvalue = *p->kdata1;		
		switch (int(*p->kstatus))
		{
		case 144:  //noteon
			{
				plugin->AddNoteOn(p->h.insdshead->p2, (int)(*p->kchan), *p->kdata1, *p->kdata2);
			}
			break;
		case 128:   //noteoff
			{
				plugin->AddNoteOff(p->h.insdshead->p2, (int)(*p->kchan), *p->kdata1);
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
				plugin->AddControlChange((int)*p->kdata1, (int)*p->kdata2);
			}
			break;
		case 192:   //program change
			{
				plugin->AddProgramChange(int(*p->kdata1));
			}
			break;
		case 208: //aftertouch
			{
				plugin->AddAftertouch(int(*p->kdata1));
			}
			break;
		case 224:   //pitch bend
			{
				plugin->AddPitchBend(int(*p->kdata1));
			}
			break;
		default:
			break;
		}
		return OK;
	}
	
	int vstparamget_init (void *csound_, void *data)
	{
  		return OK; 
	}
		
	int vstparamget(void *csound_, void *data)
	{
		VSTPARAMGET *p = (VSTPARAMGET *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstparamset(%d).\n", int(*p->kparam));
		*p->kvalue = plugin->GetParamValue(int(*p->kparam));
		if ((*p->kvalue) == -1)
			plugin->Log("Invalid parameter number %d.\n", int(*p->kparam));
		return OK;	
	}	
	
	int vstparamset_init (void *csound_, void *data)
	{
		VSTPARAMSET *p = (VSTPARAMSET *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstparamset_init.\n");
		p->oldkparam = 0;
		p->oldkvalue = 0;
  		return OK; 
	}
	
	int vstparamset(void *csound_, void *data)
	{
		VSTPARAMSET *p = (VSTPARAMSET *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		if(*p->kparam == p->oldkparam &&
           *p->kvalue == p->oldkvalue)
            return OK;
        p->oldkparam = *p->kparam;
        p->oldkvalue = *p->kvalue;
		plugin->Debug("vstsend(%d, %f).\n", int(*p->kparam), *p->kvalue);
		plugin->SetParameter(int(*p->kparam), float (*p->kvalue));
		return OK;	
	}
	
	int vstbankload(void *csound_, void *data)
	{
  		VSTBANKLOAD *p = (VSTBANKLOAD *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		void *dummyPointer = 0;          
		char bankname[64];
		if (*p->ibank == SSTRCOD) {
			strcpy(bankname, p->STRARG);          /*   use that         */
		}
		CFxBank fxBank(bankname);            /* load the bank                     */
		plugin->Dispatch(effBeginLoadBank, 
            0, 0, (VstPatchChunkInfo*) fxBank.GetChunk(), 0);
		if (plugin->Dispatch(effBeginLoadBank, 
            0, 0, (VstPatchChunkInfo*) fxBank.GetChunk(), 0)) {
			plugin->Log("Error: BeginLoadBank.\n");
  			return NOTOK;
		}
		//csound->Message(csound,"EffBeginLoadBank\n");
		if (fxBank.IsLoaded()) {
			if (plugin->aeffect->uniqueID != fxBank.GetFxID()) {
			    plugin->Log("Error: Loaded bank ID doesn't match plug-in ID.\n");
			    return NOTOK;
            }
            if (fxBank.IsChunk()) {
    		    if (!(plugin->aeffect->flags & effFlagsProgramChunks)) {
                    plugin->Log("Error: Loaded bank contains a chunk format that the effect can't handle.\n");
                    return NOTOK;
                }
    		    plugin->Dispatch( effSetChunk, 0, fxBank.GetChunkSize(), fxBank.GetChunk(), 0);  //isPreset=0
    		    plugin->Log("Chunks loaded OK.\n");
            } else {
    		    //int cProg = plugin->EffGetProgram();
    		    int cProg = plugin->Dispatch(effGetProgram, 0,0,dummyPointer,0);
    		    //csound->Message(csound,"Current Program= %i\n",cProg);
    		    int i, j;
    		    int nParms = fxBank.GetNumParams();
    		    //csound->Message(csound,"nParms= %i\n",nParms);
    		    for (i = 0; i < fxBank.GetNumPrograms(); i++) {
                    plugin->Dispatch(effSetProgram,0,i,dummyPointer,0);
                    plugin->Dispatch(effSetProgramName,0,0,fxBank.GetProgramName(i),0);
                    for (j = 0; j < nParms; j++)
    					plugin->SetParameter(j, fxBank.GetProgParm(i, j));
    		    }
    		    //pEffect->EffSetProgram(cProg);
    		    plugin->Dispatch(effSetProgram,0,cProg,dummyPointer,0);
    		    //csound->Message(csound,"Programs OK\n");
    	    }
      	    //pEffect->SetChunkFile(dlg.GetPathName());
      	    //ShowDetails();
      	    //OnSetProgram();
    	} else {
    	    plugin->Log("Problem loading bank.\n");
       		return NOTOK;        /* check if error loading */
    	}
    	plugin->Log("Bank loaded OK.\n");
    	//if (fxBank.SetChunk()) {
    	//}
    	return OK;
    }
	
	int vstprogset(void *csound_, void *data)
	{
  		VSTPROGSET *p = (VSTPROGSET *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->SetCurrentProgram(int(*p->iprogram));
		return OK;
    }

	int vstedit_init(void *csound_, void *data)
	{
  		VSTEDIT *p = (VSTEDIT *)data;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
        plugin->OpenEditor();
		return OK;
    }
	
    int vstedit_deinit(void *csound_, void *data)
	{
  		VSTEDIT *p = (VSTEDIT *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->CloseEditor();
		return OK;
    }

#ifdef WIN32
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
#endif
	
    OENTRY vstOentry[] = { 
        {"vstinit",     sizeof(VSTINIT),     1, "i",  "So",    &vstinit,          0,            0,          &vstdeinit     },
		{"vstinfo",     sizeof(VSTINFO),     1, "",   "i",     &vstinfo,          0,            0,          0              },
		{"vstaudio",    sizeof(VSTAUDIO),    5, "mm", "iaa",   &vstaudio_init,    0,            &vstaudio,  0              },
		{"vstaudiog",   sizeof(VSTAUDIO),    5, "mm", "iaa",   &vstaudio_init,    0,            &vstaudiog, 0              },
		{"vstnote",     sizeof(VSTNOTE),     3, "",   "ikkkk", &vstnote_init,     &vstnote,     0,          0              },
		{"vstmidiout",  sizeof(VSTMIDIOUT),  3, "",   "ikkkk", &vstmidiout_init,  &vstmidiout,  0,          0              },
  		{"vstparamget", sizeof(VSTPARAMGET), 3, "k",  "ik",    &vstparamget_init, &vstparamget, 0,          0              },
  		{"vstparamset", sizeof(VSTPARAMSET), 3, "",   "ikk",   &vstparamset_init, &vstparamset, 0,          0              },
    	{"vstbankload", sizeof(VSTBANKLOAD), 1, "" ,  "iS",    &vstbankload,      0,            0,          0              },
    	{"vstprogset",  sizeof(VSTPROGSET),  1, "" ,  "iS",    &vstprogset,       0,            0,          0              },
    	{"vstedit",     sizeof(VSTEDIT),     1, "" ,  "ik",    &vstedit_init,     0,            0,          &vstedit_deinit}
    };
   
    /**
    * Called by Csound to obtain the size of
    * the table of OENTRY structures defined in this shared library.
    */
    PUBLIC int opcode_size()
    {
        return sizeof(vstOentry);
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
