 /*
    babo.c:

    Copyright (C) 2000 Davide Rocchesso, Nicola Bernardini

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

/***************************************************************************
 * vi:set nowrap ts=4:
 *
 * Ball-within-a-box physical model reverberator
 * originally written by Paolo Filippi (paolfili@ml.swapnet.it) of CSC
 * (Centro di Sonologia Computazionale - Universita' di Padova, Italia)
 *
 * Model of a rectangular enclosure implemented by means of a
 * Circulant Feedback Delay Network
 *
 * The name BaBo and the whole structure of the model is
 * described in the article :
 * [1] D. Rocchesso  "The Ball within the Box: a sound-processing
 *                metaphor", Computer Music Journal, Vol 19, N. 4,
 *                pp. 45-47, Winter 1995.
 *
 * Circulant Feedback Delay Networks (CFDN) are described in the article:
 * [2] D. Rocchesso and J.O. Smith "Circulant and Elliptic Feedback Delay Networks
 *                for Artificial Reverberation", IEEE Trans. on Speech and
 *                Audio Processing, vol. 5, n. 1, pp. 51-63, jan. 1997.
 *
 * Maximally-Diffusive CFDNs are described in the letter:
 * [3] D. Rocchesso "Maximally-Diffusive yet Efficient Feedback Delay Networks
 *                for Artificial Reverberation", IEEE Signal Processing Letters,
 *                vol. 4, n. 9, pp. 252-255, sep. 1997.
 *
 * This implementation has been developed under the advice of
 * Davide Rocchesso (rocchesso@sci.univr.it) and
 * Nicola Bernardini (nicb@axnet.it)
 *
 * $Id$
 *
 ***************************************************************************/

/*
 * You will find the functions you are looking for (probably baboset()
 * and babo()) at the end of this file. Before that, there is some
 * explanation of the structure of babo and then a bunch of private
 * functions used by baboset() and babo().
 */

/*
 * In general, (x,y,z) are coordinates in a three-dimensional space, either
 * of the sound source or of the pickup point (we use a pair of omnidi-
 * rectional pickups aligned with the x axis).
 * When the diffusion is maximal, each row of the matrix contains a
 * maximum-length sequence of +1 and -1. When the diffusion is minimal, the
 * matrix is diagonal. The continuous control of diffusion is done by moving
 * the eigenvalues along the unit circle and taking the inverse discrete fourier
 * transform of them [2,3].
 */

/*------------------------------------------------------------------------*\

BABO STRUCTURE
^^^^ ^^^^^^^^^

        ____
------>|    |tap0 (direct)
input  |    |------>|
       | T  |tap1   |
       | A  |------>|
       | P  |tap2   |
       | L  |------>|                _____________________________
       | I  |       |               |                             |
       | N  |......>|  +------------| g(0)  g(1)  g(2) ... g(14)  |-------+
       | E  |tap6   |  |  +---------| g(14) g(0)  g(1) ... g(13)  |-----+ |
       |    |------>|  |  |  +------| g(13) g(14) g(0) ... g(12)  |----+| |
       |____|   |   |  |  |  |      | ..........................  |    || |
                |   |  |  |  |      | ..........................  |....|| |
                |   |  |  |  |      | ..........................  |   .|| |
                |   |  |  |  |  +---| g(1)  g(2)  g(3) ... g(0)   |-+ .|| |
                |   |  |  |  |  |   |_____________________________| | .|| |
                |   |  |  |  |  |                                   | .|| |
                |   |  |  |  |  |                                   | .|| |
                |   |  |  |  |  |                                   | .|| |
                ----|-(+)-|--|--|-----(z0)--------------------------+-.||-+
                |   |     |  |  |                                   | .|| |
                ----|----(+)-|--|--------------(z1)-----------------|-.|+ |
                |   |        |  |                                   | .|| |
                ----|-------(+)-|--------------------(z2)-----------|-.+| |
                |   |           |                                   | .|| |
                ....|...........|...................................|.+|| |
                |   |           |                                   | .|| |
                ----|----------(+)-------------------------(z14)----+ .|| |
                    | tapline_out                                   (  +  )
                    |                                                  |
                    |                                                  |
                    |                                                  |
                    |             | early_diffuse                      |
                    |             v              (1 - early_diffuse)->(*)
                    |------------(*)----------------------------------(+)
                                                                       |
                                                                       |+-->

\*-------------------------------------------------------------------------*/

