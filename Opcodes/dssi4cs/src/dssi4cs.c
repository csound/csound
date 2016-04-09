/*
 *  Copyright (C) 2005 Andres Cabrera
 *
 *  The dssi4cs library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  The dssi4cs library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with The dssi4cs library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *
 * Uses code by Richard W.E. Furse from the ladspa sdk
 */

#include "utils.h"
#include "dssi4cs.h"
#include <dlfcn.h>
#include <dirent.h>

#undef CS_KSMPS
#define CS_KSMPS     (csound->GetKsmps(csound))

#ifdef BETA
#define DEBUG 1
#endif
#define DSSI4CS_MAX_NUM_EVENTS 128

#if !defined(HAVE_STRLCAT) && !defined(strlcat)
size_t
strlcat(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (n-- != 0 && *d != '\0')
      d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
      return (dlen + strlen(s));
    while (*s != '\0') {
      if (n != 1) {
        *d++ = *s;
        n--;
      }
      s++;
    }
    *d = '\0';

    return (dlen + (s - src));  /* count does not include NUL */
}
#endif

//static const char   *version = "0.1alpha";

/* TODO accomodate plugins which return control outputs */

/****************************************************************************
           dssiinit
*****************************************************************************/
void info(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin_)
{
    int     Ksmps = csound->GetKsmps(csound);
    unsigned long PortCount;
    LADSPA_Descriptor *Descriptor;
    uint32 i;

    if (DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    PortCount = Descriptor->PortCount;

    csound->Message(csound, "============Plugin %i"
                            "========================================\n",
                            DSSIPlugin_->PluginNumber);
    csound->Message(csound, "Plugin Type: %s\n",
                    ((DSSIPlugin_->Type == LADSPA) ? "LADSPA" : "DSSI"));
    csound->Message(csound, "Plugin UniqueID: %lu\n", Descriptor->UniqueID);
    csound->Message(csound, "Label: %s\n", Descriptor->Label);
    csound->Message(csound, "Name: %s\n", Descriptor->Name);
    csound->Message(csound, "Maker: %s\n", Descriptor->Maker);
    csound->Message(csound, "Copyright: %s\n", Descriptor->Copyright);
    csound->Message(csound, "Number of Ports: %lu\n", PortCount);
    for (i = 0; i < PortCount; i++) {
      csound->Message(csound, "  Port #%u: %s %s: %s - Range: ", i,
                      (LADSPA_IS_PORT_CONTROL(Descriptor->PortDescriptors[i]) ?
                       "Control" : "Audio"),
                      (LADSPA_IS_PORT_INPUT(Descriptor->PortDescriptors[i]) ?
                       "Input" : "Output"), Descriptor->PortNames[i]);
      if (LADSPA_IS_HINT_TOGGLED(Descriptor->PortRangeHints[i].HintDescriptor))
        csound->Message(csound, "Toggle.\n");
      else {
        if (LADSPA_IS_HINT_BOUNDED_BELOW
            (Descriptor->PortRangeHints[i].HintDescriptor))
          csound->Message(csound, "%f",
                          (Descriptor->PortRangeHints[i].LowerBound) *
                          (LADSPA_IS_HINT_SAMPLE_RATE
                           (Descriptor->PortRangeHints[i].
                            HintDescriptor) ? Ksmps : 1));
        else
          csound->Message(csound, "-Inf");
        if (LADSPA_IS_HINT_BOUNDED_ABOVE
            (Descriptor->PortRangeHints[i].HintDescriptor))
          csound->Message(csound, " -> %f\n",
                          Descriptor->PortRangeHints[i].UpperBound *
                          (LADSPA_IS_HINT_SAMPLE_RATE
                           (Descriptor->PortRangeHints[i].
                            HintDescriptor) ? Ksmps : 1));
        else
          csound->Message(csound, " -> +Inf\n");
        if (DSSIPlugin_->Type == DSSI)
          if ((LADSPA_IS_PORT_CONTROL(Descriptor->PortDescriptors[i])) &&
              (LADSPA_IS_PORT_INPUT(Descriptor->PortDescriptors[i])))
            csound->Message(csound, "        MIDI cc: %i\n",
                            DSSIPlugin_->DSSIDescriptor->
                            get_midi_controller_for_port(DSSIPlugin_->Handle,
                                                         i));
      }
    }
    csound->Message(csound, "Must run in realtime: %s\n",
                    (LADSPA_IS_REALTIME(Descriptor->Properties) ? "YES" :
                     "NO"));
    csound->Message(csound, "Is hard realtime capable: %s\n",
                    (LADSPA_IS_HARD_RT_CAPABLE(Descriptor->Properties) ? "YES" :
                     "NO"));
    csound->Message(csound, "Has activate() function: %s\n",
                    ((Descriptor->activate) ? "YES" : "NO"));
    csound->Message(csound, "=============================="
                            "===============================\n");
}

DSSI4CS_PLUGIN *LocatePlugin(int PluginNumber, CSOUND * csound)
{
    DSSI4CS_PLUGIN *DSSIPlugin_;
    DSSI4CS_PLUGIN *DSSIPlugin =
        (DSSI4CS_PLUGIN *) csound->QueryGlobalVariable(csound, "$DSSI4CS");

    if (!DSSIPlugin)
      return NULL;
    DSSIPlugin_ = DSSIPlugin;
    if (UNLIKELY(PluginNumber > *DSSIPlugin->PluginCount)) {
#ifdef DEBUG
      csound->Message(csound,
                      "DSSI4CS: PluginNumber > *DSSIPlugin->PluginCount.\n");
#endif

      return NULL;
    }
    while (DSSIPlugin_->PluginNumber != PluginNumber) {
#ifdef DEBUG
      csound->Message(csound, "DSSI4CS: Scanning plugin: %i.\n",
                      DSSIPlugin_->PluginNumber);
#endif

      if (UNLIKELY(!DSSIPlugin_->NextPlugin)) {
        return NULL;
      }
      DSSIPlugin_ = DSSIPlugin_->NextPlugin;
    }
#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: Plugin %i Located.\n",
                    DSSIPlugin_->PluginNumber);
#endif

    return DSSIPlugin_;
}

