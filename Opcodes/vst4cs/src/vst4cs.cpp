//  vst4cs: VST HOST OPCODES FOR CSOUND
//
//  Uses code by Hermann Seib from the vst~ object, 
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
	void path_convert(char* in);
		
	int vstinit (void *data)
	{		
  		VSTINIT *p = (VSTINIT *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = new VSTPlugin(csound);
        *p->iVSThandle = vstPlugins.size();
        vstPlugins.push_back(plugin);
		if (vstPlugins.size() == 1) {
           plugin->Log("=============================================================\n");
           plugin->Log("vst4cs version %s by Andres Cabrera and Michael Gogins\n", version.c_str());
           plugin->Log("Using code from Hermann Seib and the vst~ object in PD\n");
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
		path_convert (vstplugname);
		if (plugin->Instantiate(vstplugname)) {
			plugin->Log("Error loading effect.\n");
			return NOTOK;
		}
		plugin->Init();
		if (*p->iverbose) 
            plugin->Info();
		return OK;
	}
	
	int vstinit_free(void*data)
	{
		return OK;
	}
	
	int vstinfo(void *data)
	{
		VSTINFO *p = (VSTINFO *)data;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Info();
		return OK;
	}
	
	int vstplug_init(void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstplug_init.\n");
		p->framesPerBlock = csound->GetKsmps(csound);
  		p->channels = csound->GetNchnls(csound);
		return OK;
	}
	
	int vstplug(void *data)
	{
		VSTPLUG_ *p = (VSTPLUG_ *)data;
  		//ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		//plugin->Log("vstplug: plugin %x.\n", plugin);
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
	
	int vstnote_init (void * data)
	{
		VSTNOTE *p = (VSTNOTE *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *vstPlugin = vstPlugins[(size_t) *p->iVSThandle];
		vstPlugin->Debug("vstnote_init\n");
		p->framesRemaining = *p->kdur * vstPlugin->framesPerSecond;
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
                VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
                plugin->Debug("vstnote.\n");
                plugin->AddNoteOff(*p->kchan, *p->knote);
            }
        }
 		return OK;		
	}
	
	int outvst_init(void *data)
	{
		OUTVST_ *p = (OUTVST_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
  		plugin->Debug("outvst_init.\n");
		p->oldkstatus = 0;
		p->oldkchan = 0;
		p->oldkvalue = 0;
		return OK;
	}
	
	int outvst (void *data)
	{
		OUTVST_ *p = (OUTVST_ *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
        VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		if(round(p->oldkstatus) == round(*p->kstatus) &&
		   round(p->oldkchan) == round(*p->kchan) &&
		   round(p->oldkvalue) == round(*p->kdata1))
		   return OK;
  		plugin->Debug("outvst.\n");
    	p->oldkstatus = *p->kstatus;
    	p->oldkchan = *p->kchan;
     	p->oldkvalue = *p->kdata1;		
		switch (int(*p->kstatus))
		{
		case 144:  //noteon
			{
				plugin->AddNoteOn((int)(*p->kchan), *p->kdata1, *p->kdata2);
			}
			break;
		case 128:   //noteoff
			{
				plugin->AddNoteOff((int)(*p->kchan), *p->kdata1);
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
	
	int vstpret_init (void *data)
	{
  		return OK; 
	}
		
	int vstpret(void *data)
	{
		VSTPRET *p = (VSTPRET *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstpret(%d).\n", int(*p->kparam));
		*p->kvalue = plugin->GetParamValue(int(*p->kparam));
		if ((*p->kvalue) == -1)
			plugin->Log("Invalid parameter number %d.\n", int(*p->kparam));
		return OK;	
	}	
	
	int vstpsend_init (void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->Debug("vstpsend_init.\n");
		p->oldkparam = 0;
		p->oldkvalue = 0;
  		return OK; 
	}
	
	int vstpsend(void *data)
	{
		VSTPSEND *p = (VSTPSEND *)data;
  		ENVIRON *csound = p->h.insdshead->csound;
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
	
	int vstbload(void *data)
	{
  		VSTBLOAD *p = (VSTBLOAD *)data;
		ENVIRON *csound = p->h.insdshead->csound;
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
			csound->Message(csound,"Error: BeginLoadBank.\n");
  			return NOTOK;
		}
		//csound->Message(csound,"EffBeginLoadBank\n");
		if (fxBank.IsLoaded()) {
			if (plugin->aeffect->uniqueID != fxBank.GetFxID()) {
			    csound->Message(csound,"Error: Loaded bank ID doesn't match plug-in ID.\n");
			    return NOTOK;
            }
            if (fxBank.IsChunk()) {
    		    if (!(plugin->aeffect->flags & effFlagsProgramChunks)) {
                    csound->Message(csound,"Error: Loaded bank contains a chunk format that the effect can't handle.\n");
                    return NOTOK;
                }
    		    plugin->Dispatch( effSetChunk, 0, fxBank.GetChunkSize(), fxBank.GetChunk(), 0);  //isPreset=0
    		    csound->Message(csound,"Chunks loaded OK.\n");
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
    	    csound->Message(csound,"Problem loading bank.\n");
       		return NOTOK;        /* check if error loading */
    	}
    	csound->Message(csound,"Bank loaded OK.\n");
    	//if (fxBank.SetChunk()) {
    	//}
    	return OK;
    }
	
	int vstprogset(void *data)
	{
  		VSTPROGSET *p = (VSTPROGSET *)data;
		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->SetCurrentProgram(*p->iprogram);
    }

	int vsteditdlg(void *data)
	{
  		VSTPROGSET *p = (VSTPROGSET *)data;
		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->OpenEditor();
    }
	
    int vsteditdlg_deinit(void *data)
	{
  		VSTPROGSET *p = (VSTPROGSET *)data;
		ENVIRON *csound = p->h.insdshead->csound;
		VSTPlugin *plugin = vstPlugins[(size_t) *p->iVSThandle];
		plugin->CloseEditor();
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
        {"vstinit",    sizeof(VSTINIT),    1, "i",  "So",    &vstinit,       0,         0,        &vstinit_free     },
		{"vstinfo",    sizeof(VSTINFO),    1, "",   "i",     &vstinfo,       0,         0,        0                 },
		{"vstplug",    sizeof(VSTPLUG_),   5, "mm", "iaa",   &vstplug_init,  0,         &vstplug, 0                 },
		{"vstnote",    sizeof(VSTNOTE),    3, "",   "ikkkk", &vstnote_init,  &vstnote,  0,        0                 },
		{"vstout",     sizeof(OUTVST_),    3, "",   "ikkkk", &outvst_init,   &outvst,   0,        0                 },
  		{"vstpret",    sizeof(VSTPRET),    3, "k",  "ik",    &vstpret_init,  &vstpret,  0,        0                 },
  		{"vstpsend",   sizeof(VSTPSEND),   3, "",   "ikk",   &vstpsend_init, &vstpsend, 0,        0                 },
    	{"vstbload",   sizeof(VSTBLOAD),   1, "" ,  "iS",    &vstbload,      0,         0,        0                 },
    	{"vstprogset", sizeof(VSTPROGSET), 1, "" ,  "iS",    &vstprogset,    0,         0,        0                 },
    	{"vsteditdlg", sizeof(VSTEDITDLG), 1, "" ,  "i",     &vsteditdlg,    0,         0,        &vsteditdlg_deinit}
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
