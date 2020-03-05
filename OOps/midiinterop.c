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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "midiinterop.h"
#include "math.h"

#define dv127   (FL(1.0)/FL(127.0))

/* aops.c, table for CPSOCTL */
/* extern  MYFLT   cpsocfrc[]; */

int32_t midinoteoff(CSOUND *csound, MIDINOTEON *p)
{
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int32_t midinoteonkey(CSOUND *csound, MIDINOTEON *p)
{
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int32_t midinoteoncps(CSOUND *csound, MIDINOTEON *p)
{
    MYFLT octave;
    int32_t longOctave;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    octave = (MYFLT)(p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    longOctave = (int32_t)(octave * OCTRES);
    *p->xkey = (MYFLT)CPSOCTL(longOctave);
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int32_t midinoteonoct(CSOUND *csound, MIDINOTEON *p)
{
    MYFLT octave;
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    octave = (MYFLT)(p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    *p->xkey = octave;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int32_t midinoteonpch(CSOUND *csound, MIDINOTEON *p)
{
    double pitch;
    double octave;
    double integer;
    double fraction;
    IGN(csound);
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

int32_t midipolyaftertouch(CSOUND *csound, MIDIPOLYAFTERTOUCH *p)
{
    MYFLT scale;
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xpolyaftertouch = *p->olow +
      p->h.insdshead->m_chnbp->polyaft[(int32_t)*p->xcontroller] * scale;
    return OK;
}

int32_t midicontrolchange(CSOUND *csound, MIDICONTROLCHANGE *p)
{
    MYFLT scale;
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xcontrollervalue =
      *p->olow + p->h.insdshead->m_chnbp->ctl_val[(int32_t)*p->xcontroller] * scale;
    return OK;
}

int32_t midiprogramchange(CSOUND *csound, MIDIPROGRAMCHANGE *p)
{
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xprogram = p->h.insdshead->m_chnbp->pgmno;
    return OK;
}

int32_t midichannelaftertouch(CSOUND *csound, MIDICHANNELAFTERTOUCH *p)
{
    MYFLT scale;
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xchannelaftertouch = *p->olow + p->h.insdshead->m_chnbp->aftouch * scale;
    return OK;
}

int32_t midipitchbend(CSOUND *csound, MIDIPITCHBEND *p)
{
    MYFLT scale;
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xpitchbend = *p->olow + p->h.insdshead->m_chnbp->pchbend * scale;
    return OK;
}

int32_t mididefault(CSOUND *csound, MIDIDEFAULT *p)
{
    IGN(csound);
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xvalue = *p->xdefault;
    return OK;
}

