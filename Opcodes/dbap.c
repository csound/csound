/*
    dbap.c:

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

/*******************************************************************************************
    This opcode implements the algorithm described in:

    T. Lossius, P. Baltazar, and T. de la Hogue, "DBAP - Distance-Based Amplitude Panning",
    Proceedings of the International Computer Music Conference (ICMC), 2009.

    The implementation follows the original formulation with optional
    extensions for soft-limiting and spatial blur.
*******************************************************************************************/

#include "stdopcod.h"
#include "dbap.h"
#include "coreDefs.h"
#include "csoundCore.h"
#include "csound.h"
#include "sysdep.h"
#include <stdint.h>
#include <string.h>

#define DB_RATIO FL(6.02059991327962)
#define TO_RAD(x) ((x) * PI / FL(180.0))

static inline double wrap_angle(MYFLT x) {
    x = FMOD(x, FL(360.0));
    if (x < FL(0.0)) x += FL(360.0);
    return x;
}

static inline MYFLT clamp_elevation(MYFLT x) {
    if (x > FL(90.0)) return FL(90.0);
    if (x < FL(-90.0)) return FL(-90.0);
    return x;
}

CARTESIAN_COORD pol_to_car(const POLAR_COORD *p, int32_t polar_mode) {
    CARTESIAN_COORD pol;

    MYFLT phi = p->phi;
    MYFLT theta = p->theta;
    if (polar_mode == DEGREE_KIND) {
        phi = TO_RAD(wrap_angle(p->phi));
        theta = TO_RAD(clamp_elevation(p->theta));
    }

    pol.x = p->rho * COS(theta) * COS(phi);
    pol.y = p->rho * COS(theta) * SIN(phi);
    pol.z = p->rho * SIN(theta);

    return pol;
};

static inline MYFLT get_distance(CARTESIAN_COORD a, CARTESIAN_COORD b, MYFLT spatial_blur) {
    spatial_blur = spatial_blur <= FL(0.0) ? FL(0.0) : spatial_blur * spatial_blur;

    MYFLT dx = (a.x - b.x) * (a.x - b.x);
    MYFLT dy = (a.y - b.y) * (a.y - b.y);
    MYFLT dz = (a.z - b.z) * (a.z - b.z);

    MYFLT dist = dx + dy + dz;
    return SQRT(dist + spatial_blur);
}

static inline void swap(MYFLT *a, MYFLT *b) {
    MYFLT temp = *a;
    *a = *b;
    *b = temp;
}

void build_source(CARTESIAN_COORD *source, MYFLT *input_source, int32_t source_size, int32_t coord_mode) {
    MYFLT rho;
    MYFLT phi;
    MYFLT theta;
    if (coord_mode != CARTESIAN_KIND) {
        if (source_size == 2) {
            rho = FL(1.0);
            phi = input_source[0];
            theta = input_source[1];
        } else {
            rho = input_source[0];
            phi = input_source[1];
            theta = input_source[2];
        }
        POLAR_COORD p = { rho, phi, theta };
        *source = pol_to_car(&p, coord_mode);
    } else {
        MYFLT z = (source_size == 2) ? FL(0.0) : input_source[2];
        *source = (CARTESIAN_COORD) { input_source[0], input_source[1], z };
    }
}

static int32_t partition(MYFLT *arr, int32_t left, int32_t right) {
    int32_t index = left;
    MYFLT pivot = arr[right];
    for (int32_t i = left; i < right; i++) {
        if (arr[i] <= pivot) {
            swap(&arr[index], &arr[i]);
            index++;
        }
    }
    swap(&arr[index], &arr[right]);
    return index;
}

static MYFLT quickselect(MYFLT *arr, int32_t n, int32_t k) {
    int32_t left = 0;
    int32_t right = n - 1;

    while (left <= right) {
        if (left == right) return arr[left];

        int32_t pivot_index = partition(arr, left, right);

        if (k == pivot_index) return arr[pivot_index];
        if (k > pivot_index) {
            left = pivot_index + 1;
        } else {
            right = pivot_index - 1;
        }
    }
    return arr[k];
}