/* Changes by JPff: #define'd out include of "config.h"
                    Defined FLT_MAX
                    Move static fn declarations out of function
 */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "babo.h"
#include <math.h>
#include "interlocks.h"

#if !defined(FLT_MAX)
#define FLT_MAX         (1.0e38)
#endif

static const int32_t sound_speed = 330;

/*
 * private Babo tools
 */

#define square(x)   ((x)*(x))

/*
 * Memory allocation object methods
 *
 * (this object is a bit funny because it is in fact a wrapper of the
 * memory allocation functions in csound)
 */

static BaboMemory *
BaboMemory_create(CSOUND *csound, BaboMemory *this, size_t size_in_floats)
{
    size_t size_in_bytes = size_in_floats * sizeof(MYFLT);

    csound->AuxAlloc(csound, size_in_bytes, &this->memptr);

    //memset(this->memptr.auxp, 0, size_in_bytes);

    this->samples = size_in_floats;

    return this;
}

static inline size_t
BaboMemory_samples(const BaboMemory *this)
{
    return this->samples;
}

static inline MYFLT *
BaboMemory_start(const BaboMemory *this)
{
    return (MYFLT *) this->memptr.auxp;
}

static inline MYFLT *
BaboMemory_end(const BaboMemory *this)
{
    return (MYFLT *) this->memptr.endp;
}

/* static inline MYFLT * */
/* BaboMemory_size(const BaboMemory *this) */
/* { */
/*     return (MYFLT *) this->memptr.size; */
/* } */

/*
 * common delay/tapline methods
 */

static void
_Babo_common_delay_create(CSOUND *csound, BaboDelay *this, MYFLT max_time)
{
    size_t num_floats =
      (size_t)MYFLT2LRND((MYFLT)ceil((double)(max_time*this->sr)));

    BaboMemory_create(csound, &this->core, num_floats);
}

/*
 * Babo Delay object methods
 */

static BaboDelay *
BaboDelay_create(CSOUND *csound, BaboDelay *this, MYFLT max_time)
{
    _Babo_common_delay_create(csound, this, max_time);

    this->input = BaboMemory_start(&this->core);

    return this;
}

static MYFLT
BaboDelay_input(BaboDelay *this, MYFLT input)
{
     *this->input++ = input;

    if (this->input >= BaboMemory_end(&this->core))
        this->input -= BaboMemory_samples(&this->core);

    return input;
}

static MYFLT
BaboDelay_output(const BaboDelay *this)
{
    size_t num_samples = BaboMemory_samples(&this->core);
    MYFLT *output_ptr = this->input - (num_samples - 1);

    if (output_ptr < BaboMemory_start(&this->core))
        output_ptr += num_samples;

    return *output_ptr;
}

/*
 * Babo Tapline object methods
 */

/*
 * The tapline is created with a size that is dependent on the size
 * of the room (selected at i-time)
 */

static BaboTapline *
BaboTapline_create(CSOUND *csound, BaboTapline *this, MYFLT x, MYFLT y, MYFLT z)
{
    MYFLT max_time = (FL(2.0) * SQRT((x*x) + (y*y) + (z*z))) / sound_speed;

    _Babo_common_delay_create(csound, (BaboDelay *) this, max_time);

    this->input = BaboMemory_start(&this->core);

    return this;
}

static inline MYFLT
BaboTapline_maxtime(CSOUND *csound, BaboDelay *this)
{
  return (((MYFLT) BaboMemory_samples(&this->core)) * (1./this->sr));
}

