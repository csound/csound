/*
  mididevice.c:

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

#include "cs.h"
#include "midiops.h"
#include "oload.h"
#include <portmidi.h>
#include <porttime.h>
#include <errno.h>

extern  int     csoundIsExternalMidiEnabled(void *);
extern  void    csoundExternalMidiDeviceOpen(void *);
extern  void    csoundExternalMidiDeviceClose(void *);
extern  long    csoundExternalMidiRead(void*, unsigned char*, int);

void OpenMIDIDevice(ENVIRON *csound)
{
    csound = csound;
}

int GetMIDIData(ENVIRON *csound, unsigned char *mbuf, int nbytes)
{
    int     n;
    /*
     * Reads from user-defined MIDI input.
     */
    if (csoundIsExternalMidiEnabled(csound)) {
      n = csoundExternalMidiRead(csound, mbuf, nbytes);
      return n;
    }
    return 0;
}

void CloseMIDIDevice(ENVIRON *csound)
{
    csound = csound;
}

