/*
    paulstretch.c:

    Copyright (C) 2016 by Paul Batchelor

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
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include "csoundCore.h"

typedef struct {
    OPDS h;
    MYFLT *out, *stretch, *winsize, *ifn;
    MYFLT start_pos, displace_pos;
    MYFLT *window;
    MYFLT *old_windowed_buf;
    MYFLT *hinv_buf;
    MYFLT *buf;
    MYFLT *output;
    FUNC *ft;
    uint32_t windowsize;
    uint32_t half_windowsize;
    MYFLT *tmp;
    uint32_t counter;
    AUXCH m_window;
    AUXCH m_old_windowed_buf;
    AUXCH m_hinv_buf;
    AUXCH m_buf;
    AUXCH m_output;
    AUXCH m_tmp;
} PAULSTRETCH;

static void compute_block(CSOUND *csound, PAULSTRETCH *p) {
    uint32_t istart_pos = floor(p->start_pos);
    uint32_t pos; 
    uint32_t i;
    uint32_t windowsize = p->windowsize;
    uint32_t half_windowsize = p->half_windowsize;
    MYFLT *buf = p->buf;
    MYFLT *hinv_buf = p->hinv_buf;
    MYFLT *old_windowed_buf= p->old_windowed_buf;
    MYFLT *tbl = p->ft->ftable;
    MYFLT *window = p->window;
    MYFLT *output= p->output;
    MYFLT *tmp = p->tmp;
    for(i = 0; i < windowsize; i++) {
        pos = istart_pos + i;
        if(pos < p->ft->flen) {
            tmp[i] = tbl[pos] * window[i];
        } else {
            tmp[i] = 0;
        }
    }
    tmp[p->windowsize] = tmp[1];
    tmp[p->windowsize + 1] = 0.0;
    csoundRealFFTnp2(csound, tmp, p->windowsize);
    for(i = 0; i < windowsize + 2; i += 2) {
        MYFLT mag = sqrt(tmp[i]*tmp[i] + tmp[i + 1]*tmp[i + 1]);
        complex ph = cexpf(I * ((MYFLT)rand() / RAND_MAX) * 2 * M_PI);
        tmp[i] = mag * (MYFLT)crealf(ph); 
        tmp[i + 1] = mag * (MYFLT)cimagf(ph); 
    }
    tmp[1] = tmp[p->windowsize];
    csoundInverseRealFFTnp2(csound, tmp, p->windowsize);
    for(i = 0; i < windowsize; i++) {
        tmp[i] *= window[i];
        if(i < half_windowsize) {
            output[i] = (MYFLT)(tmp[i] + old_windowed_buf[half_windowsize + i]);
            output[i] *= hinv_buf[i];
        }
        old_windowed_buf[i] = tmp[i];
    }
    p->start_pos += p->displace_pos;
}

static int ps_init(CSOUND* csound, PAULSTRETCH *p)
{
    FUNC *ftp;
    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
      return NOTOK;
    p->ft = ftp;
    uint32_t i = 0;
    p->windowsize = (uint32_t)(CS_ESR * *p->winsize);
    if(p->windowsize < 16) {
        p->windowsize = 16;
    }
    p->half_windowsize = p->windowsize / 2;
    p->displace_pos = (p->windowsize * 0.5) / *p->stretch;

    csound->AuxAlloc(csound, (size_t) sizeof(MYFLT) * p->windowsize, &(p->m_window));
    p->window = p->m_window.auxp;

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->windowsize, &p->m_old_windowed_buf);
    p->old_windowed_buf = p->m_old_windowed_buf.auxp;

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->half_windowsize, &p->m_hinv_buf);
    p->hinv_buf = p->m_hinv_buf.auxp;

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->windowsize, &p->m_buf);
    p->buf = p->m_buf.auxp;

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->half_windowsize, &p->m_output);
    p->output = p->m_output.auxp;

    csound->AuxAlloc(csound, sizeof(MYFLT) * p->windowsize + 2, &p->m_tmp);
    p->tmp = p->m_tmp.auxp;

    /* Create Hann window */
    for(i = 0; i < p->windowsize; i++) {
        p->window[i] = 0.5 - cos(i * 2.0 * M_PI / (p->windowsize - 1)) * 0.5;
    }
    /* creatve inverse hann window */
    MYFLT hinv_sqrt2 = (1 + sqrt(0.5)) * 0.5;
    for(i = 0; i < p->half_windowsize; i++) {
        p->hinv_buf[i] = hinv_sqrt2 - (1.0 - hinv_sqrt2) * cos(i * 2.0 * M_PI / p->half_windowsize);
    }
    p->start_pos = 0.0;
    p->counter = 0;

    printf("PaulStretch: window size %d stretch %g half windowsize %d\n", 
            p->windowsize, *p->stretch, p->half_windowsize);
    return OK;
}

static int paulstretch_perf(CSOUND* csound, PAULSTRETCH *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT *out = p->out;

    if (UNLIKELY(offset)) {
      memset(p->out, '\0', offset*sizeof(MYFLT));
      memset(p->out, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&p->out[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n = offset; n < nsmps; n++) {
        if(p->counter == 0) {
            compute_block(csound, p);
        }
        out[n] = p->output[p->counter];
        //out[n] = p->ft->ftable[p->counter];
        p->counter = (p->counter + 1) % p->half_windowsize;
    }
    return OK;
}



int paulstretch_init_(CSOUND *csound)
{
    return csound->AppendOpcode(csound, "paulstretch",
                                (int) sizeof(PAULSTRETCH), 0, 5, "a", "jjj",
                                (int (*)(CSOUND *, void *)) ps_init,
                                (int (*)(CSOUND *, void *)) NULL,
                                (int (*)(CSOUND *, void *)) paulstretch_perf);
}