static inline MYFLT
BaboTapline_input(BaboTapline *this, MYFLT input)
{
    return BaboDelay_input((BaboDelay *) this, input);
}

typedef struct
{
    MYFLT   attenuation;
    MYFLT   delay_size;
    MYFLT   sr;
} BaboTapParameter;

typedef struct
{
    BaboTapParameter    direct;
    BaboTapParameter    tap[BABO_TAPS];
} BaboTaplineParameters;

/*
 * BaboTapline_single_output:
 * this function calculates the delay output by doing linear interpolation
 * between the two adjacent integer sample delays of an otherwise fractional
 * delay time. This is why BaboTapParameter.delay_size is kept as a float
 * in the first place.
 */
/* a-rate function */
static MYFLT BaboTapline_single_output(const BaboTapline *this,
                                       const BaboTapParameter *pp)
{
        /*
         * the assignment right below should be really a floor(p->delay_size),
         * but apparently floor() calls are really expensive on some
         * architectures (notably Pentiums), so we do a simple cast to a
         * size_t instead. This should always work, but I cannot test it on
         * other architectures than mine (k6), so it potentially is a source
         * of problems. [nicb@axnet.it]
         */
    size_t delay_floor  = (size_t) pp->delay_size;
    size_t delay_ceil   = delay_floor + 1;
    MYFLT fractional    = pp->delay_size - (MYFLT) delay_floor;
    MYFLT *output_floor = this->input - delay_floor;
    MYFLT *output_ceil  = this->input - delay_ceil;
    MYFLT output        = FL(0.0);

    if (output_floor <  BaboMemory_start(&this->core))
        output_floor += BaboMemory_samples(&this->core);

    if (output_ceil <  BaboMemory_start(&this->core))
        output_ceil += BaboMemory_samples(&this->core);

    output = (*output_floor*(1-fractional)) + (*output_ceil*fractional);

    return output * pp->attenuation;
}

/* k-rate function */
static inline void BaboTapline_preload_parameter(CSOUND *csound,
                                                 BaboTapParameter *this,
                                                 MYFLT distance)
{
    /*
     * Direct sound parameters at the input of delay tap_lines.
     * Right and left direct_att is not really physical, but ensures that:
     *          direct_att=(1/2) when distance is 1 m
     *          direct_att=1     when distance is 0 m.
     */
    this->delay_size    = (distance / sound_speed) * this->sr;
    this->attenuation   = FL(1.0) / (FL(1.0) + distance);
}

/* k-rate function */
static BaboTaplineParameters *
BaboTapline_precalculate_parameters(
    CSOUND *csound, BaboTaplineParameters    *results,
    MYFLT r_x, MYFLT r_y, MYFLT r_z,    /* receiver position (i-rate) */
    MYFLT s_x, MYFLT s_y, MYFLT s_z,    /* source   position (k-rate) */
    MYFLT l_x, MYFLT l_y, MYFLT l_z)    /* room     coords   (i-rate) */
{
    MYFLT   sqr_xy, sqr_yz, sqr_xz, /* x^2+y^2  y^2+z^2 .......         */
            sqr_diff_x, sqr_diff_y, sqr_diff_z; /* optimization temps   */

    /* image method distance calculation */

    sqr_diff_x  = square(r_x - s_x);
    sqr_diff_y  = square(r_y - s_y);
    sqr_diff_z  = square(r_z - s_z);

    sqr_yz      = sqr_diff_y + sqr_diff_z;
    sqr_xz      = sqr_diff_x + sqr_diff_z;
    sqr_xy      = sqr_diff_x + sqr_diff_y;

    BaboTapline_preload_parameter(csound, &results->direct,
                                  SQRT(sqr_diff_x + sqr_yz));

    BaboTapline_preload_parameter(csound, &results->tap[0],
                                  SQRT(square(l_x + r_x + s_x) + sqr_yz));
    BaboTapline_preload_parameter(csound, &results->tap[1],
                                  SQRT(square(l_x - r_x - s_x) + sqr_yz));
    BaboTapline_preload_parameter(csound, &results->tap[2],
                        SQRT(sqr_xz + square(l_y - r_y - s_y)));
    BaboTapline_preload_parameter(csound, &results->tap[3],
                        SQRT(sqr_xz + square(l_y + r_y + s_y)));

    BaboTapline_preload_parameter(csound,
                                  &results->tap[4],
                                  SQRT(sqr_xy + square(l_z - r_z - s_z)));
    BaboTapline_preload_parameter(csound,
                                  &results->tap[5],
                                  SQRT(sqr_xy + square(l_z + r_z + s_z)));

    return results;
}