static int dssideinit(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin)
{
    int     i;

    for (i = 0; DSSIPlugin != NULL; i++) {
      DSSI4CS_PLUGIN *nxt = (DSSI4CS_PLUGIN *) DSSIPlugin->NextPlugin;
      if (DSSIPlugin->Descriptor) {
        if (DSSIPlugin->Type == LADSPA) {
          if (DSSIPlugin->Descriptor->deactivate != NULL)
            DSSIPlugin->Descriptor->deactivate(DSSIPlugin->Handle);
          if (DSSIPlugin->Descriptor->cleanup != NULL)
            DSSIPlugin->Descriptor->cleanup(DSSIPlugin->Handle);
        }
        else {
          if (DSSIPlugin->DSSIDescriptor->LADSPA_Plugin->deactivate != NULL)
            DSSIPlugin->DSSIDescriptor->LADSPA_Plugin->
              deactivate(DSSIPlugin->Handle);
          if (DSSIPlugin->DSSIDescriptor->LADSPA_Plugin->cleanup != NULL)
            DSSIPlugin->DSSIDescriptor->LADSPA_Plugin->cleanup(DSSIPlugin->Handle);
        }
      }
      else csound->Message(csound, "missing descriptor\n");
      if (i != 0)
        csound->Free(csound, DSSIPlugin);
      DSSIPlugin = nxt;
    }
    csound->DestroyGlobalVariable(csound, "$DSSI4CS");
#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: Deinit OK.\n");
#endif

    return OK;
}

