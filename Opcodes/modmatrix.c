/*
Modmatrix - modulation matrix
Copyright (C) 2009 Øyvind Brandtsegg, Thom Johansen

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "modmatrix.h"

#define INITERROR(x) csound->InitError(csound, "%s", Str("modmatrix: " x))

#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__SSE__)
#include <xmmintrin.h>
#endif

static int32_t modmatrix_init(CSOUND *csound, MODMATRIX *m)
{
    uint32_t size;

    m->restab = csound->FTFind(csound, m->ires);
    m->modtab = csound->FTFind(csound, m->imod);
    m->parmtab = csound->FTFind(csound, m->iparm);
    m->mattab = csound->FTFind(csound, m->imatrix);
    if (UNLIKELY(!m->restab))
        return INITERROR("unable to load result table");
    if (UNLIKELY(!m->modtab))
        return INITERROR("unable to load modulator table");
    if (UNLIKELY(!m->parmtab))
        return INITERROR("unable to load parameter value table");
    if (UNLIKELY(!m->mattab))
        return INITERROR("unable to load routing matrix table");

    m->nummod = (int32_t)*m->inummod;
    m->numparm = (int32_t)*m->inumparm;
    if (UNLIKELY(m->nummod <= 0))
        return INITERROR("number of modulators must be a positive integer");
    if (UNLIKELY(m->numparm <= 0))
        return INITERROR("number of parameters must be a positive integer");

    /* Malloc one big chunk instead of several small ones, we need (worst case):
       MYFLTs - nummod*numparm, nummod, numparm
       ints - nummod, numparm */
    size =  (m->nummod*m->numparm + m->nummod + m->numparm)*sizeof(MYFLT) +
            (m->nummod + m->numparm)*sizeof(int32_t);
    if (m->aux.auxp == NULL || m->aux.size < size)
        csound->AuxAlloc(csound, size, &m->aux);
    if (UNLIKELY(m->aux.auxp == NULL))
        return INITERROR("memory allocation error");

    m->proc_mat = (MYFLT *)m->aux.auxp;
    m->mod_map = (int32_t *)(m->proc_mat + m->nummod*m->numparm);
    m->parm_map = m->mod_map + m->nummod;
    m->remap_mod = (MYFLT *)(m->parm_map + m->numparm);
    m->remap_parm = m->remap_mod + m->nummod;

    m->scanned = 0;
    m->doscan = 1;
    return OK;
}

static void scan_modulation_matrix(CSOUND *csound, MODMATRIX *m)
{
    IGN(csound);
    int32_t i, j, k;
    MYFLT *matval;
    /* Use the still unused process matrix for this temporary array */
    int32_t *coltab = (int32_t *)m->proc_mat;

    memset(coltab, 0, m->numparm*sizeof(int32_t));
    m->nummod_scanned = m->numparm_scanned = 0;

    /* Scan for rows containing only zero */
    k = 0;
    for (i = 0; i < m->nummod; ++i) {
        MYFLT *cur = &m->mattab->ftable[m->numparm*i];

        for (j = 0; j < m->numparm; ++j) {
            if (*cur++ != FL(0.0)) {
                m->mod_map[k++] = i;
                ++m->nummod_scanned;
                coltab[j] = 1; /* Mark column as non-zero */
                break;
            }
        }
    }

    k = 0;
    /* Scan for columns containing only zero */
    for (i = 0; i < m->numparm; ++i) {
        MYFLT *cur = &m->mattab->ftable[i];
        int32_t nonzero = coltab[i];

        if (!nonzero) {
            /* Columns is not previously marked as being non-zero, scan it */
            for (j = 0; j < m->nummod; ++j) {
                if (*cur != FL(0.0)) {
                    nonzero = 1;
                    break;
                }
                cur += m->numparm;
            }
        }
        if (nonzero) {
            m->parm_map[k++] = i;
            ++m->numparm_scanned;
        }
    }

    /* TODO Possibly check numparm_scanned and nummod_scanned here to see if it
       even makes sense to process the matrix, set m->scanned accordingly */
    /* Rebuild matrix without zero rows and columns */
    matval = m->proc_mat;
    for (i = 0; i < m->nummod_scanned; ++i) {
        int32_t mod = m->mod_map[i];
        MYFLT *row = &m->mattab->ftable[mod*m->numparm];

        for (j = 0; j < m->numparm_scanned; ++j) {
            int32_t parm = m->parm_map[j];
            *matval++ = row[parm];
        }
    }

    m->scanned = 1;
    m->doscan = 0;
}

