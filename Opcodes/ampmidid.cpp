/*
    ampmidid.cpp

    Copyright (C) 2006 Michael Gogins

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
#include <cmath>
#include "OpcodeBase.hpp"
#include "csdl.h"
#include "csound.h"
#include "sysdep.h"

using namespace csound;

/**
 * Musically map MIDI velocity to peak amplitude
 * within a specified dynamic range in decibels:
 * a = (m * v + b) ^ 2
 * where a = amplitude,
 * v = MIDI velocity,
 * r = 10 ^ (R / 20),
 * b = 127 / (126 * sqrt( r )) - 1 / 126,
 * m = (1 - b) / 127,
 * and R = specified dynamic range in decibels.
 * See Roger Dannenberg, "The Interpretation of MIDI Velocity,"
 * in Georg Essl and Ichiro Fujinaga (Eds.), Proceedings of the
 * 2006 International Computer Music Conference,
 * November 6-11, 2006 (San Francisco:
 * The International Computer Music Association), pp. 193-196.
 */
class KAMPMIDID : public OpcodeBase<KAMPMIDID> {
public:
    // Outputs.
    MYFLT *kamplitude;
    // Inputs.
    MYFLT *kvelocity;
    MYFLT *irdb;
    MYFLT *iuse0dbfs;
    // State.
    MYFLT ir;
    MYFLT im;
    MYFLT ib;
    MYFLT onedrms;
    MYFLT dbfs;
    KAMPMIDID()
        : kamplitude(0), kvelocity(0), irdb(0), iuse0dbfs(0), ir(0), im(0), ib(0),
          onedrms(0), dbfs(1) {}
    int init(CSOUND *csound) {
        // Convert RMS power to amplitude (assuming a sinusoidal signal).
        onedrms = MYFLT(1.0) / MYFLT(0.707);
        // Convert dynamic range in decibels to RMS dynamic range.
        ir = std::pow(MYFLT(10.0), *irdb / MYFLT(20.0));
        // Solve for coefficients of the linear conversion function given
        // RMS dynamic range.
        ib = MYFLT(127.0) / (MYFLT(126.0) * std::sqrt(ir)) -
             MYFLT(1.0) / MYFLT(126.0);
        im = (MYFLT(1.0) - ib) / MYFLT(127.0);
        if (*iuse0dbfs == FL(0.0)) {
            dbfs = csound->Get0dBFS(csound);
        } else {
            dbfs = *iuse0dbfs;
        }
        return OK;
    }
    int kontrol(CSOUND *csound) {
        IGN(csound);
        *kamplitude =
            dbfs * std::pow((*kvelocity * im) + ib, MYFLT(2.0)) * onedrms;
        return OK;
    }
};

class IAMPMIDID : public OpcodeBase<IAMPMIDID> {
public:
    // Outputs.
    MYFLT *iamplitude;
    // Inputs.
    MYFLT *ivelocity;
    MYFLT *irdb;
    MYFLT *iuse0dbfs;
    // State.
    MYFLT ir;
    MYFLT im;
    MYFLT ib;
    MYFLT onedrms;
    MYFLT dbfs;
    IAMPMIDID()
        : iamplitude(0), ivelocity(0), irdb(0), iuse0dbfs(0), ir(0), im(0), ib(0),
          onedrms(0), dbfs(1) {}
    int init(CSOUND *csound) {
        // Convert RMS power to amplitude (assuming a sinusoidal signal).
        onedrms = MYFLT(1.0) / MYFLT(0.707);
        // Convert dynamic range in decibels to RMS dynamic range.
        ir = std::pow(MYFLT(10.0), *irdb / MYFLT(20.0));
        // Solve for coefficients of the linear conversion function given
        // RMS dynamic range.
        ib = MYFLT(127.0) / (MYFLT(126.0) * std::sqrt(ir)) -
             MYFLT(1.0) / MYFLT(126.0);
        im = (MYFLT(1.0) - ib) / MYFLT(127.0);
        if (*iuse0dbfs == FL(0.0)) {
            dbfs = csound->Get0dBFS(csound);
        } else {
            dbfs = *iuse0dbfs;
        }
        *iamplitude =
            dbfs * std::pow((*ivelocity * im) + ib, MYFLT(2.0)) * onedrms;
        return OK;
    }
    int noteoff(CSOUND *) {
        return OK;
    }
};