int dssiinit(CSOUND * csound, DSSIINIT * p)
{
    /* TODO check if plugin has already been loaded and use same function */
    csound = p->h.insdshead->csound;
    int     SampleRate = (int) MYFLT2LRND(csound->GetSr(csound));
    int     Ksmps = csound->GetKsmps(csound);
    unsigned long     i;
    LADSPA_Descriptor_Function pfDescriptorFunction;
    DSSI_Descriptor_Function pfDSSIDescriptorFunction;
    LADSPA_Descriptor *LDescriptor;
    char    dssiFilename[MAXNAME];
    unsigned long PluginIndex;
    void   *PluginLibrary;
    unsigned long PortCount;
    unsigned long ConnectedControlPorts = 0;
    unsigned long ConnectedAudioPorts = 0;
    LADSPA_PortDescriptor PortDescriptor;
    DSSI4CS_PLUGIN *DSSIPlugin_;
    DSSI4CS_PLUGIN *DSSIPlugin =
        (DSSI4CS_PLUGIN *) csound->QueryGlobalVariable(csound, "$DSSI4CS");
    CS_TYPE* argType = csound->GetTypeForArg(p->iplugin);

    if(strcmp("S", argType->varTypeName) == 0)
      strncpy(dssiFilename,((STRINGDAT *)p->iplugin)->data, MAXNAME-1);
    else
      csound->strarg2name(csound, dssiFilename, ISSTRCOD(*p->iplugin) ?
                          csound->GetString(csound, *p->iplugin) :
                          (char *) p->iplugin, "dssiinit.",
                          (int) ISSTRCOD(*p->iplugin));
    PluginIndex = (unsigned long) *p->iindex;
    PluginLibrary = dlopenLADSPA(csound, dssiFilename, RTLD_NOW);
    if (UNLIKELY(!PluginLibrary))
      return csound->InitError(csound, "DSSI4CS: Failed to load %s.",
                                       dssiFilename);
    if (!DSSIPlugin) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "$DSSI4CS",
                                                sizeof(DSSI4CS_PLUGIN))) != 0)
        csound->Die(csound, "Error creating global variable '$DSSI4CS'");
      DSSIPlugin = (DSSI4CS_PLUGIN *) csound->QueryGlobalVariable(csound,
                                                                  "$DSSI4CS");
      csound->RegisterResetCallback(csound, DSSIPlugin,
                                    (int (*)(CSOUND*, void*)) dssideinit);
      DSSIPlugin->PluginNumber = 0;
      DSSIPlugin->PluginCount = (int *) csound->Malloc(csound, sizeof(int));
      *DSSIPlugin->PluginCount = 1;
      DSSIPlugin_ = DSSIPlugin;
      if (p->iverbose != 0) {
        csound->Message(csound, "DSSI4CS: Loading first instance.\n");
      }
    }
    else {
      DSSIPlugin_ = LocatePlugin(*DSSIPlugin->PluginCount - 1, csound);
      if (p->iverbose != 0) {
        csound->Message(csound, "DSSI4CS: Located plugin: %i.\n",
                        DSSIPlugin_->PluginNumber);
      }
      DSSIPlugin_->NextPlugin =
          (DSSI4CS_PLUGIN *) csound->Malloc(csound, sizeof(DSSI4CS_PLUGIN));
      DSSIPlugin_ = DSSIPlugin_->NextPlugin;
      DSSIPlugin_->PluginNumber = *DSSIPlugin->PluginCount;
      DSSIPlugin_->PluginCount = DSSIPlugin->PluginCount;
      *DSSIPlugin_->PluginCount = (*DSSIPlugin_->PluginCount) + 1;
    }
    *p->iDSSIHandle = DSSIPlugin_->PluginNumber;
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: About to load descriptor function "
                      "for plugin %i of %i.\n",
                      DSSIPlugin_->PluginNumber, *DSSIPlugin_->PluginCount);
    }
    pfDSSIDescriptorFunction =
        (DSSI_Descriptor_Function) dlsym(PluginLibrary, "dssi_descriptor");
    if (pfDSSIDescriptorFunction) {
      DSSIPlugin_->DSSIDescriptor =
          (DSSI_Descriptor *) csound->Calloc(csound, sizeof(DSSI_Descriptor));
      DSSIPlugin_->DSSIDescriptor =
          (DSSI_Descriptor *) pfDSSIDescriptorFunction(PluginIndex);
      LDescriptor =
          (LADSPA_Descriptor *) DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
      DSSIPlugin_->Type = DSSI;
      if (p->iverbose != 0) {
        csound->Message(csound, "DSSI4CS: DSSI Plugin detected.\n");
      }
    }
    else {
      pfDescriptorFunction =
          (LADSPA_Descriptor_Function) dlsym(PluginLibrary,
                                             "ladspa_descriptor");
      if (pfDescriptorFunction==NULL) {
        return csound->InitError(csound, "No lapspa descriptor\n");
      }
      DSSIPlugin_->Descriptor =
          (LADSPA_Descriptor *) csound->Calloc(csound,
                                               sizeof(LADSPA_Descriptor));
      DSSIPlugin_->Descriptor =
          (LADSPA_Descriptor *) pfDescriptorFunction(PluginIndex);
      LDescriptor = (LADSPA_Descriptor *) DSSIPlugin_->Descriptor;
      DSSIPlugin_->Type = LADSPA;
      if (p->iverbose != 0) {
        csound->Message(csound, "DSSI4CS: LADSPA Plugin Detected\n");
      }
    }
    if (UNLIKELY((!DSSIPlugin_->Descriptor) &&
                 (!DSSIPlugin_->DSSIDescriptor))) {
      const char *pcError = dlerror();

      /* TODO: cleanup if error */
      /* csound->Free(csound, DSSIPlugin_->Descriptor); */
      if (pcError)
        return
          csound->InitError(csound, "DSSI4CS: Unable to find "
                            "ladspa_descriptor() function or\n"
                            "dssi_descriptor() function in plugin file "
                            "\"%s\": %s.\n"
                            "Are you sure this is a LADSPA or "
                            "DSSI plugin file ?",
                            dssiFilename, pcError);
      else
        return
          csound->InitError(csound, "DSSI4CS: Unable to find "
                            "ladspa_descriptor() function or\n"
                            "dssi_descriptor() function in plugin file "
                            "\"%s\".\n"
                            "Are you sure this is a LADSPA or "
                            "DSSI plugin file ?",
                            dssiFilename);
    }
    if (UNLIKELY(!LDescriptor)) {
      return csound->InitError(csound, "DSSI4CS: No plugin index %lu in %s",
                               PluginIndex, dssiFilename);
    }
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: About to instantiate plugin.\n");
    }

    if (DSSIPlugin_->Type == LADSPA) {
      if (UNLIKELY(!
          (DSSIPlugin_->Handle =
           (LADSPA_Handle) DSSIPlugin_->Descriptor->instantiate(DSSIPlugin_->
                                                                Descriptor,
                                                                SampleRate))))
        return csound->InitError(csound,
                                 "DSSI4CS: Could not instantiate plugin: %s",
                                 dssiFilename);
      if (UNLIKELY(!DSSIPlugin_->Descriptor->run))
        return csound->InitError(csound, "DSSI4CS: No run() funtion in: %s",
                                         LDescriptor->Name);
      PortCount = DSSIPlugin_->Descriptor->PortCount;
    }
    else {
      if (UNLIKELY(!
          (DSSIPlugin_->Handle =
           (LADSPA_Handle) DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->
           instantiate(DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin, SampleRate))))
        return csound->InitError(csound,
                                 "DSSI4CS: Could not instantiate plugin: %s",
                                 dssiFilename);
      if (UNLIKELY(!DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->run))
        return csound->InitError(csound, "DSSI4CS: No run() funtion in: %s",
                                         LDescriptor->Name);
      PortCount = DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->PortCount;
      DSSIPlugin_->Events =
          (snd_seq_event_t *) csound->Calloc(csound,
                                             DSSI4CS_MAX_NUM_EVENTS
                                             * sizeof(snd_seq_event_t));
    }
    if (p->iverbose != 0) {
      if (DSSIPlugin_->Handle)
        csound->Message(csound, "DSSI4CS: Plugin instantiated.\n");
      else
        csound->Message(csound, "DSSI4CS: Problem instantiating.\n");
    }

    for (i = 0; i < PortCount; i++) {
      PortDescriptor =
          (DSSIPlugin_->Type ==
           LADSPA ? DSSIPlugin_->Descriptor->PortDescriptors[i]
           : DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->PortDescriptors[i]);
      if (LADSPA_IS_PORT_CONTROL(PortDescriptor))
        ConnectedControlPorts++;

      if (LADSPA_IS_PORT_AUDIO(PortDescriptor))
        ConnectedAudioPorts++;
    }
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: Found %lu control ports for: '%s'\n",
                      ConnectedControlPorts, LDescriptor->Name);
      csound->Message(csound, "DSSI4CS: Found %lu audio ports for: '%s'\n",
                      ConnectedAudioPorts, LDescriptor->Name);
    }

    DSSIPlugin_->control =
        (LADSPA_Data **) csound->Calloc(csound, ConnectedControlPorts
                                                * sizeof(LADSPA_Data *));
    DSSIPlugin_->audio =
        (LADSPA_Data **) csound->Calloc(csound, ConnectedAudioPorts
                                                * sizeof(LADSPA_Data *));
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: Created port array.\n");
    }

    ConnectedControlPorts = 0;
    ConnectedAudioPorts = 0;
    for (i = 0; i < PortCount; i++) {
      PortDescriptor =
          (DSSIPlugin_->Type ==
           LADSPA ? DSSIPlugin_->Descriptor->PortDescriptors[i]
           : DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->PortDescriptors[i]);
      if (p->iverbose != 0) {
        csound->Message(csound, "DSSI4CS: Queried port descriptor.\n");
      }

      if (LADSPA_IS_PORT_CONTROL(PortDescriptor)) {
        DSSIPlugin_->control[ConnectedControlPorts] =
            (LADSPA_Data *) csound->Calloc(csound, sizeof(LADSPA_Data));
        if (DSSIPlugin_->Type == LADSPA) {
          DSSIPlugin_->Descriptor->connect_port(DSSIPlugin_->Handle,
                                                i,
                                                (LADSPA_Data *) DSSIPlugin_->
                                                control[ConnectedControlPorts]);
        }
        else {
          DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->connect_port(
              DSSIPlugin_->Handle, i,
              (LADSPA_Data *) DSSIPlugin_->control[ConnectedControlPorts]);
        }
        if (p->iverbose != 0) {
          csound->Message(csound,
                          "DSSI4CS: Created internal control port "
                          "%lu for Port %lu.\n",
                          ConnectedControlPorts, i);
        }

        ConnectedControlPorts++;
      }
      if (LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
        DSSIPlugin_->audio[ConnectedAudioPorts] =
            (LADSPA_Data *) csound->Calloc(csound, Ksmps * sizeof(LADSPA_Data));
        if (DSSIPlugin_->Type == LADSPA)
          DSSIPlugin_->Descriptor->connect_port(DSSIPlugin_->Handle,
                                                i,
                                                (LADSPA_Data *) DSSIPlugin_->
                                                audio[ConnectedAudioPorts]);
        else
          DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->connect_port(
              DSSIPlugin_->Handle, i,
              (LADSPA_Data *) DSSIPlugin_->audio[ConnectedAudioPorts]);
        if (p->iverbose != 0) {
          csound->Message(csound,
                          "DSSI4CS: Created internal audio port"
                          " %lu for Port %lu.\n",
                          ConnectedAudioPorts, i);
        }

        ConnectedAudioPorts++;
      }

    }
    /* All ports must be connected before calling run() */
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: Created %lu control ports for: '%s'\n",
                      ConnectedControlPorts, LDescriptor->Name);
      csound->Message(csound, "DSSI4CS: Created %lu audio ports for: '%s'\n",
                      ConnectedAudioPorts, LDescriptor->Name);
    }

    DSSIPlugin_->Active = 0;
    DSSIPlugin_->EventCount = 0;
    if (p->iverbose != 0) {
      csound->Message(csound, "DSSI4CS: Init Done.\n");
      if (*p->iverbose != 0)
        info(csound, DSSIPlugin_);
    }
    dlclose(PluginLibrary);
    return OK;
}

