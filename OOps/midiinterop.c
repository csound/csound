/*
    midiinterop.c:

    Copyright (C) 2002 Michael Gogins

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

#include "csoundCore.h"
#include "midiinterop.h"
#include "math.h"

#define dv127   (FL(1.0)/FL(127.0))

/* aops.c, table for CPSOCTL */
extern  MYFLT   cpsocfrc[];

int midinoteoff(CSOUND *csound, MIDINOTEON *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonkey(CSOUND *csound, MIDINOTEON *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteoncps(CSOUND *csound, MIDINOTEON *p)
{
    MYFLT octave;
    int32 longOctave;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    octave = (MYFLT)(p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    longOctave = (int32)(octave * OCTRES);
    *p->xkey = (MYFLT)CPSOCTL(longOctave);
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonoct(CSOUND *csound, MIDINOTEON *p)
{
    MYFLT octave;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    octave = (MYFLT)(p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    *p->xkey = octave;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonpch(CSOUND *csound, MIDINOTEON *p)
{
    double pitch;
    double octave;
    double integer;
    double fraction;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    pitch = (double)p->h.insdshead->m_pitch;
    octave = pitch / 12.0 + 3.0;
    fraction = modf(octave, &integer);
    fraction *= 0.12;
    *p->xkey = (MYFLT)(integer + fraction);
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midipolyaftertouch(CSOUND *csound, MIDIPOLYAFTERTOUCH *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xpolyaftertouch = *p->olow +
      p->h.insdshead->m_chnbp->polyaft[(int)*p->xcontroller] * scale;
    return OK;
}

int midicontrolchange(CSOUND *csound, MIDICONTROLCHANGE *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xcontrollervalue =
      *p->olow + p->h.insdshead->m_chnbp->ctl_val[(int)*p->xcontroller] * scale;
    return OK;
}

int midiprogramchange(CSOUND *csound, MIDIPROGRAMCHANGE *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xprogram = p->h.insdshead->m_chnbp->pgmno;
    return OK;
}

int midichannelaftertouch(CSOUND *csound, MIDICHANNELAFTERTOUCH *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xchannelaftertouch = *p->olow + p->h.insdshead->m_chnbp->aftouch * scale;
    return OK;
}

int midipitchbend(CSOUND *csound, MIDIPITCHBEND *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xpitchbend = *p->olow + p->h.insdshead->m_chnbp->pchbend * scale;
    return OK;
}

int mididefault(CSOUND *csound, MIDIDEFAULT *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xvalue = *p->xdefault;
    return OK;
}