int32_t init_dbap(CSOUND *csound, DBAP_STATE *dbap, int32_t n, int32_t ncoords, MYFLT rolloff, MYFLT *weights, int32_t nsamples, int32_t coord_kind) {
    if (n < 2) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Need at least 2 loudspeakers\n");
    }

    dbap->nchnls = n;
    dbap->ncoords = ncoords;
    dbap->coord_kind = coord_kind;

    csound->AuxAlloc(csound, sizeof(CARTESIAN_COORD) * n, &dbap->lpos);
    csound->AuxAlloc(csound, sizeof(MYFLT) * (n * 3), &dbap->lgains);
    MYFLT *gains_block = (MYFLT *)dbap->lgains.auxp;
    memset(gains_block, 0, sizeof(MYFLT) * (3 * n));

    csound->AuxAlloc(csound, sizeof(MYFLT) * (n * 2 + 2), &dbap->distances);
    MYFLT *distances_block = (MYFLT *)dbap->distances.auxp;
    memset(distances_block, 0, sizeof(MYFLT) * (2 * n + 2));

    csound->AuxAlloc(csound, sizeof(MYFLT) * n, &dbap->temp_b);
    memset(dbap->temp_b.auxp, 0, sizeof(MYFLT) * n);

    csound->AuxAlloc(csound, sizeof(MYFLT) * n, &dbap->temp_u);
    memset(dbap->temp_u.auxp, 0, sizeof(MYFLT) * n);

    dbap->a = rolloff / DB_RATIO;
    dbap->spatial_blur = FL(0.0);
    dbap->eta = FL(0.0);

    csound->AuxAlloc(csound, sizeof(MYFLT) * (n * nsamples), &dbap->internal_out_frame);
    memset(dbap->internal_out_frame.auxp, 0, sizeof(MYFLT) * (n * nsamples));

    csound->AuxAlloc(csound, sizeof(MYFLT) * n, &dbap->weights);
    MYFLT *weights_block = (MYFLT *)dbap->weights.auxp;

    if (weights) {
        for (int i = 0; i < n; i++) weights_block[i] = (MYFLT)weights[i];
    } else {
        for (int i = 0; i < n; i++) weights_block[i] = FL(1.0);
    }

    return OK;
}

void update_loudspeaker_distances(DBAP_STATE *dbap, const CARTESIAN_COORD *source, MYFLT spatial_blur) {
    MYFLT *distances = (MYFLT *)dbap->distances.auxp;
    CARTESIAN_COORD *lpos = (CARTESIAN_COORD *)dbap->lpos.auxp;
    distances[2 * dbap->nchnls] = FL(0.0);
    distances[2 * dbap->nchnls + 1] = FL(0.0);

    MYFLT max_dist = 0.0;
    MYFLT mean_dist = 0.0;
    for (int i = 0; i < dbap->nchnls; i++) {
        MYFLT d =  get_distance(lpos[i], *source, spatial_blur);
        distances[i] = FL(d);
        max_dist = d > max_dist ? d : max_dist;
        mean_dist += d;
    }

    mean_dist /= (MYFLT)dbap->nchnls;

    distances[2 * dbap->nchnls] = max_dist;
    distances[2 * dbap->nchnls + 1] = mean_dist;

}

MYFLT get_spatial_blur(DBAP_STATE *dbap) {
    MYFLT *distances = (MYFLT *)dbap->distances.auxp;
    update_loudspeaker_distances(dbap, &dbap->center, 0.0);
    MYFLT sb = FL(0.0);
    for (int i = 0; i < dbap->nchnls; i++) {
        sb += distances[i];
    }
    return (sb / dbap->nchnls) + FL(0.2);
}

void set_loudspeaker_position(CARTESIAN_COORD *lpos, MYFLT *input_coords, int32_t n, int32_t ncoords, int32_t coord_kind) {

    for (int i = 0; i < n; i++) {
        int32_t index = i * ncoords;
        if (coord_kind != CARTESIAN_KIND) {
            MYFLT rho, phi, theta;
            if (ncoords == 2) {
                rho = FL(1.0);
                phi = input_coords[index];
                theta = input_coords[index + 1];
            } else {
                rho = input_coords[index];
                phi = input_coords[index + 1];
                theta = input_coords[index + 2];
            }

            POLAR_COORD p = { rho, phi, theta };
            lpos[i] = (CARTESIAN_COORD) pol_to_car(&p, coord_kind);
        } else {
            MYFLT x = input_coords[index];
            MYFLT y = input_coords[index + 1];
            MYFLT z = (ncoords == 3) ? input_coords[index + 2] : FL(0.0);
            lpos[i] = (CARTESIAN_COORD) { x, y, z };
        }
    }
};