/****************************************************************************
           dssiactivate
*****************************************************************************/

int ActivatePlugin(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin_, int ktrigger)
{
    const LADSPA_Descriptor *Descriptor;

    if (!DSSIPlugin_)
      return -100;

    if (DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;

    if (Descriptor->activate != NULL) {

      if ((ktrigger == 1) && (DSSIPlugin_->Active == 0)) {
        Descriptor->activate(DSSIPlugin_->Handle);
        DSSIPlugin_->Active = 1;
        return 1;
      }
      if ((ktrigger == 0) && (DSSIPlugin_->Active == 1)) {
        DSSIPlugin_->Active = 0;
        if (Descriptor->deactivate != NULL) {
          Descriptor->deactivate(DSSIPlugin_->Handle);
          return 0;
        }
        return -2;
      }
      return 100;

    }
    else {
      if ((ktrigger == 1) && (DSSIPlugin_->Active == 0)) {
        DSSIPlugin_->Active = 1;
        return -1;
      }
      if ((ktrigger == 0) && (DSSIPlugin_->Active == 1)) {
        DSSIPlugin_->Active = 0;
        return -2;
      }
    }
    return -200;
}

int dssiactivate_init(CSOUND * csound, DSSIACTIVATE * p)
{
    int     Number = *p->iDSSIhandle;

#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: activate-Locating plugin %i\n", Number);
#endif

    p->DSSIPlugin_ = LocatePlugin(Number, csound);
    p->printflag = -999;
    if (UNLIKELY((!p->DSSIPlugin_) || (Number > *p->DSSIPlugin_->PluginCount) ||
                 (!p->DSSIPlugin_->Handle)))
      return csound->InitError(csound, "DSSI4CS: Invalid plugin: %i (MAX= %i).",
                                       Number, *p->DSSIPlugin_->PluginCount);
#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: activate-Finished locating plugin %i\n",
                    p->DSSIPlugin_->PluginNumber);
#endif

    return OK;
}

