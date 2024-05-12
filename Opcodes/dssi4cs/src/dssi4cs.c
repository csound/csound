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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *
 * Uses code by Richard W.E. Furse from the ladspa sdk
 */

#include "utils.h"
#include "dssi4cs.h"
#include <dlfcn.h>
#include <dirent.h>

#define DSSI4CS_MAX_NUM_EVENTS 128

#if !defined(HAVE_STRLCAT) && !defined(strlcat)
size_t strlcat(char *dst, const char *src, size_t siz)
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

/* Modified from BSD sources for strlcpy */
/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
/* modifed for speed -- JPff */
char *
strNcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit or until NULL */
    if (n != 0) {
      while (--n != 0) {
        if ((*d++ = *s++) == '\0')
          break;
      }
    }

    /* Not enough room in dst, add NUL */
    if (n == 0) {
      if (siz != 0)
        *d = '\0';                /* NUL-terminate dst */

      //while (*s++) ;
    }
    return dst;        /* count does not include NUL */
}


/* TODO accomodate plugins which return control outputs */

/****************************************************************************
           dssiinit
*****************************************************************************/
void info(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin_)
{
    int32_t     Ksmps = DSSIPlugin_->ksmps;
    uint64_t PortCount = 0;
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
    csound->Message(csound, "Plugin UniqueID: %lu\n",
                    (unsigned long) Descriptor->UniqueID);
    csound->Message(csound, "Label: %s\n", Descriptor->Label);
    csound->Message(csound, "Name: %s\n", Descriptor->Name);
    csound->Message(csound, "Maker: %s\n", Descriptor->Maker);
    csound->Message(csound, "Copyright: %s\n", Descriptor->Copyright);
    csound->Message(csound, "Number of Ports: %lu\n",(unsigned long) PortCount);
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

static int32_t dssideinit(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin)
{
    int32_t     i;

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

int32_t dssiinit(CSOUND * csound, DSSIINIT * p)
{
    /* TODO check if plugin has already been loaded and use same function */
    csound = p->h.insdshead->csound;
    int32_t     SampleRate = (int32_t) MYFLT2LRND(CS_ESR);
    int32_t     Ksmps = CS_KSMPS;
    uint64_t     i;
    int32_t     verbose = (int32_t)*p->iverbose;
    LADSPA_Descriptor_Function pfDescriptorFunction;
    DSSI_Descriptor_Function pfDSSIDescriptorFunction;
    LADSPA_Descriptor *LDescriptor;
    char    dssiFilename[MAXNAME];
    uint64_t PluginIndex;
    void   *PluginLibrary;
    uint64_t PortCount = 0;
    uint64_t ConnectedControlPorts = 0;
    uint64_t ConnectedAudioPorts = 0;
    LADSPA_PortDescriptor PortDescriptor;
    DSSI4CS_PLUGIN *DSSIPlugin_;
    DSSI4CS_PLUGIN *DSSIPlugin =
        (DSSI4CS_PLUGIN *) csound->QueryGlobalVariable(csound, "$DSSI4CS");
    CS_TYPE* argType = GetTypeForArg(p->iplugin);
    DSSIPlugin->ksmps = Ksmps;


    if (strcmp("S", argType->varTypeName) == 0)
      strNcpy(dssiFilename,((STRINGDAT *)p->iplugin)->data, MAXNAME);
    else
      csound->StringArg2Name(csound, dssiFilename, IsStringCode(*p->iplugin) ?
                          csound->GetString(csound, *p->iplugin) :
                          (char *) p->iplugin, "dssiinit.",
                          (int32_t) IsStringCode(*p->iplugin));
    PluginIndex = (uint64_t) *p->iindex;
    PluginLibrary = dlopenLADSPA(csound, dssiFilename, RTLD_NOW);
    if (UNLIKELY(!PluginLibrary))
      return csound->InitError(csound, Str("DSSI4CS: Failed to load %s."),
                                       dssiFilename);
    if (!DSSIPlugin) {
      if (UNLIKELY(csound->CreateGlobalVariable(csound, "$DSSI4CS",
                                                sizeof(DSSI4CS_PLUGIN))) != 0)
        csound->Die(csound, "%s", Str("Error creating global variable '$DSSI4CS'"));
      DSSIPlugin = (DSSI4CS_PLUGIN *) csound->QueryGlobalVariable(csound,
                                                                  "$DSSI4CS");
      csound->RegisterResetCallback(csound, DSSIPlugin,
                                    (int32_t (*)(CSOUND*, void*)) dssideinit);
      DSSIPlugin->PluginNumber = 0;
      DSSIPlugin->PluginCount = (int32_t *) csound->Malloc(csound, sizeof(int32_t));
      *DSSIPlugin->PluginCount = 1;
      DSSIPlugin_ = DSSIPlugin;
      if (verbose != 0) {
        csound->Message(csound, "%s", Str("DSSI4CS: Loading first instance.\n"));
      }
    }
    else {
      DSSIPlugin_ = LocatePlugin(*DSSIPlugin->PluginCount - 1, csound);
      if (verbose != 0) {
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
    if (verbose != 0) {
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
      if (verbose != 0) {
        csound->Message(csound, "DSSI4CS: DSSI Plugin detected.\n");
      }
    }
    else {
      pfDescriptorFunction =
          (LADSPA_Descriptor_Function) dlsym(PluginLibrary,
                                             "ladspa_descriptor");
      if (pfDescriptorFunction==NULL) {
        dlclose(PluginLibrary);
        return csound->InitError(csound, "%s", Str("No ladspa descriptor\n"));
      }
      DSSIPlugin_->Descriptor =
          (LADSPA_Descriptor *) csound->Calloc(csound,
                                               sizeof(LADSPA_Descriptor));
      /* DSSIPlugin_->Descriptor = */
      /*     (LADSPA_Descriptor *) pfDescriptorFunction(PluginIndex); */
      LDescriptor = (LADSPA_Descriptor *) DSSIPlugin_->Descriptor;
      memcpy(LDescriptor,
             pfDescriptorFunction(PluginIndex), sizeof(LADSPA_Descriptor));
      DSSIPlugin_->Type = LADSPA;
      if (verbose != 0) {
        csound->Message(csound, "DSSI4CS: LADSPA Plugin Detected\n");
      }
    }
    if (UNLIKELY((!DSSIPlugin_->Descriptor) &&
                 (!DSSIPlugin_->DSSIDescriptor))) {
      const char *pcError = dlerror();

      /* TODO: cleanup if error */
      /* csound->Free(csound, DSSIPlugin_->Descriptor); */
      if (pcError)
        csound->InitError(csound, Str("DSSI4CS: Unable to find "
                                      "ladspa_descriptor(%lu) function or\n"
                                      "dssi_descriptor(%lu) function in plugin "
                                      "file \"%s\": %s.\n"
                                      "Are you sure this is a LADSPA or "
                                      "DSSI plugin file ?"),
                          (unsigned long)PluginIndex, (unsigned long)PluginIndex,
                          dssiFilename, pcError);
      else
        csound->InitError(csound, Str("DSSI4CS: Unable to find "
                                      "ladspa_descriptor(%lu) function or\n"
                                      "dssi_descriptor(%lu) function in plugin "
                                      "file \"%s\".\n"
                                      "Are you sure this is a LADSPA or "
                                      "DSSI plugin file ?"),
                          (unsigned long)PluginIndex, (unsigned long)PluginIndex,
                          dssiFilename);
      dlclose(PluginLibrary);
      return NOTOK;
    }
    if (UNLIKELY(!LDescriptor)) {
      csound->InitError(csound, Str("DSSI4CS: No plugin index %lu in %s"),
                              (unsigned long) PluginIndex, dssiFilename);
      dlclose(PluginLibrary);
      return NOTOK;
    }
    if (verbose != 0) {
      csound->Message(csound, "DSSI4CS: About to instantiate plugin.\n");
    }

    if (DSSIPlugin_->Type == LADSPA) {
      if (UNLIKELY(!
          (DSSIPlugin_->Handle =
           (LADSPA_Handle) DSSIPlugin_->Descriptor->instantiate(DSSIPlugin_->
                                                                Descriptor,
                                                                SampleRate)))){
        csound->InitError(csound,
                          Str("DSSI4CS: Could not instantiate plugin: %s"),
                          dssiFilename);
        dlclose(PluginLibrary);
        return NOTOK;

      }
      if (UNLIKELY(!DSSIPlugin_->Descriptor->run)) {
        return csound->InitError(csound, Str("DSSI4CS: No run() function in: %s"),
                                 LDescriptor->Name);
      }
      PortCount = DSSIPlugin_->Descriptor->PortCount;
      //dlclose(PluginLibrary);
      //return NOTOK;
    }
    else {
      if (UNLIKELY(!
          (DSSIPlugin_->Handle =
           (LADSPA_Handle) DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->
           instantiate(DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin, SampleRate)))) {
        unloadLADSPAPluginLibrary(csound, PluginLibrary);
        return csound->InitError(csound,
                                 Str("DSSI4CS: Could not instantiate plugin: %s"),
                                 dssiFilename);
      }
      if (UNLIKELY(!DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->run)) {
        unloadLADSPAPluginLibrary(csound, PluginLibrary);
        return csound->InitError(csound, Str("DSSI4CS: No run() function in: %s"),
                                         LDescriptor->Name);
      }
      PortCount = DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->PortCount;
      DSSIPlugin_->Events =
          (snd_seq_event_t *) csound->Calloc(csound,
                                             DSSI4CS_MAX_NUM_EVENTS
                                             * sizeof(snd_seq_event_t));
    }
    if (verbose != 0) {
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
    if (verbose != 0) {
      csound->Message(csound, "DSSI4CS: Found %lu control ports for: '%s'\n",
                      (unsigned long)ConnectedControlPorts, LDescriptor->Name);
      csound->Message(csound, "DSSI4CS: Found %lu audio ports for: '%s'\n",
                      (unsigned long)ConnectedAudioPorts, LDescriptor->Name);
    }

    DSSIPlugin_->control =
        (LADSPA_Data **) csound->Calloc(csound, ConnectedControlPorts
                                              * sizeof(LADSPA_Data *));
    DSSIPlugin_->audio =
        (LADSPA_Data **) csound->Calloc(csound, ConnectedAudioPorts
                                               * sizeof(LADSPA_Data *));
    //    if (verbose != 0) {
      csound->Message(csound, "DSSI4CS: Created port array.\n");
      //    }

    ConnectedControlPorts = 0;
    ConnectedAudioPorts = 0;
    for (i = 0; i < PortCount; i++) {
      PortDescriptor =
          (DSSIPlugin_->Type ==
           LADSPA ? DSSIPlugin_->Descriptor->PortDescriptors[i]
           : DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin->PortDescriptors[i]);
      if (verbose != 0) {
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
        if (verbose != 0) {
          csound->Message(csound,
                          "DSSI4CS: Created internal control port "
                          "%lu for Port %lu.\n",
                          (unsigned long)ConnectedControlPorts,(unsigned long) i);
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
        if (verbose != 0) {
          csound->Message(csound,
                          "DSSI4CS: Created internal audio port"
                          " %lu for Port %lu.\n",
                          (unsigned long)ConnectedAudioPorts,(unsigned long) i);
        }

        ConnectedAudioPorts++;
      }

    }
    /* All ports must be connected before calling run() */
    if (verbose != 0) {
      csound->Message(csound, "DSSI4CS: Created %lu control ports for: '%s'\n",
                      (unsigned long)ConnectedControlPorts,LDescriptor->Name);
      csound->Message(csound, "DSSI4CS: Created %lu audio ports for: '%s'\n",
                      (unsigned long)ConnectedAudioPorts,LDescriptor->Name);
    }

    DSSIPlugin_->Active = 0;
    DSSIPlugin_->EventCount = 0;
    if (verbose != 0) {
      csound->Message(csound, "DSSI4CS: Init Done.\n");
      info(csound, DSSIPlugin_);
    }
    //dlclose(PluginLibrary);
    return OK;
}

/****************************************************************************
           dssiactivate
*****************************************************************************/

int32_t ActivatePlugin(CSOUND * csound, DSSI4CS_PLUGIN * DSSIPlugin_,
                       int32_t ktrigger)
{
  IGN(csound);
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

int32_t dssiactivate_init(CSOUND * csound, DSSIACTIVATE * p)
{
    int32_t     Number = *p->iDSSIhandle;

#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: activate-Locating plugin %i\n", Number);
#endif

    p->DSSIPlugin_ = LocatePlugin(Number, csound);
    p->printflag = -999;
    if (UNLIKELY((!p->DSSIPlugin_) || (Number > *p->DSSIPlugin_->PluginCount) ||
                 (!p->DSSIPlugin_->Handle)))
      return csound->InitError(csound,
                               Str("DSSI4CS: Invalid plugin: %i (MAX= %i)."),
                               Number, *p->DSSIPlugin_->PluginCount);
#ifdef DEBUG
    csound->Message(csound, "DSSI4CS: activate-Finished locating plugin %i\n",
                    p->DSSIPlugin_->PluginNumber);
#endif

    return OK;
}

int32_t dssiactivate(CSOUND * csound, DSSIACTIVATE * p)
{
    LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    int32_t     val = ActivatePlugin(csound, p->DSSIPlugin_, *p->ktrigger);

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

          return csound->PerfError(csound, &(p->h),
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
int32_t dssiaudio_init(CSOUND * csound, DSSIAUDIO * p)
{
    /* TODO not realtime safe, try to make it so. */
    int32_t     Number = *p->iDSSIhandle;
    int32_t     icnt = GetInputArgCnt(p) - 1;
    int32_t     ocnt = GetOutputArgCnt(p);

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
                    GetInputArgCnt(p), GetOutputArgCnt(p));
    csound->Message(csound, "DSSI4CS: dssiaudio LocatePlugin # %i\n", Number);
#endif

    p->DSSIPlugin_ = LocatePlugin(Number, csound);

    if (!p->DSSIPlugin_)
      return csound->InitError(csound, "%s",
                               Str("DSSI4CS: dssiaudio: Invalid plugin handle."));
    const LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;

    uint64_t PortIndex = 0;
    int32_t           ConnectedInputPorts = 0;
    int32_t           ConnectedOutputPorts = 0;
    int32_t           ConnectedPorts = 0;
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
    p->InputPorts = (uint64_t *)
        csound->Calloc(csound, ConnectedInputPorts * sizeof(uint64_t));
    p->OutputPorts = (uint64_t *)
        csound->Calloc(csound, ConnectedOutputPorts * sizeof(uint64_t));
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
                        (unsigned long)p->InputPorts[ConnectedInputPorts],
                        PortIndex);
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
                        (unsigned long)p->OutputPorts[ConnectedOutputPorts],
                        PortIndex);
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
        csound->Message(csound, Str("DSSI4CS: Plugin '%s' has %i audio input ports "
                                    "audio input discarded.\n"),
                                Descriptor->Name, p->NumInputPorts);
      else
        return csound->InitError(csound, Str("DSSI4CS: Plugin '%s' "
                                             "has %i audio input ports."),
                                         Descriptor->Name, p->NumOutputPorts);
    }
    if (p->NumOutputPorts < ocnt)
      return csound->InitError(csound, Str("DSSI4CS: Plugin '%s' "
                                           "has %i audio output ports."),
                                       Descriptor->Name, p->NumOutputPorts);
    return OK;
}

int32_t dssiaudio(CSOUND * csound, DSSIAUDIO * p)
{
    const LADSPA_Descriptor *Descriptor;

    if (p->DSSIPlugin_->Type == LADSPA)
      Descriptor = (LADSPA_Descriptor *) p->DSSIPlugin_->Descriptor;
    else
      Descriptor =
          (LADSPA_Descriptor *) p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    uint32_t i, j;


    uint32_t icnt = GetInputArgCnt(p) - 1;
    uint32_t ocnt = GetOutputArgCnt(p);
    uint64_t Ksmps = (uint64_t) CS_KSMPS;


    if (p->DSSIPlugin_->Active == 1) {
      for (j = 0; j < icnt; j++) {
        for (i = 0; i < Ksmps; i++) {
          p->DSSIPlugin_->audio[p->InputPorts[j]][i] =
            p->ain[j][i] * (1.0/csound->Get0dBFS(csound));
        }
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
int32_t dssictls_kk(CSOUND * csound, DSSICTLS * p)
{
    LADSPA_Data Value = *p->val;
    if (!p->DSSIPlugin_) {
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("DSSI4CS: Invalid plugin handle."));
    }
    if (*p->ktrig == 1) {
      *p->DSSIPlugin_->control[p->PortNumber] = Value * (p->HintSampleRate);
    }
    return OK;
}

int32_t dssictls_ak(CSOUND * csound, DSSICTLS * p)
{
    csound->PerfError(csound, &(p->h),
                      "%s", Str("DSSI4CS: Audio Rate control "
                          "ports not implemented yet."));
    return NOTOK;
}

int32_t dssictls_init(CSOUND * csound, DSSICTLS * p)
{
    /* TODO warning possible crash or unpredictable behaviour if invalid port.
            Crash if audio port selected */
    const LADSPA_Descriptor *Descriptor;
    int32_t     Number = *p->iDSSIhandle;
    int32_t     Sr = (int32_t) MYFLT2LRND(CS_ESR);
    uint64_t PortIndex = *p->iport;
    uint32_t  i;
    uint64_t ControlPort = 0;
    uint64_t AudioPort = 0;
    uint64_t Port = 0;

    p->DSSIPlugin_ = LocatePlugin(Number, csound);
    if (!p->DSSIPlugin_) {
      return csound->InitError(csound, "DSSI4CS: Invalid plugin handle.");
    }
    if (p->DSSIPlugin_->Type == LADSPA) {
      Descriptor = p->DSSIPlugin_->Descriptor;
    }
    else {
      Descriptor = p->DSSIPlugin_->DSSIDescriptor->LADSPA_Plugin;
    }

    if (PortIndex >= Descriptor->PortCount) {
      return
        csound->InitError(csound,
                          Str("DSSI4CS: Port %lu from '%s' does not exist."),
                          (unsigned long)PortIndex, Descriptor->Name);
    }
    p->HintSampleRate =
        (LADSPA_IS_HINT_SAMPLE_RATE
         (Descriptor->PortRangeHints[PortIndex].HintDescriptor) ? Sr : 1);
#ifdef DEBUG
    csound->Message(csound,
                    "DSSI4CS: Port %lu multiplier (HintSampleRate): %i.\n",
                    (unsigned long)PortIndex, p->HintSampleRate);
#endif
    LADSPA_PortDescriptor PortDescriptor =
        Descriptor->PortDescriptors[PortIndex];
    if (LADSPA_IS_PORT_OUTPUT(PortDescriptor))
      return csound->InitError(csound,
                               Str("DSSI4CS: Port %lu from '%s' is an "
                                   "output port."),
                               (unsigned long)PortIndex, Descriptor->Name);
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

#endif

    return OK;
}

int32_t dssictls_dummy(CSOUND * csound, DSSICTLS * p)
{
    csound->PerfError(csound, &(p->h),
                      "%s", Str("DSSI4CS: Not initialised or wrong argument types."));
    return NOTOK;
}

/****************************************************************************
           dssisynth
*****************************************************************************/

int32_t dssisynth_init(CSOUND * csound, DSSISYNTH * p)
{
  IGN(p);
    /* TODO docs: dssisynth only for DSSI plugs */
    return csound->InitError(csound, "DSSI4CS: dssisynth not implemented yet.");
}

int32_t dssisynth(CSOUND * csound, DSSISYNTH * p)
{
  IGN(p); IGN(csound);
    return OK;
}

/****************************************************************************
           dssinotes
*****************************************************************************/
int32_t dssinote_init(CSOUND * csound, DSSINOTE * p)
{
  IGN(p);
    return csound->InitError(csound, "%s", Str("DSSI4CS: dssinote not implemented yet."));
}

int32_t dssinote(CSOUND * csound, DSSINOTE * p)
{
  IGN(p); IGN(csound);
    return OK;
}

int32_t dssievent_init(CSOUND * csound, DSSINOTEON * p)
{
  IGN(p);
    return
      csound->InitError(csound, "%s", Str("DSSI4CS: dssievent not implemented yet."));
}

int32_t dssievent(CSOUND * csound, DSSINOTEON * p)
{
  IGN(p); IGN(csound);
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
    int64_t    lDirLength;
    int64_t    slen;
    int64_t    iNeedSlash;
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
      strNcpy(pcFilename, pcDirectory, slen); /*pcFilename[slen-1] = '\0';*/
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
                      "%s", Str("DSSI4CS: LADSPA_PATH environment variable not set.\n"));
#ifdef LIB64
      pcLADSPAPath = "/usr/lib64/ladspa/";
#else
      pcLADSPAPath = "/usr/lib/ladspa/";
#endif
    }
    if (!pcDSSIPath) {
      csound->Message(csound,
                      "%s", Str("DSSI4CS: DSSI_PATH environment variable not set.\n"));
      pcStart = pcLADSPAPath;
    }
    else {
      int32_t len = strlen(pcLADSPAPath)+strlen(pcDSSIPath)+2;
      char *tmp = (char*)malloc(len);
      snprintf(tmp, len, "%s:%s", pcLADSPAPath, pcDSSIPath);
      pcLADSPAPath = tmp;
      pcStart = (const char*)tmp;
    }
    // Search the list
    while (*pcStart != '\0') {
      pcEnd = pcStart;
      while (*pcEnd != ':' && *pcEnd != '\0')
        pcEnd++;

      pcBuffer = csound->Malloc(csound, 1 + (pcEnd - pcStart));
      if (pcEnd > pcStart) {
        strNcpy(pcBuffer, pcStart, pcEnd - pcStart +1);
        //pcBuffer[pcEnd - pcStart] = '\0';
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
    int32_t     lIndex;

    csound->Message(csound, "Plugin: %s:\n", pcFullFilename);
    for (lIndex = 0;
         (psDescriptor = fDescriptorFunction(lIndex)) != NULL; lIndex++)
      csound->Message(csound, "  Index: %i : %s (%lu/%s)\n",
                      lIndex,
                      psDescriptor->Name,
                      (unsigned long)psDescriptor->UniqueID, psDescriptor->Label);

    dlclose(pvPluginHandle);
}

int32_t dssilist(CSOUND * csound, DSSILIST * p)
{
    /* Most of this function comes from the ladspa sdk by Richard Furse */
  IGN(p);
    char   *pcBuffer;
    const char *pcEnd;
          char *pcLADSPAPath;
    const char *pcDSSIPath;
    const char *pcStart;
    const char *src;

    src = getenv("LADSPA_PATH");
    if (src)
      pcLADSPAPath = strndup(src, 1024);
    else
      pcLADSPAPath = NULL;

    pcDSSIPath = getenv("DSSI_PATH");
    src = getenv("DSSI_PATH");
    if (src)
      pcDSSIPath = strndup(src, 1024);
    else
      pcDSSIPath = NULL;


    if (!pcLADSPAPath) {
      csound->Message(csound,
                      "%s", Str("DSSI4CS: LADSPA_PATH environment variable not set.\n"));
    }
    if (!pcDSSIPath) {
      csound->Message(csound,
                      "%s", Str("DSSI4CS: DSSI_PATH environment variable not set.\n"));
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
        free(pcLADSPAPath);
        pcLADSPAPath = nn;
      }
      else {
        pcLADSPAPath = strdup(pcDSSIPath);
      }

    }
    pcStart = pcLADSPAPath;
    while (*pcStart != '\0') {
      pcEnd = pcStart;
      while (*pcEnd != ':' && *pcEnd != '\0')
        pcEnd++;
      pcBuffer = csound->Calloc(csound, 1 + (pcEnd - pcStart));
      if (pcEnd > pcStart)
        strNcpy(pcBuffer, pcStart, pcEnd - pcStart +1);
      //pcBuffer[pcEnd - pcStart] = '\0';
      LADSPADirectoryPluginSearch(csound, pcBuffer, describePluginLibrary);
      csound->Free(csound, pcBuffer);
      pcStart = pcEnd;
      if (*pcStart == ':')
        pcStart++;
    }
    free(pcLADSPAPath);
    free((void *)pcDSSIPath);
    return OK;
}

static OENTRY dssi_localops[] = {
  {"dssiinit", sizeof(DSSIINIT), 0, "i", "Tip", (SUBR) dssiinit, NULL, NULL },
  {"dssiactivate", sizeof(DSSIACTIVATE), 0, "", "ik",
   (SUBR) dssiactivate_init, (SUBR) dssiactivate, NULL },
  {"dssiaudio", sizeof(DSSIAUDIO), 0,  "mmmmmmmmm", "iMMMMMMMMM",
   (SUBR) dssiaudio_init, (SUBR) dssiaudio },
    {"dssictls", sizeof(DSSICTLS), 0,  "", "iikk", (SUBR) dssictls_init,
     (SUBR) dssictls_kk, NULL },
    {"dssilist", sizeof(DSSILIST), 0,  "", "", (SUBR) dssilist, NULL, NULL }
#if 0
    ,
    {"dssisynth", sizeof(DSSISYNTH), 0,  "aa",  "i", (SUBR) dssisynth_init,
     (SUBR) dssisynth}
    ,
    {"dssinote", sizeof(DSSINOTE), 0,  "",  "kikkk", (SUBR) dssinote_init,
     (SUBR) dssinote}
    ,
    {"dssievent", sizeof(DSSINOTEON), 0,  "",  "kikk", (SUBR) dssievent_init,
     (SUBR) dssievent}
#endif
};

LINKAGE_BUILTIN(dssi_localops)