void finalize_dbap(DBAP_STATE *dbap, MYFLT *input_coords, int32_t ncoords) {
    CARTESIAN_COORD *lpos = (CARTESIAN_COORD *)dbap->lpos.auxp;
    set_loudspeaker_position(lpos, input_coords, dbap->nchnls, ncoords, dbap->coord_kind);

    MYFLT x = FL(0.0);
    MYFLT y = FL(0.0);
    MYFLT z = FL(0.0);

    for (int i = 0; i < dbap->nchnls; i++) {
        x += lpos[i].x;
        y += lpos[i].y;
        z += lpos[i].z;
    }

    dbap->center.x = x / dbap->nchnls;
    dbap->center.y = y / dbap->nchnls;
    dbap->center.z = z / dbap->nchnls;

    dbap->spatial_blur = get_spatial_blur(dbap);
    dbap->eta = dbap->spatial_blur / dbap->nchnls;
    memset(dbap->distances.auxp, 0, sizeof(MYFLT) * dbap->nchnls);
}

int32_t initialize_dbap(CSOUND *csound, DBAP_STATE *dbap, MYFLT *input_coords, int32_t n, int32_t ncoords, MYFLT rolloff, MYFLT *weights, int32_t nsamples, int32_t coord_kind) {
    int32_t err = init_dbap(csound, dbap, n, ncoords, rolloff, weights, nsamples, coord_kind);
    if (err != OK) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] DBAP init error\n");
    }

    finalize_dbap(dbap, input_coords, ncoords);
    return OK;
}

MYFLT get_p(DBAP_STATE *dbap, CARTESIAN_COORD *source) {
    MYFLT dist_source_center = get_distance(dbap->center, *source, dbap->spatial_blur);
    dist_source_center = dist_source_center > FL(0.0) ? dist_source_center : FL(1.0);
    MYFLT q = ((MYFLT *)dbap->distances.auxp)[2 * dbap->nchnls] / dist_source_center;
    MYFLT p = q < FL(1.0) ? q : FL(1.0);
    return p;
}

void get_b(DBAP_STATE *dbap, MYFLT p) {
    memset(dbap->temp_b.auxp, 0, sizeof(MYFLT) * dbap->nchnls);
    memset(dbap->temp_u.auxp, 0, sizeof(MYFLT) * dbap->nchnls);

    MYFLT *u = (MYFLT *)dbap->temp_u.auxp;
    MYFLT *distances = dbap->distances.auxp;
    MYFLT unorm = FL(0.0);
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        u[i] = distances[i] - distances[2 * dbap->nchnls];
        unorm += u[i] * u[i];
    }

    unorm = unorm > FL(0.0001) ? unorm : FL(1.0);
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        MYFLT v = u[i] / unorm;
        u[i] = (v * v) + dbap->eta;
    }

    memcpy(distances + dbap->nchnls, distances, sizeof(MYFLT) * dbap->nchnls);

    MYFLT *sorted_distances = distances + dbap->nchnls;
    MYFLT upper_mid = quickselect(sorted_distances, dbap->nchnls, dbap->nchnls / 2);

    MYFLT median;
    if (dbap->nchnls % 2 == 1) {
        median = upper_mid;
    } else {
        MYFLT lower_mid = sorted_distances[0];
        for (int32_t i = 0; i < dbap->nchnls / 2; i++) {
            if (sorted_distances[i] > lower_mid) lower_mid = sorted_distances[i];
        }
        median = (lower_mid + upper_mid) * FL(0.5);
    }

    MYFLT *b_values = (MYFLT *)dbap->temp_b.auxp;
    MYFLT fac = (FL(1.0) / p) + FL(1.0);
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        MYFLT b = u[i] / (median > FL(0.0000001) ? median : FL(0.0000001)) * fac;
        b_values[i] = (b * b) + FL(1.0);
    }
}

MYFLT get_k(DBAP_STATE *dbap, MYFLT p) {
    MYFLT pfac = FL(2.0) * dbap->a;
    MYFLT k_num = POWER(p, pfac);
    MYFLT k_den = FL(0.0);
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        MYFLT b = ((MYFLT *)dbap->temp_b.auxp)[i];
        MYFLT w = ((MYFLT *)dbap->weights.auxp)[i];
        MYFLT d = ((MYFLT *)dbap->distances.auxp)[i];
        k_den += (b * b * w * w) / POWER(d, pfac);
    }

    MYFLT k = k_num / SQRT(k_den);
    return k;
}