int dssiactivate(CSOUND * csound, DSSIACTIVATE * p)
{
    LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    int     val = ActivatePlugin(csound, p->DSSIPlugin_, *p->ktrigger);

    switch (val) {
    case -1:
      {
        if (p->printflag != -1) {
          csound->Message(csound,
                          "DSSI4CS: '%s' activated (No activate function).\n",
                          Descriptor->Name);
          p->printflag = -1;
        }
      }
      break;
    case 0:
      {
        if (p->printflag != 0) {
          csound->Message(csound,
                          "DSSI4CS: Deactivate function called for: %s\n",
                          Descriptor->Name);
          p->printflag = 0;
        }
      }
      break;
    case 1:
      {
        if (p->printflag != 1) {
          csound->Message(csound, "DSSI4CS: Activate function called for: %s\n",
                          Descriptor->Name);
          p->printflag = 1;
        }
      }
      break;
    case -2:
      {
        if (p->printflag != -2) {
          csound->Message(csound, "DSSI4CS: '%s' deactivated "
                                  "(No deactivate function).\n",
                                  Descriptor->Name);
          p->printflag = -2;
        }
      }
      break;
    case -100:
      {
        if (p->printflag != -100)
          return csound->PerfError(csound, p->h.insdshead,
                                   "DSSI4CS: dssiactivate "
                                   "not properly initialised.");
        p->printflag = -100;
      }
      break;

    default:
      break;
    }
    return OK;
}

/****************************************************************************
           dssiaudio
*****************************************************************************/
int dssiaudio_init(CSOUND * csound, DSSIAUDIO * p)
{
    /* TODO not realtime safe, try to make it so. */
    int     Number = *p->iDSSIhandle;
    int     icnt = csound->GetInputArgCnt(p) - 1;
    int     ocnt = csound->GetOutputArgCnt(p);

    if (UNLIKELY(icnt > DSSI4CS_MAX_IN_CHANNELS))
      csound->Die(csound,
                  Str("DSSI4CS: number of audio input channels "
                      "is greater than %d"),
                  DSSI4CS_MAX_IN_CHANNELS);

    if (UNLIKELY(ocnt > DSSI4CS_MAX_OUT_CHANNELS))
      csound->Die(csound,
                  Str("DSSI4CS: number of audio output channels "
                      "is greater than %d"),
                  DSSI4CS_MAX_OUT_CHANNELS);

#ifdef DEBUG
    csound->Message(csound,
                    "DSSI4CS: dssiaudio- %i input args, %i output args.\n",
                    csound->GetInputArgCnt(p), csound->GetOutputArgCnt(p));
    csound->Message(csound, "DSSI4CS: dssiaudio LocatePlugin # %i\n", Number);
#endif

    p->DSSIPlugin_ = LocatePlugin(Number, csound);

    if (!p->DSSIPlugin_)
      return csound->InitError(csound,
                               "DSSI4CS: dssiaudio: Invalid plugin handle.");
    const LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;

    unsigned long PortIndex = 0;
    int           ConnectedInputPorts = 0;
    int           ConnectedOutputPorts = 0;
    int           ConnectedPorts = 0;
    LADSPA_PortDescriptor PortDescriptor = 0;

    for (PortIndex = 0; PortIndex < Descriptor->PortCount; PortIndex++) {
      PortDescriptor = Descriptor->PortDescriptors[PortIndex];
      if (LADSPA_IS_PORT_INPUT(PortDescriptor)
          && LADSPA_IS_PORT_AUDIO(PortDescriptor))
        ConnectedInputPorts++;
      else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor)
               && LADSPA_IS_PORT_AUDIO(PortDescriptor))
        ConnectedOutputPorts++;
    }
    p->InputPorts = (unsigned long *)
        csound->Calloc(csound, ConnectedInputPorts * sizeof(unsigned long));
    p->OutputPorts = (unsigned long *)
        csound->Calloc(csound, ConnectedOutputPorts * sizeof(unsigned long));
    ConnectedInputPorts = 0;
    ConnectedOutputPorts = 0;
    ConnectedPorts = 0;
    for (PortIndex = 0; PortIndex < Descriptor->PortCount; PortIndex++) {
      PortDescriptor = Descriptor->PortDescriptors[PortIndex];
      if (LADSPA_IS_PORT_INPUT(PortDescriptor)
          && LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
        p->InputPorts[ConnectedInputPorts] = ConnectedPorts;
#ifdef DEBUG
        csound->Message(csound, "DSSI4CS: Connected Audio port: "
                                "%lu to Input port : %li\n",
                        p->InputPorts[ConnectedInputPorts], PortIndex);
#endif

        ConnectedInputPorts++;
        ConnectedPorts++;
      }
      else if (LADSPA_IS_PORT_OUTPUT(PortDescriptor)
               && LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
        p->OutputPorts[ConnectedOutputPorts] = ConnectedPorts;
#ifdef DEBUG
        csound->Message(csound, "DSSI4CS: Connected Audio Port: "
                                "%lu to Output port: %li\n",
                        p->OutputPorts[ConnectedOutputPorts], PortIndex);
#endif

        ConnectedOutputPorts++;
        ConnectedPorts++;
      }

    }
