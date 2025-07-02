/*
    babo.h:

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

/*-------------------------------------------------------------------------*\
 * vi:set nowrap ts=4:
 *
 * $Id$
 *
 * Header file for babo.c                                                  *
 * We choose 15  nodes for the BABO structure tryng to achieve             *
 * sound quality results and minimal computational load according the rule *
 * that the nodes must be  (2^N-1)                                         *
 *
\*------------------------------------------------------------------------*/

/* Changes by JPff: Fixed constant  BABO_DEFAULT_DIFFUSION_COEFF */

#if !defined(__babo_h__)
#   define  __babo_h__

#define BABO_NODES     (15) /* Number of nodes of feedback delay network    */
#define BABO_TAPS       (6) /* Number of taps in the early reflections line */

typedef struct
{
    AUXCH   memptr;
    size_t  samples;
} BaboMemory;

typedef struct
{
    BaboMemory   core;
    MYFLT       *input;
    MYFLT         sr;
} BaboDelay;

typedef BaboDelay BaboTapline;

typedef struct
{
    MYFLT   a0,
            a1,
            z1,
            z2,
            input;
} BaboLowPass;

typedef struct
{
    BaboDelay   delay;
    BaboLowPass filter;
} BaboNode;

typedef struct
{
    MYFLT    complementary_early_diffusion;
    MYFLT    fdn[BABO_NODES][BABO_NODES];
    BaboNode node[BABO_NODES] ;
} BaboMatrix;

/*
 * The opcode functionality should be:
 *
 * ar,al babo asig,kx,ky,kz,ix,iy,iz[,idiff[,ifno]]
 *
 * where - mandatory arguments:
 *
 * kx,ky,kz     - position of the source
 * ix,iy,iz     - dimensions of the resonator (room)
 *
 *         optional arguments:
 *
 * idiff        - diffusion coefficient (a number from 0 to 1: default 1)
 * ifno         - number of the GEN 02 function which holds all the other
 *                defaulted parameters
 *
 * the optional size-8 gen 02 function may hold the following parameters:
 *
 * fn 0 8 2 decay hidecay rx ry rz rdistance direct early_diff
 *
 * where:
 *
 * decay        - global decay coefficient (a number from 0 to 1: default 0.99)
 * hidecay      - hi-freq decay coefficient (a number from 0 to 1: default 0.1)
 * rx,ry,rz     - position of the receiver (default: 0, 0, 0)
 * rdistance    - x distance between stereo receivers (e.g.: ears :)
 *                (default: 0.3)
 * direct       - direct delayed signal coefficient (default: 0.5)
 * early_diff   - diffusion coefficient of the early reflections (default: 0.8)
 *
 */

typedef struct
{
    OPDS        h;                              /* defined in cs.h      */
                /* output args          */
    MYFLT       *outleft,*outright,
                /* start input args     */
                *input,
                *ksource_x,*ksource_y,*ksource_z,
                *lx,*ly,*lz,
                /* optional args */
                *odiffusion_coeff,
                *oexpert_values;
                /* end input args       */
                /* backward-logic optional args copy */
    MYFLT       diffusion_coeff,
                expert_values;
                /* internal values */
    MYFLT       decay, hidecay,
                receiver_x, receiver_y, receiver_z,
                inter_receiver_distance,
                direct,
                early_diffuse;
    BaboTapline tapline;                        /* early reflections    */
    BaboDelay   matrix_delay;                   /* diffusion delay      */
    BaboMatrix  matrix;                         /* diffusion            */
} BABO;

#define BABO_DEFAULT_DIFFUSION_COEFF        (FL(1.0))
#define BABO_DEFAULT_DECAY                  (FL(0.99))
#define BABO_DEFAULT_HIDECAY                (FL(0.1))
#define BABO_DEFAULT_RECV_X                 (FL(0.0))
#define BABO_DEFAULT_RECV_Y                 (FL(0.0))
#define BABO_DEFAULT_RECV_Z                 (FL(0.0))
#define BABO_DEFAULT_INTER_RECV_DISTANCE    (FL(0.3))
#define BABO_DEFAULT_DIRECT                 (FL(0.5))
#define BABO_DEFAULT_DIFFUSE                (FL(0.8))

#endif /* !defined(__babo_h__) */