/* a-rate function */
static MYFLT
BaboTapline_output(CSOUND *csound, const BaboTapline *this,
                   const BaboTaplineParameters *pars)
{
    IGN(csound);
    int32_t     i;
    MYFLT   output = BaboTapline_single_output(this, &pars->direct);

    for (i = 0; i < BABO_TAPS; ++i)
      output  += BaboTapline_single_output(this, &pars->tap[i]);

    return output;
}

static MYFLT
BaboTapline_output2(CSOUND *csound, const BaboTapline *this,
                    const BaboTaplineParameters *pars, MYFLT dir)
{
    IGN(csound);
    int32_t     i;
    MYFLT   output = BaboTapline_single_output(this, &pars->direct)*dir;

    for (i = 0; i < BABO_TAPS; ++i)
      output  += BaboTapline_single_output(this, &pars->tap[i]);

    return output;
}

/*
 * Babo lowpass filter object methods
 */

static BaboLowPass *
BaboLowPass_create(BaboLowPass *this, MYFLT decay, MYFLT hidecay, MYFLT norm)
{
    MYFLT real_decay    = EXP(norm * LOG(decay));
    MYFLT real_hidecay  = EXP(norm * LOG(hidecay));

    this->a0 = (real_decay + real_hidecay) * FL(0.25);
    this->a1 = (real_decay - real_hidecay) * FL(0.5);
    this->z1 = this->z2 = FL(0.0);

    return this;
}

static inline MYFLT
BaboLowPass_input(BaboLowPass *this, MYFLT input)
{
    this->z2 = this->z1;
    this->z1 = this->input;
    this->input = input;
    return input;
}
static inline MYFLT
BaboLowPass_output(const BaboLowPass *this)
{
    return  (this->a0 * this->input)    +
            (this->a1 * this->z1)       +
            (this->a0 * this->z2);
}
/*
 * Babo node object methods
 */

static BaboNode *
BaboNode_create(CSOUND *csound, BaboNode *this, MYFLT time,
                MYFLT min_time, MYFLT decay,
    MYFLT hidecay)
{
    BaboDelay_create(csound, &this->delay, time);
    BaboLowPass_create(&this->filter, decay, hidecay, time/min_time);

    return this;
}

static inline MYFLT
BaboNode_input(BaboNode *this, MYFLT input)
{
    return BaboDelay_input(&this->delay, input);
}

static inline void
BaboNode_feed_filter(BaboNode *this)
{
    BaboLowPass_input(&this->filter, BaboDelay_output(&this->delay));
}

static inline MYFLT
BaboNode_output(const BaboNode *this)
{
    return BaboLowPass_output(&this->filter);
}

/*
 * Babo Matrix object methods
 */