#ifdef DEBUG
    csound->Message(csound,
                    "DSSI4CS: Connected %i audio input ports for: '%s'\n",
                    ConnectedInputPorts, Descriptor->Name);
    csound->Message(csound,
                    "DSSI4CS: Connected %i audio output ports for: '%s'\n",
                    ConnectedOutputPorts, Descriptor->Name);
#endif

    p->NumInputPorts = ConnectedInputPorts;
    p->NumOutputPorts = ConnectedOutputPorts;
    if ((p->NumInputPorts) < icnt) {
      if (p->NumInputPorts == 0)
        csound->Message(csound, "DSSI4CS: Plugin '%s' has %i audio input ports "
                                "audio input discarded.\n",
                                Descriptor->Name, p->NumInputPorts);
      else
        return csound->InitError(csound, "DSSI4CS: Plugin '%s' "
                                         "has %i audio input ports.",
                                         Descriptor->Name, p->NumOutputPorts);
    }
    if (p->NumOutputPorts < ocnt)
      return csound->InitError(csound, "DSSI4CS: Plugin '%s' "
                                       "has %i audio output ports.",
                                       Descriptor->Name, p->NumOutputPorts);
    return OK;
}

int dssiaudio(CSOUND * csound, DSSIAUDIO * p)
{
    const LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    unsigned int i, j;
    unsigned int icnt = csound->GetInputArgCnt(p) - 1;
    unsigned int ocnt = csound->GetOutputArgCnt(p);
    unsigned long Ksmps = (unsigned long) csound->GetKsmps(csound);

    if (p->DSSIPlugin_->Active == 1) {
      for (j = 0; j < icnt; j++) {
        for (i = 0; i < Ksmps; i++)
          p->DSSIPlugin_->audio[p->InputPorts[j]][i] =
            p->ain[j][i] * (1.0/csound->Get0dBFS(csound));
      }
      Descriptor->run(p->DSSIPlugin_->Handle, Ksmps);
      for (j = 0; j < ocnt; j++) {
        for (i = 0; i < Ksmps; i++)
          p->aout[j][i] =
            p->DSSIPlugin_->audio[p->OutputPorts[j]][i] * csound->Get0dBFS(csound);
      }
    }
    else {
      for (j = 0; j < ocnt; j++)
        for (i = 0; i < Ksmps; i++)
          p->aout[j][i] = FL(0.0);
    }

    return OK;
}

/****************************************************************************
           dssictls
*****************************************************************************/
int dssictls_kk(CSOUND * csound, DSSICTLS * p)
{
    LADSPA_Data Value = *p->val;
    if (!p->DSSIPlugin_) {
      return csound->PerfError(csound, p->h.insdshead,
                               "DSSI4CS: Invalid plugin handle.");
    }
    if (*p->ktrig == 1) {
      *p->DSSIPlugin_->control[p->PortNumber] = Value * (p->HintSampleRate);
    }
    return OK;
}

int dssictls_ak(CSOUND * csound, DSSICTLS * p)
{
    csound->PerfError(csound, p->h.insdshead,
                      "DSSI4CS: Audio Rate control ports not implemented yet.");
    return NOTOK;
}

int dssictls_init(CSOUND * csound, DSSICTLS * p)
{
    /* TODO warning possible crash or unpredictable behaviour if invalid port.
            Crash if audio port selected */
    const LADSPA_Descriptor *Descriptor;
    int     Number = *p->iDSSIhandle;
    int     Sr = (int) MYFLT2LRND(csound->GetSr(csound));
    unsigned long PortIndex = *p->iport;
    unsigned int  i;
    unsigned long ControlPort = 0;
    unsigned long AudioPort = 0;
    unsigned long Port = 0;

    p->DSSIPlugin_ = LocatePlugin(Number, csound);
    if (!p->DSSIPlugin_) {
      return csound->InitError(csound, "DSSI4CS: Invalid plugin handle.");
    }
    if (p->DSSIPlugin_->Type == LADSPA) {
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    }
    else {
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    }
    p->HintSampleRate =
        (LADSPA_IS_HINT_SAMPLE_RATE
         (Descriptor->PortRangeHints[PortIndex].HintDescriptor) ? Sr : 1);
#ifdef DEBUG
    csound->Message(csound,
                    "DSSI4CS: Port %lu multiplier (HintSampleRate): %i.\n",
                    PortIndex, p->HintSampleRate);
#endif

    if (PortIndex > Descriptor->PortCount) {
      csound->InitError(csound, "DSSI4CS: Port %lu from '%s' does not exist.",
                                PortIndex, Descriptor->Name);
      return NOTOK;
    }
    LADSPA_PortDescriptor PortDescriptor =
        Descriptor->PortDescriptors[PortIndex];
    if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
      return csound->InitError(csound,
                               "DSSI4CS: Port %lu from '%s' is an output port.",
                               PortIndex, Descriptor->Name);
    for (i = 0; i < PortIndex; i++) {
      PortDescriptor = Descriptor->PortDescriptors[i];
      if (LADSPA_IS_PORT_CONTROL(PortDescriptor)) {
        ControlPort++;
        Port = ControlPort;
      }
      if (LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
        AudioPort++;
        Port = AudioPort;
      }
    }
    p->PortNumber = Port;
#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: Port %lu using internal port %lu.\n",
                    PortIndex, p->PortNumber);
    /*csound->Message(csound, "DSSI4CS: ArgMask: %lu.\n",*/
    /*                csound->GetInputArgAMask(p));*/
