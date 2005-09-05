/*
    alsamidi.c:

    Copyright (C) 2005 Istvan Varga

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "csdl.h"

/* TODO */

/* module interface functions */

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    /* nothing to do, report success */
 /* csound->Message(csound, "ALSA real time MIDI plugin for Csound\n"); */
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    char    *drv;

    drv = (char*) (csound->QueryGlobalVariable(csound, "_RTMIDI"));
    if (drv == NULL)
      return 0;
    if (!(strcmp(drv, "ALSA") == 0 || strcmp(drv, "Alsa") == 0 ||
          strcmp(drv, "alsa") == 0))
      return 0;
    csound->Die(csound, "rtmidi: ALSA module enabled\n");
 /* csound->SetExternalMidiInOpenCallback(csound, midi_in_open);
    csound->SetExternalMidiReadCallback(csound, midi_in_read);
    csound->SetExternalMidiInCloseCallback(csound, midi_in_close);
    csound->SetExternalMidiOutOpenCallback(csound, midi_out_open);
    csound->SetExternalMidiWriteCallback(csound, midi_out_write);
    csound->SetExternalMidiOutCloseCallback(csound, midi_out_close); */
    return 0;
}

PUBLIC int csoundModuleInfo(void)
{
    /* does not depend on MYFLT type */
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8));
}