static void
BaboMatrix_create_FDN(BaboMatrix *this, MYFLT diffusion)
{
    int32_t  i,j;
    MYFLT _2PI_NODES = TWOPI_F / BABO_NODES;
    /*
     * The following sequence of eigenvalues provides, by IDFT,
     * the maximally diffusive sequence, i.e. a row of the circulant
     * feedback matrix.
     * eigenvalues are expressed in radians, because only the argument
     * is expressed, since the magnitude is one
     */

    const MYFLT  max_diffusion_eigenvalues[BABO_NODES]=
    {
        FL(3.142592),
       -FL(1.7370),
       -FL(2.1559),
       -FL(1.2566),
       -FL(2.9936),
        FL(1.0472),
       -FL(2.5133),
       -FL(1.6140),
        FL(1.6140),
        FL(2.5133),
       -FL(1.0472),
        FL(2.9936),
        FL(1.2566),
        FL(2.1559),
        FL(1.7370)
    };

    /*
     * Here we multiply the arguments of the sequence of eigenvalues by
     * p->idiffusion_coeff. In this way we scale the amount of diffusion
     * Range of diffusion:   0 = no diffusion
     *                       1 = maximum diffusion
     */

    MYFLT  real_X[BABO_NODES]       = { FL(0.0) },
           imaginary_X[BABO_NODES]  = { FL(0.0) },
           arg_X[BABO_NODES]        = { FL(0.0) },
           real_x[BABO_NODES]       = { FL(0.0) };

    for (i = 0; i < BABO_NODES; ++i)
    {
        real_X[i] = imaginary_X[i] = FL(0.0);
        arg_X[i]  = max_diffusion_eigenvalues[i] * diffusion;
        real_X[i] = COS(arg_X[i]);
        imaginary_X[i] = SIN(arg_X[i]);
    }

    /*
     * The Real part of the InverseDFT of the eigenvalues supplies the
     * circulant matrix coefficients.
     */

    for (i = 0; i < BABO_NODES; ++i)
        for (j = 0; j < BABO_NODES; ++j)
            real_x[j] += (real_X[j] * COS(_2PI_NODES*i*j)-
                     imaginary_X[j] * SIN(_2PI_NODES*i*j))/BABO_NODES;

    for (i = 0; i < BABO_NODES; ++i)
        for (j = 0; j < BABO_NODES; ++j)
            this->fdn[i][j] = real_x[(j-i+15) % BABO_NODES];
}

static MYFLT
BaboMatrix_calculate_delays(MYFLT delay_time[], MYFLT x, MYFLT y, MYFLT z)
{
    int32_t i = 0;
    MYFLT min = FL(0.0);

    static const struct babo_diffusion_constants
    {
        int32_t x, y, z;

    } BABO_DIRECTIONS[] =
    {
        /*
         * Each triplet is a mode identifier.
         * E.g. {1,0,0} is the first axial mode
         */
        { 1, 0, 0 },
        { 2, 1, 0 },
        { 1, 1, 0 },
        { 1, 2, 0 },
        { 0, 1, 0 },
        { 0, 2, 1 },
        { 0, 1, 1 },
        { 0, 1, 2 },
        { 0, 0, 1 },
        { 1, 0, 2 },
        { 1, 0, 1 },
        { 1, 1, 1 },
        { 1, 2, 1 },
        { 2, 1, 1 },
        { 2, 0, 1 }
    };
    /*
     * we calculate the delays related to each node, in the following
     * way:
     *
     *                                   2
     * delay[i] = -----------------------------------------------
     *                      +-----------------------------------
     *    sound_speed * \  / |       |2  |       |2  |       |2
     *                   \/  |  x[i] |   |  y[i] |   |  z[i] |
     *                       | ----- | + | ----- | + | ----- |
     *                       |   X   |   |   Y   |   |   Z   |
     *
     * and we keep a notion of the minimum delay path which is
     * needed later on to do the rescaling of the decay and hidecay
     * parameters.
     */
    min = (MYFLT)FLT_MAX; /* let's initialize this with something really big */

    for (i = 0; i < BABO_NODES; ++i)
    {
        const struct babo_diffusion_constants *dbdp = &BABO_DIRECTIONS[i];
        delay_time[i] = FL(2.0) / (sound_speed *
                        SQRT(((dbdp->x/x) * (dbdp->x/x)) +
                             ((dbdp->y/y) * (dbdp->y/y)) +
                             ((dbdp->z/z) * (dbdp->z/z))));

        min = min > delay_time[i] ? delay_time[i] : min;
    }
    return min;
}