#endif

//    if ((int) csound->GetInputArgAMask(p) & 4) {
//      p->h.opadr = (SUBR) dssictls_ak;  /* "iiak" */
//    }
//    else {
//      p->h.opadr = (SUBR) dssictls_kk;  /* "iikk" */
//    }

    return OK;
}

int dssictls_dummy(CSOUND * csound, DSSICTLS * p)
{
    csound->PerfError(csound, p->h.insdshead,
                      Str("DSSI4CS: Not initialised or wrong argument types."));
    return NOTOK;
}

/****************************************************************************
           dssisynth
*****************************************************************************/

int dssisynth_init(CSOUND * csound, DSSISYNTH * p)
{
    /* TODO docs: dssisynth only for DSSI plugs */
    csound->InitError(csound, "DSSI4CS: dssisynth not implemented yet.");
    return NOTOK;
}

int dssisynth(CSOUND * csound, DSSISYNTH * p)
{
    return OK;
}

/****************************************************************************
           dssinotes
*****************************************************************************/
int dssinote_init(CSOUND * csound, DSSINOTE * p)
{
    csound->InitError(csound, "DSSI4CS: dssinote not implemented yet.");
    return NOTOK;
}

int dssinote(CSOUND * csound, DSSINOTE * p)
{
    return OK;
}

int dssievent_init(CSOUND * csound, DSSINOTEON * p)
{
    csound->InitError(csound, "DSSI4CS: dssievent not implemented yet.");
    return NOTOK;
}

int dssievent(CSOUND * csound, DSSINOTEON * p)
{
    return OK;
}

/****************************************************************************
           dssilist
*****************************************************************************/

static void
      LADSPADirectoryPluginSearch
      (CSOUND * csound,
       const char * pcDirectory,
       LADSPAPluginSearchCallbackFunction fCallbackFunction) {
    char   *pcFilename;
    DIR    *psDirectory;
    LADSPA_Descriptor_Function fDescriptorFunction;
    long    lDirLength;
    long    slen;
    long    iNeedSlash;
    struct dirent *psDirectoryEntry;
    void   *pvPluginHandle;

    lDirLength = strlen(pcDirectory);
    if (!lDirLength)
      return;
    if (pcDirectory[lDirLength - 1] == '/')
      iNeedSlash = 0;
    else
      iNeedSlash = 1;

    psDirectory = opendir(pcDirectory);
    if (!psDirectory)
      return;

    while (1) {
      psDirectoryEntry = readdir(psDirectory);
      if (!psDirectoryEntry) {
        closedir(psDirectory);
        //if (pvPluginHandle) closedir(pvPluginHandle);
        return;
      }

      pcFilename =
        csound->Malloc(csound,
                       slen = (lDirLength + strlen(psDirectoryEntry->d_name) + 2));
      strncpy(pcFilename, pcDirectory, slen); /*pcFilename[slen-1] = '\0';*/
      if (iNeedSlash)
        strlcat(pcFilename, "/",slen);
      strlcat(pcFilename, psDirectoryEntry->d_name, slen);

      pvPluginHandle = dlopen(pcFilename, RTLD_LAZY);
      if (pvPluginHandle) {
        /* This is a file and the file is a shared library! */
        dlerror();
        fDescriptorFunction
            = (LADSPA_Descriptor_Function) dlsym(pvPluginHandle,
                                                 "ladspa_descriptor");
        if (dlerror() == NULL && fDescriptorFunction) {
          /* We've successfully found a ladspa_descriptor function. Pass
             it to the callback function. */
          fCallbackFunction(csound,
                            pcFilename, pvPluginHandle, fDescriptorFunction);
        }
        else {
          /* It was a library, but not a LADSPA one. Unload it. */
          dlclose(pvPluginHandle);
          pvPluginHandle = NULL;
          //csound->Free(csound, pcFilename);
        }
        csound->Free(csound, pcFilename);
      }
    }
}

void
LADSPAPluginSearch(CSOUND *csound,
                   LADSPAPluginSearchCallbackFunction fCallbackFunction)
{
    char   *pcBuffer;
    const char *pcEnd;
          char *pcLADSPAPath;
    const char *pcDSSIPath;
    const char *pcStart;

    pcLADSPAPath = getenv("LADSPA_PATH");
    pcDSSIPath = getenv("DSSI_PATH");
    if (!pcLADSPAPath) {
      csound->Message(csound,
                      "DSSI4CS: LADSPA_PATH environment variable not set.\n");
#ifdef LIB64
      pcLADSPAPath = "/usr/lib64/ladspa/";
#else
      pcLADSPAPath = "/usr/lib/ladspa/";
#endif
    }
    if (!pcDSSIPath) {
      csound->Message(csound,
                      "DSSI4CS: DSSI_PATH environment variable not set.\n");
      pcStart = pcLADSPAPath;
    }
    else {
      int len = strlen(pcLADSPAPath)+strlen(pcDSSIPath)+2;
      char *tmp = (char*)malloc(len);
      snprintf(tmp, len, "%s:%s", pcLADSPAPath, pcDSSIPath);
      pcLADSPAPath = pcStart = (const char*)tmp;
    }
    // Search the list
    while (*pcStart != '\0') {
      pcEnd = pcStart;
      while (*pcEnd != ':' && *pcEnd != '\0')
        pcEnd++;

      pcBuffer = csound->Malloc(csound, 1 + (pcEnd - pcStart));
      if (pcEnd > pcStart) {
        strncpy(pcBuffer, pcStart, pcEnd - pcStart);
        pcBuffer[pcEnd - pcStart] = '\0';
      }
      LADSPADirectoryPluginSearch(csound, pcBuffer, fCallbackFunction);
      csound->Free(csound, pcBuffer);

      pcStart = pcEnd;
      if (*pcStart == ':')
        pcStart++;
    }
    if (pcDSSIPath) free(pcLADSPAPath);
}

