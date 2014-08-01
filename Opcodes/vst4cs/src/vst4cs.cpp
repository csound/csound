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

#ifdef _WIN32
#pragma warning(disable:4786) //gab
#endif

#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include "vst4cs.h"
#include "vsthost.h"
#include "fxbank.h"

// These two collections replace similar ones that used to be in widglobals.h.

std::vector<VSTPlugin*> &vstPlugEditors()
{
    static std::vector<VSTPlugin*> vstPlugEditors_;
    return vstPlugEditors_;
}

std::vector<VSTPlugin*> &vstPlugins()
{
    static std::vector<VSTPlugin*> vstPlugins_;
    return vstPlugins_;
}


extern "C" {
  std::string version = "0.2beta";
#ifdef WIN32
  static void path_convert(char *in);
#endif

    static MYFLT getCurrentTime(CSOUND *csound)
    {
        return csound->GetCurrentTimeSamples(csound) / csound->GetSr(csound);
    }

  static int vstinit(CSOUND *csound, void *data)
  {
    VSTINIT *p = (VSTINIT *) data;
    VSTPlugin *plugin = new VSTPlugin(csound);
    *p->iVSThandle = (MYFLT) vstPlugins().size();
    vstPlugins().push_back(plugin);
    if ((int) vstPlugins().size() == 1) {
      plugin->Log("============================================================\n");
      plugin->Log("vst4cs version %s by Andres Cabrera and Michael Gogins\n",
                  version.c_str());
      plugin->Log("Using code from H. Seib's VstHost and T. Grill's vst~ object\n");
      plugin->Log("VST is a trademark of Steinberg Media Technologies GmbH\n");
      plugin->Log("VST Plug-In Technology by Steinberg\n");
      plugin->Log("============================================================\n");
    }
    char vstplugname[0x100];
    strncpy(vstplugname,((STRINGDAT *)p->iplugin)->data, MAXNAME-1);
#if WIN32
    path_convert(vstplugname);
#endif
    if (plugin->Instantiate(vstplugname)) {
      csound->InitError(csound, "vstinit: Error loading effect.");
      csound->LongJmp(csound, 1);
    }
    plugin->Init();
    if (*p->iverbose)
      plugin->Info();
    return OK;
  }

  static int vstinfo(CSOUND *csound, void *data)
  {
    VSTINFO *p = (VSTINFO *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->Info();
    return OK;
  }

  static int vstaudio_init(CSOUND *csound, void *data)
  {
    VSTAUDIO *p = (VSTAUDIO *) data;

    p->opcodeInChannels = (size_t) (csound->GetInputArgCnt(data) - 1);
    if (p->opcodeInChannels > 32)
      csound->InitError(csound, "vstaudio: too many input args");
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->Debug("vstaudio_init.\n");
    p->framesPerBlock = csound->GetKsmps(csound);
    p->pluginInChannels  = (size_t) plugin->getNumInputs();
    p->pluginOutChannels = (size_t) plugin->getNumOutputs();
    p->opcodeOutChannels = (size_t) csound->GetOutputArgCnt(data);
    return OK;
  }

  static int vstaudio(CSOUND *csound, void *data)
  {
    VSTAUDIO *p = (VSTAUDIO *) data;
    size_t  i, j;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    for(j=0; j < p->pluginOutChannels; j++)
       memset(p->aouts[j], '\0', offset*sizeof(MYFLT));
    // plugin->Debug("vstaudio: plugin %x.\n", plugin);
    if (!p->h.insdshead->nxtact) {
      for (j = 0; j < p->pluginInChannels && j < p->opcodeInChannels; j++)
        for (i = offset; i < p->framesPerBlock; i++)
          plugin->inputs_[j][i] =
            (float) (p->ains[j][i] / csound->Get0dBFS(csound));
      for ( ; j < p->pluginInChannels; j++)
        for (i = 0; i < p->framesPerBlock; i++)
          plugin->inputs_[j][i] = 0.0f;
      for (j = 0; j < p->pluginOutChannels; j++)
        for (i = offset; i < p->framesPerBlock; i++)
          plugin->outputs_[j][i] = 0.0f;
      plugin->process(&plugin->inputs.front(), &plugin->outputs.front(),
                      p->framesPerBlock);
      for (j = 0; j < p->pluginOutChannels && j < p->opcodeOutChannels; j++)
        for (i = offset; i < p->framesPerBlock; i++)
          p->aouts[j][i] = (MYFLT) plugin->outputs_[j][i] * csound->Get0dBFS(csound);
      for ( ; j < p->opcodeOutChannels; j++)
        for (i = 0; i < p->framesPerBlock; i++)
          p->aouts[j][i] = FL(0.0);
    }
    else {
      for (j = 0; j < p->opcodeInChannels && j < p->opcodeOutChannels; j++)
        for (i = offset; i < p->framesPerBlock; i++)
          p->aouts[j][i] = p->ains[j][i];
      for ( ; j < p->opcodeOutChannels; j++)
        for (i = 0; i < p->framesPerBlock; i++)
          p->aouts[j][i] = FL(0.0);
    }
    return OK;
  }

  static int vstaudiog(CSOUND *csound, void *data)
  {
    VSTAUDIO *p = (VSTAUDIO *) data;
    size_t  i, j;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    for(j=0; j < p->pluginOutChannels; j++)
       memset(p->aouts[j], '\0', offset*sizeof(MYFLT));
    for (j = 0; j < p->pluginInChannels && j < p->opcodeInChannels; j++)
      for (i = offset; i < p->framesPerBlock; i++)
        plugin->inputs_[j][i] =
          (float) (p->ains[j][i] / csound->Get0dBFS(csound));
    for ( ; j < p->pluginInChannels; j++)
      for (i = 0; i < p->framesPerBlock; i++)
        plugin->inputs_[j][i] = 0.0f;
    for (j = 0; j < p->pluginOutChannels; j++)
      for (i = offset; i < p->framesPerBlock; i++)
        plugin->outputs_[j][i] = 0.0f;
    plugin->process(&plugin->inputs.front(), &plugin->outputs.front(),
                    p->framesPerBlock);
    for (j = 0; j < p->pluginOutChannels && j < p->opcodeOutChannels; j++)
      for (i = offset; i < p->framesPerBlock; i++)
        p->aouts[j][i] = (MYFLT) plugin->outputs_[j][i] * csound->Get0dBFS(csound);
    for ( ; j < p->opcodeOutChannels; j++)
      for (i = 0; i < p->framesPerBlock; i++)
        p->aouts[j][i] = FL(0.0);
    return OK;
  }


  static int vstmidiout_init(CSOUND *csound, void *data)
  {
    VSTMIDIOUT *p = (VSTMIDIOUT *) data;
    VSTPlugin *plugin;
    p->vstHandle = (size_t) *p->iVSThandle;
    plugin = vstPlugins()[p->vstHandle];
    plugin->Debug("vstmidiout_init.\n");
    p->prvMidiData = 0;
    return OK;
  }

  static int vstmidiout(CSOUND *csound, void *data)
  {
    VSTMIDIOUT *p = (VSTMIDIOUT *) data;
    VSTPlugin *plugin;
    int     st, ch, d1, d2, midiData;
    st = (int) *(p->kstatus);
    if (st < 128 || st >= 240) {
      p->prvMidiData = 0;
      return OK;
    }
    ch = (int) *(p->kchan) & 15;
    if ((st & 15) > ch)
      ch = st & 15;
    st &= 240;
    d1 = (int) *(p->kdata1);
    if (st != 192 && st != 208) {
      d2 = (int) *(p->kdata2);
      if (st == 224) {
        d2 += ((d1 >> 7) & 127);
        d1 &= 127;
      }
      d2 = (d2 >= 0 ? (d2 < 128 ? d2 : 127) : 0);
    }
    else
      d2 = 0;
    d1 = (d1 >= 0 ? (d1 < 128 ? d1 : 127) : 0);
    midiData = st | ch | (d1 << 8) | (d2 << 16);
    if (midiData == p->prvMidiData)
      return OK;
    p->prvMidiData = midiData;
    plugin = vstPlugins()[p->vstHandle];
    plugin->Debug("vstmidiout. kstatus = %i kdata1 = %i kdata2 = %i"
                  "--- mididata = %i\n",
                  (int) *(p->kstatus),
                  (int) *(p->kdata1),
                  (int) *(p->kdata2),
                  midiData);
    plugin->AddMIDI(midiData, 0, 0);
    return OK;
  }

  static int vstparamget_init(CSOUND *csound, void *data)
  {
    return OK;
  }

  static int vstparamget(CSOUND *csound, void *data)
  {
    VSTPARAMGET *p = (VSTPARAMGET *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->Debug("vstparamset(%d).\n", int(*p->kparam));
    *p->kvalue = plugin->GetParamValue(int(*p->kparam));
    if (*(p->kvalue) == FL(-1.0))
      plugin->Log("Invalid parameter number %d.\n", int(*p->kparam));
    return OK;
  }

  static int vstparamset_init(CSOUND *csound, void *data)
  {
    VSTPARAMSET *p = (VSTPARAMSET *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->Debug("vstparamset_init.\n");
    p->oldkparam = 0;
    p->oldkvalue = 0;
    return OK;
  }

  static int vstparamset(CSOUND *csound, void *data)
  {
    VSTPARAMSET *p = (VSTPARAMSET *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    if (*p->kparam == p->oldkparam && *p->kvalue == p->oldkvalue)
      return OK;
    p->oldkparam = *p->kparam;
    p->oldkvalue = *p->kvalue;
    plugin->Debug("vstsend(%d, %f).\n", int(*p->kparam), *p->kvalue);
    plugin->SetParameter(int(*p->kparam), float(*p->kvalue));
    return OK;
  }

  static int vstbankload(CSOUND *csound, void *data)
  {
    VSTBANKLOAD *p = (VSTBANKLOAD *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    void    *dummyPointer = 0;
    CFxBank fxBank((char *) p->ibank);          /* load the bank    */
    plugin->Dispatch(effBeginLoadBank,
                     0, 0, (VstPatchChunkInfo *) fxBank.GetChunk(), 0);
    if (plugin->Dispatch(effBeginLoadBank,
                         0, 0, (VstPatchChunkInfo *) fxBank.GetChunk(), 0)) {
      csound->InitError(csound, "Error: BeginLoadBank.");
      return NOTOK;
    }
    if (fxBank.IsLoaded()) {
      if (plugin->aeffect->uniqueID != fxBank.GetFxID()) {
        csound->InitError(csound, "Loaded bank ID doesn't match plug-in ID.");
        return NOTOK;
      }
      if (fxBank.IsChunk()) {
        if (!(plugin->aeffect->flags & effFlagsProgramChunks)) {
          csound->InitError(csound, "Loaded bank contains a chunk format "
                            "that the effect cannot handle.");
          return NOTOK;
        }
        plugin->Dispatch(effSetChunk, 0, fxBank.GetChunkSize(),
                         fxBank.GetChunk(), 0); // isPreset = 0
        plugin->Log("Chunks loaded OK.\n");
      }
      else {
        int     cProg = plugin->Dispatch(effGetProgram, 0, 0, dummyPointer, 0);
        int     i, j;
        int     nParms = fxBank.GetNumParams();
        for (i = 0; i < fxBank.GetNumPrograms(); i++) {
          plugin->Dispatch(effSetProgram, 0, i, dummyPointer, 0);
          plugin->Dispatch(effSetProgramName, 0, 0, fxBank.GetProgramName(i),
                           0);
          for (j = 0; j < nParms; j++)
            plugin->SetParameter(j, fxBank.GetProgParm(i, j));
        }
        plugin->Dispatch(effSetProgram, 0, cProg, dummyPointer, 0);
      }
    }
    else {
      csound->InitError(csound, "Problem loading bank.");
      return NOTOK;           /* check if error loading */
    }
    plugin->Log("Bank loaded OK.\n");
    return OK;
  }

  static int vstprogset(CSOUND *csound, void *data)
  {
    // The changes here are part of an attempt to map 0 to 1 and others
    VSTPROGSET *p = (VSTPROGSET *) data;
    int program = (int)*p->iprogram;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    if (program<=0) {
      csound->Message(csound, "VSTprogset: Program %d treated as 1\n", program);
      program = 1;
    }
    plugin->SetCurrentProgram(program);
    return OK;
  }

  static int vstedit_init(CSOUND *csound, void *data)
  {
    VSTEDIT *p = (VSTEDIT *) data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->OpenEditor();
    //plugin->targetFLpanel = vstPlugEditors().size()-1; //gab
    vstPlugEditors().push_back(plugin); //gab
    return OK;
  }

  static int vstSetTempo(CSOUND *csound, void *data)
  {
    VSTTEMPO *p = (VSTTEMPO *)data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    plugin->vstTimeInfo.tempo = *p->tempo;
    return OK;
  }

  int vstbanksave(CSOUND *csound, void *data)
  {
    VSTBANKLOAD *p = (VSTBANKLOAD *)data;
    VSTPlugin *plugin = vstPlugins()[(size_t) *p->iVSThandle];
    char bankname[512]; //gab
    strcpy(bankname, (char *) p->ibank);          /*   use that         */
    if (!plugin)
      return NOTOK;
    CFxBank b;

    if (plugin->aeffect->flags & effFlagsProgramChunks)
      {
        void * pChunk;
        int lSize = plugin->EffGetChunk(&pChunk);
        if (lSize)
          b.SetSize(lSize);
        if (b.IsLoaded())
          b.SetChunk(pChunk);
      }
    else
      {
        b.SetSize(plugin->aeffect->numPrograms,
                  plugin->aeffect->numParams);
        if (b.IsLoaded())
          {
            int i, j;
            int cProg = plugin->EffGetProgram();
            int nParms = b.GetNumParams();
            for (i = 0; i < b.GetNumPrograms(); i++)
              {
                plugin->EffSetProgram(i);
                char szName[128];
                plugin->EffGetProgramName(szName);
                b.SetProgramName(i, szName);
                for (j = 0; j < nParms; j++)
                  b.SetProgParm(i, j,
                                plugin->aeffect->getParameter(plugin->aeffect,j));
              }
            plugin->EffSetProgram(cProg);
          }
      }
    if (!b.IsLoaded())
      {
        plugin->Log("Error: Memory Allocation Error.\n");
        return NOTOK;
      }
    b.SetFxID(plugin->aeffect->uniqueID);
    b.SetFxVersion(plugin->aeffect->version);
    if (b.SaveBank(bankname))
      {
        plugin->Log("%s Bank saved OK.\n",bankname);
      }
    else {
      plugin->Log("Error: Error saving file\n");
      return NOTOK;
    }
    return OK;
  }


#ifdef WIN32
  static void path_convert(char *in)
  {
    for (int i = 0; in[i] != '\0'; i++) {
      if (in[i] == '/')
        in[i] = '\\';
    }
  }
#endif

  typedef struct VSTNOTEOUT_ {
    OPDS h;
    MYFLT *iVSThandle;
    MYFLT *iChannel;
    MYFLT *iKey;
    MYFLT *iVelocity;
    MYFLT *iDuration;
    VSTPlugin *vstPlugin;
    MYFLT startTime;
    MYFLT offTime;
    int channel;
    int key;
    int velocity;
    int on;
  } VSTNOTEOUT;

  static int vstnote_init(CSOUND *csound, void *data)
  {
    VSTNOTEOUT *p = (VSTNOTEOUT *)data;
    size_t vstHandle = (size_t) *p->iVSThandle;
    p->vstPlugin = vstPlugins()[vstHandle];
    p->startTime = getCurrentTime(csound);
    double onTime = double(p->h.insdshead->p2.value);
    double deltaTime = onTime - getCurrentTime(csound);
    int deltaFrames = 0;
    if (deltaTime > 0) {
      deltaFrames = int(deltaTime / csound->GetSr(csound));
    }
    // Use the warped p3 to schedule the note off message.
    if (*p->iDuration > FL(0.0)) {
      p->offTime = p->startTime + double(p->h.insdshead->p3.value);
      // In case of real-time performance with indefinite p3...
    } else if (*p->iDuration == FL(0.0)) {
      if (csound->GetDebug(csound)) {
        csound->Message(csound, "vstnote_init: not scheduling 0 duration note.\n");
      }
      return OK;
    } else {
      p->offTime = p->startTime + FL(1000000.0);
    }
    p->channel = int(*p->iChannel) & 0xf;
    // Split the real-valued MIDI key number
    // into an integer key number and an integer number of cents (plus or
    // minus 50 cents).
    p->key = int(double(*p->iKey) + 0.5);
    int cents = int( ( ( double(*p->iKey) - double(p->key) ) * double(100.0) ) +
                     double(0.5) );
    p->velocity = int(*p->iVelocity) & 0x7f;
    p->vstPlugin->AddMIDI(144 | p->channel | (p->key << 8) | (p->velocity << 16),
                          deltaFrames, cents);
    // Ensure that the opcode instance is still active when we are scheduled
    // to turn the note off!
    p->h.insdshead->xtratim = p->h.insdshead->xtratim + 2;
    p->on = true;
    if (csound->GetDebug(csound)) {
      csound->Message(csound, "vstnote_init:      on time:      %f\n", onTime);
      csound->Message(csound, "                   csound time:  %f\n", getCurrentTime(csound));
      csound->Message(csound, "                   delta time:   %f\n", deltaTime);
      csound->Message(csound, "                   delta frames: %d\n", deltaFrames);
      csound->Message(csound, "                   off time:     %f\n", p->offTime);
      csound->Message(csound, "                   channel:      %d\n", p->channel);
      csound->Message(csound, "                   key:          %d\n", p->key);
      csound->Message(csound, "                   cents:        %d\n", cents);
      csound->Message(csound, "                   velocity:     %d\n", p->velocity);
    }
    return OK;
  }

  static int vstnote_perf(CSOUND *csound, void *data)
  {
    VSTNOTEOUT *p = (VSTNOTEOUT *)data;
    if (p->on) {
      if (getCurrentTime(csound) >= p->offTime || p->h.insdshead->relesing) {
        // The note may be scheduled to turn off
        // some frames after the actual start of this kperiod.
        double deltaTime = p->offTime - getCurrentTime(csound);
        int deltaFrames = 0;
        if (deltaTime > 0) {
          deltaFrames = int(deltaTime / csound->GetSr(csound));
        }
        p->vstPlugin->AddMIDI(128 | p->channel | (p->key << 8) | (0 << 16),
                              deltaFrames, 0);
        p->on = false;
        if (csound->GetDebug(csound)) {
          csound->Message(csound, "vstnote_perf:      csound time:  %f\n",
                          getCurrentTime(csound));
          csound->Message(csound, "                   off time:     %f\n",
                          p->offTime);
          csound->Message(csound, "                   delta time:   %f\n",
                          deltaTime);
          csound->Message(csound, "                   delta frames: %d\n",
                          deltaFrames);
          csound->Message(csound, "                   channel:      %d\n",
                          p->channel);
          csound->Message(csound, "                   key:          %d\n",
                          p->key);
        }
      }
    }
    return OK;
  }

  static OENTRY localops[] = {
    { "vstinit",      sizeof(VSTINIT),    0, 1, "i", "To", &vstinit, 0, 0 },
    { "vstinfo",      sizeof(VSTINFO),    0, 1, "", "i", &vstinfo, 0, 0 },
    { "vstaudio",     sizeof(VSTAUDIO),   0, 5, "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
                                             "iy", &vstaudio_init, 0, &vstaudio },
    { "vstaudiog",    sizeof(VSTAUDIO),   0, 5, "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
                                             "iy", &vstaudio_init, 0, &vstaudiog },
    { "vstmidiout",   sizeof(VSTMIDIOUT), 0, 3, "", "ikkkk", &vstmidiout_init,
                                                                  &vstmidiout, 0 },
    { "vstparamget",  sizeof(VSTPARAMGET),0, 3, "k", "ik", &vstparamget_init,
                                                                 &vstparamget, 0 },
    { "vstparamset",  sizeof(VSTPARAMSET),0, 3, "", "ikk", &vstparamset_init,
                                                                 &vstparamset, 0 },
    { "vstbankload",  sizeof(VSTBANKLOAD),0, 1, "", "iT", &vstbankload, 0, 0 },
    { "vstprogset",   sizeof(VSTPROGSET), 0, 1, "", "ii", &vstprogset, 0, 0 },
    { "vstedit",      sizeof(VSTEDIT),    0, 1, "", "i", &vstedit_init, 0, 0 },
    { "vsttempo",     sizeof(VSTTEMPO),   0, 2, "", "ki", 0,
                                             &vstSetTempo, 0/*, &vstedit_deinit*/},
    { "vstnote",      sizeof(VSTNOTEOUT), 0, 3, "", "iiiii", &vstnote_init, &vstnote_perf, 0 },
    { "vstbanksave",  sizeof(VSTBANKLOAD),0, 1, "", "iT",    &vstbanksave,  0,             0 },
    { 0,              0,                   0, 0, 0,  0,      (SUBR) 0,     (SUBR) 0, (SUBR) 0 }
  };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    OENTRY  *ep = (OENTRY *) &(localops[0]);
    int     err = 0;
    while (ep->opname != NULL) {
      err |= csound->AppendOpcode(csound,
                                  ep->opname,
                                  ep->dsblksiz,
                                  ep->flags,
                                  ep->thread,
                                  ep->outypes,
                                  ep->intypes,
                                  (int (*)(CSOUND *, void *)) ep->iopadr,
                                  (int (*)(CSOUND *, void *)) ep->kopadr,
                                  (int (*)(CSOUND *, void *)) ep->aopadr);
      ep++;
    }
    return err;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    for (size_t i = 0, n = vstPlugins().size(); i < n; ++i) {
        if (vstPlugins()[i]) {
            delete vstPlugins()[i];
        }
        vstPlugins().clear();
    }
    return 0;
  }
}  // extern "C"