static BaboMatrix *
BaboMatrix_create(CSOUND *csound,
                  BaboMatrix *this, MYFLT diffusion, MYFLT x, MYFLT y,
                  MYFLT z, MYFLT decay, MYFLT hidecay, MYFLT early_diffusion)
{
    int32_t i = 0;
    MYFLT delays[BABO_NODES];
    MYFLT min_delay = BaboMatrix_calculate_delays(delays, x, y, z);

    this->complementary_early_diffusion = FL(1.0) - early_diffusion;

    BaboMatrix_create_FDN(this, diffusion);

    for (i = 0; i < BABO_NODES; ++i)
        BaboNode_create(csound, &this->node[i], delays[i],
                        min_delay, decay, hidecay);

    return this;
}

static inline MYFLT
BaboMatrix_coefficient(const BaboMatrix *this, int32_t x, int32_t y)
{
    return this->fdn[x][y];
}

/* a-rate function */
static void
BaboMatrix_output(BaboMatrix *this, MYFLT outputs[], MYFLT input,
    MYFLT diffusion_coeff)
{
    MYFLT            filter_tmpout[BABO_NODES]  = { FL(0.0) },
                     tmp2[BABO_NODES]           = { FL(0.0) };
    register int32_t     i = 0, j = 0;

    for (i = 0; i < BABO_NODES; ++i)
    {
        filter_tmpout[i] = BaboNode_output(&this->node[i]);
        BaboNode_feed_filter(&this->node[i]);
    }

    /*
     * Here the matrix-by-vector multiply takes place, xout is the
     * column vector.
     * The mod(BABO_NODES) operation allows to write the circulant matrix-
     * by-vector multiply in a compact way.
     */

    for (i = 0; i < BABO_NODES; ++i)
    {
        for (j = 0; j < BABO_NODES; ++j)
            tmp2[i] += BaboMatrix_coefficient(this, i, j) * filter_tmpout[j];

        BaboNode_input(&this->node[i], tmp2[i] + input);
        /* We add delayed signal at the input of ^^^^^  the delay lines. */
    }

    outputs[0] = outputs[1] =   BaboNode_output(&this->node[0]) +
                                BaboNode_output(&this->node[4]) +
                                BaboNode_output(&this->node[8]);

    outputs[0] +=   (diffusion_coeff *
                    ((BaboNode_output(&this->node[7]) +
                      BaboNode_output(&this->node[12]))));

    outputs[1] +=   (diffusion_coeff *
                    ((BaboNode_output(&this->node[9]) +
                      BaboNode_output(&this->node[13]))));

    outputs[0] *=   this->complementary_early_diffusion;
    outputs[1] *=   this->complementary_early_diffusion;
}

/*
 * private utility functions
 */

static void resolve_defaults(BABO *p);
static void set_expert_values(CSOUND *csound, BABO *p);

static void
set_defaults(CSOUND *csound, BABO *p)
{
    resolve_defaults(p);

    p->diffusion_coeff = p->diffusion_coeff < 0 ?
                         BABO_DEFAULT_DIFFUSION_COEFF : p->diffusion_coeff;

    set_expert_values(csound,p);

    /*
     * the user supplies, optionally, the complete distance
     * or else it is set by default
     * we divide by two because it is more handy to deal
     * with half the distance in the program (i.e. the distance
     * from a center point)
     */
    p->inter_receiver_distance *= FL(0.5);

}

static void
resolve_defaults(BABO *p)
{
    /*
     * in typical csound backward logic :), the defaults may or may not,
     * depending on how they're used in the orchestra definition,
     * turn out to run on the same pointer - so, basically all optional
     * argument values have to be copied (from last to first) in separate
     * "real" values inside the entry structure in order to be used
     */

    p->expert_values    = *(p->oexpert_values);
    p->diffusion_coeff  = *(p->odiffusion_coeff);
}