void
describePluginLibrary(CSOUND *csound,
                      const char *pcFullFilename,
                      void *pvPluginHandle,
                      LADSPA_Descriptor_Function fDescriptorFunction)
{
    const LADSPA_Descriptor *psDescriptor;
    int     lIndex;

    csound->Message(csound, "Plugin: %s:\n", pcFullFilename);
    for (lIndex = 0;
         (psDescriptor = fDescriptorFunction(lIndex)) != NULL; lIndex++)
      csound->Message(csound, "  Index: %i : %s (%lu/%s)\n",
                      lIndex,
                      psDescriptor->Name,
                      psDescriptor->UniqueID, psDescriptor->Label);

    dlclose(pvPluginHandle);
}

int dssilist(CSOUND * csound, DSSILIST * p)
{
    /* Most of this function comes from the ladspa sdk by Richard Furse */
    char   *pcBuffer;
    const char *pcEnd;
          char *pcLADSPAPath;
    const char *pcDSSIPath;
    const char *pcStart;

    pcLADSPAPath = getenv("LADSPA_PATH");
    pcDSSIPath = getenv("DSSI_PATH");
    if (!pcLADSPAPath) {
      csound->Message(csound,
                      "DSSI4CS: LADSPA_PATH environment variable not set.\n");
    }
    if (!pcDSSIPath) {
      csound->Message(csound,
                      "DSSI4CS: DSSI_PATH environment variable not set.\n");
    }
    if ((!pcLADSPAPath) && (!pcDSSIPath)) /* Fixed - JPff */
      return NOTOK;
    if (pcDSSIPath) {  /* **** THIS CODE WAS WRONG -- NO SPACEALLOCATED **** */
      if (pcLADSPAPath) {
        char *nn =
          (char*)malloc(strlen((char *) pcLADSPAPath)+strlen(pcDSSIPath)+2);
        strcpy(nn, pcLADSPAPath);
        strcat(nn, ":");
        strcat(nn, pcDSSIPath);
        //free(pcLADSPAPath);
        pcLADSPAPath = nn;
      }
      else pcLADSPAPath = strdup(pcDSSIPath);

    }
    pcStart = pcLADSPAPath;
    while (*pcStart != '\0') {
      pcEnd = pcStart;
      while (*pcEnd != ':' && *pcEnd != '\0')
        pcEnd++;
      pcBuffer = csound->Malloc(csound, 1 + (pcEnd - pcStart));
      if (pcEnd > pcStart)
        strncpy(pcBuffer, pcStart, pcEnd - pcStart);
      pcBuffer[pcEnd - pcStart] = '\0';
      LADSPADirectoryPluginSearch(csound, pcBuffer, describePluginLibrary);
      csound->Free(csound, pcBuffer);
      pcStart = pcEnd;
      if (*pcStart == ':')
        pcStart++;
    }
    if (pcDSSIPath) free(pcLADSPAPath);
    return OK;
}

static OENTRY dssi_localops[] = {
  {"dssiinit", sizeof(DSSIINIT), 0, 1, "i", "Tip", (SUBR) dssiinit, NULL, NULL }
    ,
    {"dssiactivate", sizeof(DSSIACTIVATE), 0, 3, "", "ik",
     (SUBR) dssiactivate_init, (SUBR) dssiactivate, NULL }
    ,
    {"dssiaudio", sizeof(DSSIAUDIO), 0, 5, "mmmmmmmmm", "iMMMMMMMMM",
                                            (SUBR) dssiaudio_init,
                                            NULL, (SUBR) dssiaudio }
    ,
    {"dssictls", sizeof(DSSICTLS), 0, 3, "", "iikk", (SUBR) dssictls_init,
     (SUBR) dssictls_kk, NULL }
    ,
    {"dssilist", sizeof(DSSILIST), 0, 1, "", "", (SUBR) dssilist, NULL, NULL }
#if 0
    ,
    {"dssisynth", sizeof(DSSISYNTH), 0, 5, "aa",  "i", (SUBR) dssisynth_init,
     NULL, (SUBR) dssisynth}
    ,
    {"dssinote", sizeof(DSSINOTE), 0, 3, "",  "kikkk", (SUBR) dssinote_init,
     NULL, (SUBR) dssinote}
    ,
    {"dssievent", sizeof(DSSINOTEON), 0, 3, "",  "kikk", (SUBR) dssievent_init,
     NULL, (SUBR) dssievent}
#endif
};

LINKAGE_BUILTIN(dssi_localops)
