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

#include "cs.h"
#include "midiinterop.h"
#include "math.h"

#define dv127   (FL(1.0)/FL(127.0))

int midinoteoff(ENVIRON *csound, MIDINOTEON *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonkey(ENVIRON *csound, MIDINOTEON *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xkey = p->h.insdshead->m_pitch;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteoncps(ENVIRON *csound, MIDINOTEON *p)
{
    MYFLT octave;
    long longOctave;
    if (!p->h.insdshead->m_chnbp) {
      octave = (MYFLT) (*p->xkey / FL(12.0) + FL(3.0));
      longOctave = (long)(octave * OCTRES);
      *p->xkey = (MYFLT) CPSOCTL(longOctave);
      return OK;
    }
    octave = (MYFLT) (p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    longOctave = (long)(octave * OCTRES);
    *p->xkey = (MYFLT) CPSOCTL(longOctave);
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonoct(ENVIRON *csound, MIDINOTEON *p)
{
    MYFLT octave;
    if (!p->h.insdshead->m_chnbp) {
      octave = (MYFLT) (*p->xkey / FL(12.0) + FL(3.0));
      *p->xkey = octave;
      return OK;
    }
    octave = (MYFLT) (p->h.insdshead->m_pitch / FL(12.0) + FL(3.0));
    *p->xkey = octave;
    *p->xvelocity = p->h.insdshead->m_veloc;
    return OK;
}

int midinoteonpch(ENVIRON *csound, MIDINOTEON *p)
{
    double pitch;
    double octave;
    double integer;
    double fraction;
    if (!p->h.insdshead->m_chnbp) {
      pitch = (double)*p->xkey;
      octave = pitch / 12.0 + 3.0;
      fraction = modf(octave, &integer);
      fraction *= 0.12;
      *p->xkey = (MYFLT)(integer + fraction);
      *p->xvelocity = p->h.insdshead->m_veloc;
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

int midipolyaftertouch(ENVIRON *csound, MIDIPOLYAFTERTOUCH *p)
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

int midicontrolchange(ENVIRON *csound, MIDICONTROLCHANGE *p)
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

int midiprogramchange(ENVIRON *csound, MIDIPROGRAMCHANGE *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xprogram = p->h.insdshead->m_chnbp->pgmno;
    return OK;
}

int midichannelaftertouch(ENVIRON *csound, MIDICHANNELAFTERTOUCH *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xchannelaftertouch = *p->olow + p->h.insdshead->m_chnbp->aftouch * scale;
    return OK;
}

int midipitchbend(ENVIRON *csound, MIDIPITCHBEND *p)
{
    MYFLT scale;
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    scale = (*p->hhigh - *p->olow) * dv127;
    *p->xpitchbend = *p->olow + p->h.insdshead->m_chnbp->pchbend * scale;
    return OK;
}

int mididefault(ENVIRON *csound, MIDIDEFAULT *p)
{
    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    *p->xvalue = *p->xdefault;
    return OK;
}