static inline MYFLT
load_value_or_default(const FUNC *table, int32_t idx, MYFLT dEfault)
{
    MYFLT result = (table != (FUNC *) NULL && idx < (int32)table->flen) ?
                   table->ftable[idx] : dEfault;

    return result;
}

static void
set_expert_values(CSOUND *csound, BABO *p)
{
    FUNC    *ftp    = (FUNC *) NULL; /* brain-damaged function calling */
    int32_t      n      = 0;

    if (p->expert_values > 0)
        ftp = csound->FTFind(csound, &(p->expert_values));

    p->decay        = load_value_or_default(ftp, n++, BABO_DEFAULT_DECAY);
    p->hidecay      = load_value_or_default(ftp, n++, BABO_DEFAULT_HIDECAY);
    p->receiver_x   = load_value_or_default(ftp, n++, BABO_DEFAULT_RECV_X);
    p->receiver_y   = load_value_or_default(ftp, n++, BABO_DEFAULT_RECV_Y);
    p->receiver_z   = load_value_or_default(ftp, n++, BABO_DEFAULT_RECV_Z);
    p->inter_receiver_distance = load_value_or_default(ftp, n++,
                                            BABO_DEFAULT_INTER_RECV_DISTANCE);
    p->direct       = load_value_or_default(ftp, n++, BABO_DEFAULT_DIRECT);
    p->early_diffuse= load_value_or_default(ftp, n++, BABO_DEFAULT_DIFFUSE);
}

static void
verify_coherence(CSOUND *csound, BABO *p)
{
    if (UNLIKELY(*(p->lx) <= FL(0.0) ||
                 *(p->ly) <= FL(0.0) ||
                 *(p->lz) <= FL(0.0))) {
      csound->Warning(csound, Str("Babo: resonator dimensions are incorrect "
                              "(%.1f, %.1f, %.1f)"),
                  *(p->lx), *(p->ly), *(p->lz));
    }
}

/*
 * PUBLIC FUNCTIONS - baboset(), babo()
 *
 * these get called from the csound engine
 *
 */

static int32_t
baboset(CSOUND *csound, void *entry)
{
    BABO *p = (BABO *) entry;   /* assuming the engine is right... :)   */
    p->tapline.sr = CS_ESR;
    p->matrix_delay.sr = CS_ESR;
    set_defaults(csound,p);
    verify_coherence(csound,p);        /* exits if call is wrong */
    
    BaboTapline_create(csound,&p->tapline, *(p->lx), *(p->ly), *(p->lz));
    BaboDelay_create(csound, &p->matrix_delay,
                     BaboTapline_maxtime(csound, &p->tapline));
    BaboMatrix_create(csound, &p->matrix, p->diffusion_coeff, *(p->lx),
                      *(p->ly), *(p->lz), p->decay, p->hidecay, p->early_diffuse);
    return OK;
}