void solve_dbap_gain_vector(DBAP_STATE *dbap, CARTESIAN_COORD *source, MYFLT *spread) {
    memset(dbap->lgains.auxp, 0, sizeof(MYFLT) * dbap->nchnls);
    MYFLT *gains = (MYFLT *)dbap->lgains.auxp;

    update_loudspeaker_distances(dbap, source, dbap->spatial_blur);
    MYFLT p = get_p(dbap, source);
    get_b(dbap, p);
    MYFLT k = get_k(dbap, p);
    MYFLT mean_distance = ((MYFLT *)dbap->distances.auxp)[2 * dbap->nchnls + 1];
    MYFLT beta = *spread / (mean_distance + FL(0.000001)); // soft-limited decay
    MYFLT sum_sq = FL(0.0);
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        MYFLT b = ((MYFLT *)dbap->temp_b.auxp)[i];
        MYFLT w = ((MYFLT *)dbap->weights.auxp)[i];
        MYFLT d = ((MYFLT *)dbap->distances.auxp)[i];
        MYFLT gain = (k * w * b) / POWER(d, dbap->a);
        gain *= EXP(-beta * d);
        gains[i] = gain;
        sum_sq += gain * gain;
    }

    MYFLT norm = SQRT(sum_sq);
    norm = (norm < FL(0.000001)) ? FL(1.0) : norm;
    for (int32_t i = 0; i < dbap->nchnls; i++) {
        gains[i] /= norm;
    }
}

void gain_vector_interpolation(DBAP_STATE *dbap, MYFLT *input_frame, int32_t nsamples) {
    MYFLT *curr_gains = (MYFLT *)dbap->lgains.auxp;
    MYFLT *prev_gains = (MYFLT *)curr_gains + dbap->nchnls;
    MYFLT *internal_out = (MYFLT *)dbap->internal_out_frame.auxp;

    int32_t nchnls = dbap->nchnls;
    for (int32_t i = 0; i < nchnls; i++) {
        MYFLT g0 = prev_gains[i];
        MYFLT g1 = curr_gains[i];

        int32_t index = i *  nsamples;
        if (g0 == FL(0.0) && g1 == FL(0.0)) {
            memset(internal_out + index, 0, sizeof(MYFLT) * nsamples);
            continue;
        }

        MYFLT gdiff = (g1 - g0) / nsamples;
        MYFLT g = g0;
        for (int32_t j = 0; j < nsamples; j++) {
            internal_out[index + j] = input_frame[j] * g;
            g += gdiff;
        }
    }

    memcpy(prev_gains, curr_gains, sizeof(MYFLT) * dbap->nchnls);
}

// prepare dbap helper function (common)
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
) {
    if (coord_kind != 0 && coord_kind != 1 && coord_kind != 2) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Coordinates kind must be 0 (for cartesian), 1 (for polar degree) or 2 (for polar randians)\n");
    }

    if (nchnls != npos) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Number of out channels must be the equal to number of loudspeakers\n");
    }

    if (ncoords != 2 && ncoords != 3) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Dimension must be exactly 2 (for 2D) or 3 (for 3d)\n");
    }

    return initialize_dbap(
        csound,
        dbap_state,
        input_coords,
        nchnls,
        ncoords,
        rolloff_value,
        weights,
        csound->ksmps,
        coord_kind
    );
};