static void process(CSOUND *csound, MODMATRIX *m)
{
    IGN(csound);
    int32_t col = 0, row;

    MYFLT *src = &m->modtab->ftable[0];
    /* Use unrolled SSE based loops if possible. All loading and storing has to
       be unaligned since Csound has no alignment guarantees on anything, to the
       best of my knowledge */
#if defined(__SSE__) && !defined(USE_DOUBLE)
    for (; col < (m->numparm & ~7); col += 8) {
        __m128 acc1 = _mm_setzero_ps();
        __m128 acc2 = _mm_setzero_ps();
        float *curmod = &m->mattab->ftable[col];

        for (row = 0; row < m->nummod; ++row) {
            __m128 srcval = _mm_load1_ps(&src[row]);
            __m128 modcoef1 = _mm_loadu_ps(curmod);
            __m128 modcoef2 = _mm_loadu_ps(curmod + 4);
            acc1 = _mm_add_ps(acc1, _mm_mul_ps(srcval, modcoef1));
            acc2 = _mm_add_ps(acc2, _mm_mul_ps(srcval, modcoef2));
            curmod += m->numparm;
        }
        __m128 params1 = _mm_loadu_ps(&m->parmtab->ftable[col]);
        __m128 params2 = _mm_loadu_ps(&m->parmtab->ftable[col + 4]);
        _mm_storeu_ps(&m->restab->ftable[col], _mm_add_ps(params1, acc1));
        _mm_storeu_ps(&m->restab->ftable[col + 4], _mm_add_ps(params2, acc2));
    }
#elif defined(__SSE2__) && defined(USE_DOUBLE)
    for (; col < (m->numparm & ~3); col += 4) {
        __m128d acc1 = _mm_setzero_pd();
        __m128d acc2 = _mm_setzero_pd();

        double *curmod = &m->mattab->ftable[col];

        for (row = 0; row < m->nummod; ++row) {
            __m128d srcval = _mm_load1_pd(&src[row]);
            __m128d modcoef1 = _mm_loadu_pd(curmod);
            __m128d modcoef2 = _mm_loadu_pd(curmod + 2);

            acc1 = _mm_add_pd(acc1, _mm_mul_pd(srcval, modcoef1));
            acc2 = _mm_add_pd(acc2, _mm_mul_pd(srcval, modcoef2));
            curmod += m->numparm;
        }
        __m128d params1 = _mm_loadu_pd(&m->parmtab->ftable[col]);
        __m128d params2 = _mm_loadu_pd(&m->parmtab->ftable[col + 2]);
        _mm_storeu_pd(&m->restab->ftable[col], _mm_add_pd(params1, acc1));
        _mm_storeu_pd(&m->restab->ftable[col + 2], _mm_add_pd(params2, acc2));
    }
#endif
    /* This piece of code will either handle the entire matrix in case neither
       of the above got compiled, or handle the last columns of a matrix with
       width not divisible by four */
    for (; col < m->numparm; ++col) {
        MYFLT acc = FL(0.0);
        MYFLT *curmod = &m->mattab->ftable[col];

        for (row = 0; row < m->nummod; ++row) {
            acc += (*curmod)*src[row];
            curmod += m->numparm;
        }
        m->restab->ftable[col] = m->parmtab->ftable[col] + acc;
    }
}

static void process_scanned(CSOUND *csound, MODMATRIX *m)
{
    IGN(csound);
    int32_t col = 0, row;
    int32_t i;
    MYFLT *src = &m->remap_mod[0];

    for (i = 0; i < m->nummod_scanned; ++i)
        m->remap_mod[i] = m->modtab->ftable[m->mod_map[i]];
    for (i = 0; i < m->numparm_scanned; ++i)
        m->remap_parm[i] = m->parmtab->ftable[m->parm_map[i]];
    memcpy(m->restab->ftable, m->parmtab->ftable, m->numparm*sizeof(MYFLT));

    for (; col < m->numparm_scanned; ++col) {
        MYFLT acc = FL(0.0);
        MYFLT *curmod = &m->proc_mat[col];

        for (row = 0; row < m->nummod_scanned; ++row) {
            acc += (*curmod)*src[row];
            curmod += m->numparm_scanned;
        }
        m->restab->ftable[m->parm_map[col]] += acc;
    }
}

static int32_t
modmatrix(CSOUND *csound, MODMATRIX *m)
{
    /* We wait until the update signal has gone low again before actually
       preprocessing a matrix */
    if (*m->kupdate > FL(0.0) && !m->doscan) {
        m->doscan = 1;
        m->scanned = 0;
    } else if (*m->kupdate == FL(0.0) && m->doscan) {
        scan_modulation_matrix(csound, m);
    }
    if (!m->scanned)
        process(csound, m);
    else
        process_scanned(csound, m);
    return OK;
}

static OENTRY modmatrix_localops[] = {
    {
      "modmatrix", sizeof(MODMATRIX), TB,
      "",
      "iiiiiik",
      (SUBR)modmatrix_init,
      (SUBR)modmatrix,
      (SUBR)NULL
    }
};

LINKAGE_BUILTIN(modmatrix_localops)

