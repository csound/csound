/*
    dbap.h:

    Copyright (C) 2026 Pasquale Mainolfi

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/*
    This opcode implements the algorithm described in:

    T. Lossius, P. Baltazar, and T. de la Hogue, "DBAP - Distance-Based Amplitude Panning",
    Proceedings of the International Computer Music Conference (ICMC), 2009.

    The implementation follows the original formulation with optional
    extensions for soft-limiting and spatial blur.
*/

#ifndef DBAP_H
#define DBAP_H

#include "csound.h"
#include "stdopcod.h"
#include "sysdep.h"
#include <csdl.h>
#include <stdint.h>

#define CARTESIAN_KIND 0
#define DEGREE_KIND 1
#define RADIANS_KIND 2

typedef struct {
    MYFLT x;
    MYFLT y;
    MYFLT z;
} CARTESIAN_COORD;

typedef struct {
    MYFLT rho;
    MYFLT phi;
    MYFLT theta;
} POLAR_COORD;

typedef struct {
    AUXCH lpos;
    AUXCH weights;
    AUXCH temp_b;
    AUXCH temp_u;
    // 0 -> current gains, n -> previous gains, 3 * n -> temp gains
    AUXCH lgains;
    // 0 -> distances, n -> sorted distances, 2 * n + 1 -> max distance, 2 * n + 2 -> mean distance
    AUXCH distances;
    AUXCH internal_out_frame;

    CARTESIAN_COORD center;
    MYFLT spatial_blur;
    MYFLT eta;
    MYFLT a;
    int32_t nchnls;
    int32_t ncoords;
    int32_t coord_kind;
} DBAP_STATE;

typedef struct  {
    OPDS h;

    // output channels
    ARRAYDAT *out;

    // inputs
    MYFLT *input_frame;
    MYFLT *coord_mode; // coordinates kind 0 -> cartesian, 1 -> polar degree, 2 -> polar radians
    ARRAYDAT *source;
    ARRAYDAT *loudspeaker_pos;
    MYFLT *spread; // spread amount 0 -> 1 (0 -> dbap classis, 1 -> focus)
    MYFLT *rolloff_value;
    ARRAYDAT *loudspeakers_weights; // optional in init with NULL weights

    // internal
    DBAP_STATE dbap_state;
} DBAP_WITH_ARR;

typedef struct  {
    OPDS h;

    // output channels
    ARRAYDAT *out;

    // inputs
    MYFLT *coord_mode; // coordinates kind 0 -> cartesian, 1 -> polar degree, 2 -> polar radians
    ARRAYDAT *source;
    ARRAYDAT *loudspeaker_pos;
    MYFLT *spread; // spread amount 0 -> 1 (0 -> dbap classis, 1 -> focus)
    MYFLT *rolloff_value;
    ARRAYDAT *loudspeakers_weights; // optional in init with NULL weights

    // internal
    DBAP_STATE dbap_state;
} DBAP_GAINS_WITH_ARR;

typedef struct  {
    OPDS h;

    // output channels
    ARRAYDAT *out;

    // inputs
    MYFLT *input_frame;
    MYFLT *coord_mode; // coordinates kind 0 -> cartesian, 1 -> polar, 2 -> polar radians
    ARRAYDAT *source;
    MYFLT *loudspeaker_dimension; // 2 = 2D, 3 = 3D
    MYFLT *loudspeaker_pos;
    MYFLT *spread; // spread amount 0 -> 1 (0 -> dbap classis, 1 -> focus)
    MYFLT *rolloff_value;
    ARRAYDAT *loudspeakers_weights; // optional in init with NULL weights

    // internal
    DBAP_STATE dbap_state;
} DBAP_WITH_FUNC;

typedef struct  {
    OPDS h;

    // output channels
    ARRAYDAT *out;

    // inputs
    MYFLT *coord_mode; // coordinates kind 0 -> cartesian, 1 -> polar, 2 -> polar radians
    ARRAYDAT *source;
    MYFLT *loudspeaker_dimension; // 2 = 2D, 3 = 3D
    MYFLT *loudspeaker_pos;
    MYFLT *spread; // spread amount 0 -> 1 (0 -> dbap classis, 1 -> focus)
    MYFLT *rolloff_value;
    ARRAYDAT *loudspeakers_weights; // optional in init with NULL weights

    // internal
    DBAP_STATE dbap_state;
} DBAP_GAINS_WITH_FUNC;

int32_t prepare_dbap_helper(
    CSOUND *csound,
    DBAP_STATE *dbap_state,
    MYFLT *input_coords,
    MYFLT *weights,
    int32_t nchnls,
    int32_t npos,
    int32_t ncoords,
    int32_t coord_kind,
    MYFLT rolloff_value
);

int32_t dbap_helper(
    CSOUND *csound,
    OPDS *h,
    DBAP_STATE *dbap_state,
    MYFLT spread,
    int32_t coord_mode,
    int32_t source_size,
    MYFLT *source_array,
    MYFLT *input_frame,
    MYFLT *out
);

int32_t initialize_dbap(
    CSOUND *csound,
    DBAP_STATE *dbap,
    MYFLT *input_coords,
    int32_t nchnls,
    int32_t ncoords,
    MYFLT rolloff,
    MYFLT *weights,
    int32_t nsamples,
    int32_t coord_kind
);
int32_t prepare_dbap_with_arr(CSOUND *csound, DBAP_WITH_ARR *dbap);
int32_t prepare_dbap_with_arr_no_weights(CSOUND *csound, DBAP_WITH_ARR *dbap);
int32_t prepare_dbap_with_func(CSOUND *csound, DBAP_WITH_FUNC *dbap);
int32_t prepare_dbap_with_func_no_weights(CSOUND *csound, DBAP_WITH_FUNC *dbap);
int32_t dbap_with_arr(CSOUND *csound, DBAP_WITH_ARR *sdbap);
int32_t dbap_with_func(CSOUND *csound, DBAP_WITH_FUNC *sdbap);

int32_t prepare_dbap_gains_with_arr(CSOUND *csound, DBAP_GAINS_WITH_ARR *dbap);
int32_t prepare_dbap_gains_with_arr_no_weights(CSOUND *csound, DBAP_GAINS_WITH_ARR *dbap);
int32_t prepare_dbap_gains_with_func(CSOUND *csound, DBAP_GAINS_WITH_FUNC *dbap);
int32_t prepare_dbap_gains_with_func_no_weights(CSOUND *csound, DBAP_GAINS_WITH_FUNC *dbap);
int32_t dbap_gains_with_arr(CSOUND *csound, DBAP_GAINS_WITH_ARR *sdbap);
int32_t dbap_gains_with_func(CSOUND *csound, DBAP_GAINS_WITH_FUNC *sdbap);

#endif