// dbap perf helper function (common)
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
) {
    int32_t nsamples = csound->ksmps;
    int32_t nchnls = dbap_state->nchnls;

    if (source_size < 2 || source_size > 3) {
        return csound->PerfError(csound, h, "\n[DBAP OPCODE ERROR] Source dimension must be 2D or 3D\n");
    }

    CARTESIAN_COORD source;
    build_source(&source, source_array, source_size, coord_mode);

    solve_dbap_gain_vector(dbap_state, &source, &spread);

    if (input_frame) {
        uint32_t offset = h->insdshead->ksmps_offset;
        uint32_t early = h->insdshead->ksmps_no_end;
        gain_vector_interpolation(dbap_state, input_frame, nsamples);

        MYFLT *internal_out = (MYFLT *)dbap_state->internal_out_frame.auxp;

        for (int32_t i = 0; i < nchnls; i++) {
            int32_t ch_offset_ptr = i * nsamples;
            MYFLT *out_ch = out + ch_offset_ptr;
            if (UNLIKELY(offset)) {
                memset(out_ch, 0, sizeof(MYFLT) * offset);
            }

            int32_t n_active_part = nsamples - early - offset;
            memcpy(out_ch + offset, internal_out + ch_offset_ptr + offset, sizeof(MYFLT) * n_active_part);

            if (UNLIKELY(early)) {
                memset(out_ch + (nsamples - early), 0, sizeof(MYFLT) * early);
            }
        }
    } else {
        MYFLT *lgains = (MYFLT *)dbap_state->lgains.auxp;
        for (int32_t i = 0; i < nchnls; i++) {
            out[i] = lgains[i];
        }
    }

    return OK;
}

int32_t prepare_dbap_with_arr(CSOUND *csound, DBAP_WITH_ARR *dbap) {
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)dbap->loudspeaker_pos->data,
        (MYFLT *)dbap->loudspeakers_weights->data,
        dbap->out->sizes[0],
        dbap->loudspeaker_pos->sizes[0],
        dbap->loudspeaker_pos->sizes[1],
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_gains_with_arr(CSOUND *csound, DBAP_GAINS_WITH_ARR *dbap) {
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)dbap->loudspeaker_pos->data,
        (MYFLT *)dbap->loudspeakers_weights->data,
        dbap->out->sizes[0],
        dbap->loudspeaker_pos->sizes[0],
        dbap->loudspeaker_pos->sizes[1],
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_with_arr_no_weights(CSOUND *csound, DBAP_WITH_ARR *dbap) {
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)dbap->loudspeaker_pos->data,
        NULL,
        dbap->out->sizes[0],
        dbap->loudspeaker_pos->sizes[0],
        dbap->loudspeaker_pos->sizes[1],
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_gains_with_arr_no_weights(CSOUND *csound, DBAP_GAINS_WITH_ARR *dbap) {
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)dbap->loudspeaker_pos->data,
        NULL,
        dbap->out->sizes[0],
        dbap->loudspeaker_pos->sizes[0],
        dbap->loudspeaker_pos->sizes[1],
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
};

int32_t prepare_dbap_with_func(CSOUND *csound, DBAP_WITH_FUNC *dbap) {
    FUNC *pos_table = csound->FTFind(csound, dbap->loudspeaker_pos);
    if (!pos_table) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Invalid loudspeaker positions GEN table\n");
    }
    int32_t ncoords = (int32_t)(*dbap->loudspeaker_dimension);
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)pos_table->ftable,
        (MYFLT *)dbap->loudspeakers_weights->data,
        dbap->out->sizes[0],
        (int32_t)pos_table->flen / ncoords,
        ncoords,
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_gains_with_func(CSOUND *csound, DBAP_GAINS_WITH_FUNC *dbap) {
    FUNC *pos_table = csound->FTFind(csound, dbap->loudspeaker_pos);
    if (!pos_table) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Invalid loudspeaker positions GEN table\n");
    }
    int32_t ncoords = (int32_t)(*dbap->loudspeaker_dimension);
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)pos_table->ftable,
        (MYFLT *)dbap->loudspeakers_weights->data,
        dbap->out->sizes[0],
        (int32_t)pos_table->flen / ncoords,
        ncoords,
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_with_func_no_weights(CSOUND *csound, DBAP_WITH_FUNC *dbap) {
    FUNC *pos_table = csound->FTFind(csound, dbap->loudspeaker_pos);
    if (!pos_table) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Invalid loudspeaker positions GEN table\n");
    }
    int32_t ncoords = (int32_t)(*dbap->loudspeaker_dimension);
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)pos_table->ftable,
        NULL,
        dbap->out->sizes[0],
        (int32_t)pos_table->flen / ncoords,
        ncoords,
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t prepare_dbap_gains_with_func_no_weights(CSOUND *csound, DBAP_GAINS_WITH_FUNC *dbap) {
    FUNC *pos_table = csound->FTFind(csound, dbap->loudspeaker_pos);
    if (!pos_table) {
        return csound->InitError(csound, "\n[DBAP OPCODE ERROR] Invalid loudspeaker positions GEN table\n");
    }
    int32_t ncoords = (int32_t)(*dbap->loudspeaker_dimension);
    return prepare_dbap_helper(
        csound,
        &dbap->dbap_state,
        (MYFLT *)pos_table->ftable,
        NULL,
        dbap->out->sizes[0],
        (int32_t)pos_table->flen / ncoords,
        ncoords,
        (int32_t)(*dbap->coord_mode),
        *dbap->rolloff_value
    );
}