/**
 * Maps an input MIDI velocity number to an output gain factor with a maximum
 * value of 1, modifying the output gain by a dynamic range and a shaping
 * exponent. The minimum output gain is 1 minus the dynamic
 * range. A shaping exponent of 1 is a linear response; increasing the
 * exponent produces an increasingly depressed knee in the gain response
 * curve. This opcode was suggested by Mauro Giubileo, and its behavior
 * can be seen at https://www.desmos.com/calculator/fvxupgp4ef.
 */
class AMPMIDICURVE : public OpcodeBase<AMPMIDICURVE> {
public:
    MYFLT *k_gain;
    MYFLT *k_midi_velocity;
    MYFLT *k_dynamic_range;
    MYFLT *k_exponent;
    int init(CSOUND *csound) {
        *k_gain = *k_dynamic_range * std::pow(*k_midi_velocity / FL(127.), *k_exponent) + FL(1.) - *k_dynamic_range;
        return OK;
    }
    int kontrol(CSOUND *csound) {
        *k_gain = *k_dynamic_range * std::pow(*k_midi_velocity / FL(127.), *k_exponent) + FL(1.) - *k_dynamic_range;
        return OK;
    }
};

extern "C" {
    PUBLIC int csoundModuleInit_ampmidid(CSOUND *csound) {
        int status = csound->AppendOpcode(
                         csound, (char *)"ampmidid.k", sizeof(KAMPMIDID), 0, 3, (char *)"k",
                         (char *)"kio",
                         (int (*)(CSOUND *, void *))KAMPMIDID::init_,
                         (int (*)(CSOUND *, void *))KAMPMIDID::kontrol_,
                         (int (*)(CSOUND *, void *))0);
        status |= csound->AppendOpcode(
                      csound, (char *)"ampmidid.i", sizeof(IAMPMIDID), 0, 1, (char *)"i",
                      (char *)"iio",
                      (int (*)(CSOUND *, void *))IAMPMIDID::init_,
                      (int (*)(CSOUND *, void *))0,
                      (int (*)(CSOUND *, void *))0);
        status |= csound->AppendOpcode(csound, (char *)"ampmidid", 0xffff, 0, 0, 0, 0,
                                       0, 0, 0);
        status = csound->AppendOpcode(
                     csound, (char *)"ampmidicurve.k", sizeof(AMPMIDICURVE), 0, 3, (char *)"k",
                     (char *)"kkk",
                     (int (*)(CSOUND *, void *))AMPMIDICURVE::init_,
                     (int (*)(CSOUND *, void *))AMPMIDICURVE::kontrol_,
                     (int (*)(CSOUND *, void *))0);
        status |= csound->AppendOpcode(
                      csound, (char *)"ampmidicurve.i", sizeof(AMPMIDICURVE), 0, 1, (char *)"i",
                      (char *)"iii",
                      (int (*)(CSOUND *, void *))AMPMIDICURVE::init_,
                      (int (*)(CSOUND *, void *))0,
                      (int (*)(CSOUND *, void *))0);
        status |= csound->AppendOpcode(csound, (char *)"ampmidicurve", 0xffff, 0, 0, 0, 0,
                                       0, 0, 0);
        return status;
    }

#ifndef INIT_STATIC_MODULES
    PUBLIC int csoundModuleCreate(CSOUND *csound) {
        IGN(csound);
        return 0;
    }

    PUBLIC int csoundModuleInit(CSOUND *csound) {
        return csoundModuleInit_ampmidid(csound);
    }

    PUBLIC int csoundModuleDestroy(CSOUND *csound) {
        IGN(csound);
        return 0;
    }
#endif
}