static int32_t
babo(CSOUND *csound, void *entry)
{
    BABO    *p          = (BABO *) entry;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *outleft    = p->outleft,
            *outright   = p->outright,
            *input      = p->input;

    BaboTaplineParameters left = { {FL(0.0)}, {{FL(0.0)}} },
                          right = { {FL(0.0)}, {{FL(0.0)}} };

    BaboTapline_precalculate_parameters(csound, &left,
                                        p->receiver_x - p->inter_receiver_distance,
                                        p->receiver_y, p->receiver_z,
                                        *(p->ksource_x), *(p->ksource_y),
                                        *(p->ksource_z),
                                        *(p->lx), *(p->ly), *(p->lz));

    BaboTapline_precalculate_parameters(csound, &right,
        p->receiver_x + p->inter_receiver_distance,
        p->receiver_y, p->receiver_z,
        *(p->ksource_x), *(p->ksource_y), *(p->ksource_z),
        *(p->lx), *(p->ly), *(p->lz));

    if (UNLIKELY(offset)) {
      memset(outleft,  '\0', offset*sizeof(MYFLT));
      memset(outright, '\0', offset*sizeof(MYFLT));
    } if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outleft[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outright[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {         /* k-time cycle                */
      MYFLT  left_tapline_out        = FL(0.0),
             right_tapline_out       = FL(0.0),
             delayed_matrix_input    = FL(0.0);
      MYFLT  matrix_outputs[2]       = { FL(0.0) };

      BaboTapline_input(&p->tapline, input[n]);
      BaboDelay_input(&p->matrix_delay, input[n]);

      left_tapline_out  = BaboTapline_output(csound, &p->tapline, &left) *
        p->early_diffuse;

      right_tapline_out  = BaboTapline_output(csound, &p->tapline, &right) *
        p->early_diffuse;

      delayed_matrix_input = BaboDelay_output(&p->matrix_delay);

      BaboMatrix_output(&p->matrix, matrix_outputs, delayed_matrix_input,
                        p->diffusion_coeff);

      outleft[n]  = left_tapline_out  + matrix_outputs[0];
      outright[n] = right_tapline_out + matrix_outputs[1];
    }
    return OK;
}

/*
typedef struct
{
    BaboTapParameter    direct;
    BaboTapParameter    tap[BABO_TAPS];
} BaboTaplineParameters;
*/

static int32_t
babo2(CSOUND *csound, void *entry)
{
    BABO    *p          = (BABO *) entry;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   *outleft    = p->outleft,
            *outright   = p->outright,
            *input      = p->input;
    int32_t i;

    BaboTaplineParameters left = { {FL(0.0)}, {{FL(0.0)}} },
                          right = { {FL(0.0)}, {{FL(0.0)}} };
    left.direct.sr = CS_ESR;
    right.direct.sr = CS_ESR;
    for(i = 0; i < BABO_TAPS; i++)
      left.tap[i].sr = right.tap[i].sr = CS_ESR;

    BaboTapline_precalculate_parameters(csound, &left,
                                        p->receiver_x - p->inter_receiver_distance,
                                        p->receiver_y, p->receiver_z,
                                        *(p->ksource_x), *(p->ksource_y),
                                        *(p->ksource_z),
                                        *(p->lx), *(p->ly), *(p->lz));

    BaboTapline_precalculate_parameters(csound, &right,
        p->receiver_x + p->inter_receiver_distance,
        p->receiver_y, p->receiver_z,
        *(p->ksource_x), *(p->ksource_y), *(p->ksource_z),
        *(p->lx), *(p->ly), *(p->lz));

    if (UNLIKELY(offset)) {
      memset(outleft,  '\0', offset*sizeof(MYFLT));
      memset(outright, '\0', offset*sizeof(MYFLT));
    } if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outleft[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outright[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {         /* k-time cycle                */
      MYFLT  left_tapline_out        = FL(0.0),
             right_tapline_out       = FL(0.0),
             delayed_matrix_input    = FL(0.0);
      MYFLT  matrix_outputs[2]       = { FL(0.0) };

      BaboTapline_input(&p->tapline, input[n]);
      BaboDelay_input(&p->matrix_delay, input[n]);

      left_tapline_out  = BaboTapline_output2(csound, &p->tapline, &left, p->direct) *
        p->early_diffuse;

      right_tapline_out  = BaboTapline_output2(csound, &p->tapline, &right, p->direct) *
        p->early_diffuse;

      delayed_matrix_input = BaboDelay_output(&p->matrix_delay);

      BaboMatrix_output(&p->matrix, matrix_outputs, delayed_matrix_input,
                        p->diffusion_coeff);

      outleft[n]  = left_tapline_out  + matrix_outputs[0];
      outright[n] = right_tapline_out + matrix_outputs[1];
    }
    return OK;
}



#define S(x)    sizeof(x)

static OENTRY babo_localops[] = {
  { "babo",   S(BABO), TR, "aa", "akkkiiijj",(SUBR)baboset, (SUBR)babo   },
  { "babo2",  S(BABO), TR,  "aa", "akkkiiijj",(SUBR)baboset, (SUBR)babo2 }  
};

LINKAGE_BUILTIN(babo_localops)