int32_t dbap_with_arr(CSOUND *csound, DBAP_WITH_ARR *sdbap) {
    return dbap_helper(
        csound,
        &(sdbap->h),
        &sdbap->dbap_state,
        (MYFLT)(*sdbap->spread),
        (int32_t)(*sdbap->coord_mode),
        sdbap->source->sizes[0],
        sdbap->source->data,
        sdbap->input_frame,
        sdbap->out->data
    );
}

int32_t dbap_gains_with_arr(CSOUND *csound, DBAP_GAINS_WITH_ARR *sdbap) {
    return dbap_helper(
        csound,
        &(sdbap->h),
        &sdbap->dbap_state,
        (MYFLT)(*sdbap->spread),
        (int32_t)(*sdbap->coord_mode),
        sdbap->source->sizes[0],
        sdbap->source->data,
        NULL,
        sdbap->out->data
    );
}

int32_t dbap_with_func(CSOUND *csound, DBAP_WITH_FUNC *sdbap) {
    return dbap_helper(
        csound,
        &(sdbap->h),
        &sdbap->dbap_state,
        (MYFLT)(*sdbap->spread),
        (int32_t)(*sdbap->coord_mode),
        sdbap->source->sizes[0],
        sdbap->source->data,
        sdbap->input_frame,
        sdbap->out->data
    );
}

int32_t dbap_gains_with_func(CSOUND *csound, DBAP_GAINS_WITH_FUNC *sdbap) {
    return dbap_helper(
        csound,
        &(sdbap->h),
        &sdbap->dbap_state,
        (MYFLT)(*sdbap->spread),
        (int32_t)(*sdbap->coord_mode),
        sdbap->source->sizes[0],
        sdbap->source->data,
        NULL,
        sdbap->out->data
    );
}

static OENTRY localops[] = {
    // dbap on audio frame
    {
        "dbap", sizeof(DBAP_WITH_ARR), 0, "a[]", "aik[]i[][]kii[]", (SUBR)prepare_dbap_with_arr, (SUBR)dbap_with_arr
    },
    {
        "dbap", sizeof(DBAP_WITH_ARR), 0, "a[]", "aik[]i[][]ki", (SUBR)prepare_dbap_with_arr_no_weights, (SUBR)dbap_with_arr
    },
    {
        "dbap", sizeof(DBAP_WITH_FUNC), 0, "a[]", "aik[]iikii[]", (SUBR)prepare_dbap_with_func, (SUBR)dbap_with_func
    },
    {
        "dbap", sizeof(DBAP_WITH_FUNC), 0, "a[]", "aik[]iiki", (SUBR)prepare_dbap_with_func_no_weights, (SUBR)dbap_with_func
    },
    // gain vector generation
    {
        "dbapgains", sizeof(DBAP_GAINS_WITH_ARR), 0, "k[]", "ik[]i[][]kii[]", (SUBR)prepare_dbap_gains_with_arr, (SUBR)dbap_gains_with_arr
    },
    {
        "dbapgains", sizeof(DBAP_GAINS_WITH_ARR), 0, "k[]", "ik[]i[][]ki", (SUBR)prepare_dbap_gains_with_arr_no_weights, (SUBR)dbap_gains_with_arr
    },
    {
        "dbapgains", sizeof(DBAP_GAINS_WITH_FUNC), 0, "k[]", "ik[]iikii[]", (SUBR)prepare_dbap_gains_with_func, (SUBR)dbap_gains_with_func
    },
    {
        "dbapgains", sizeof(DBAP_GAINS_WITH_FUNC), 0, "k[]", "ik[]iiki", (SUBR)prepare_dbap_gains_with_func_no_weights, (SUBR)dbap_gains_with_func
    },
};

int32_t dbap_init_(CSOUND *csound) {
    return csound->AppendOpcodes(csound, &(localops[0]), (int32_t)(sizeof(localops) / sizeof(OENTRY)));
}
