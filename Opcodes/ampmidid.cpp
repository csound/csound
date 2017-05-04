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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 */
#include "OpcodeBase.hpp"

using namespace csound;

#include <cmath>

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
class KAMPMIDID : public OpcodeBase<KAMPMIDID>
{
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
  KAMPMIDID() :
    kamplitude(0),
    kvelocity(0),
    irdb(0),
    iuse0dbfs(0),
    ir(0),
    im(0),
    ib(0),
    onedrms(0),
    dbfs(1)
  {}
  int init(CSOUND *csound)
  {
      // Convert RMS power to amplitude (assuming a sinusoidal signal).
      onedrms = MYFLT(1.0) / MYFLT(0.707);
      // Convert dynamic range in decibels to RMS dynamic range.
      ir = std::pow( MYFLT(10.0), *irdb / MYFLT(20.0) );
      // Solve for coefficients of the linear conversion function given
      // RMS dynamic range.
      ib = MYFLT(127.0) / ( MYFLT(126.0) * std::sqrt(ir) ) -
        MYFLT(1.0) / MYFLT(126.0);
      im = ( MYFLT(1.0) - ib ) / MYFLT(127.0);
      if (*iuse0dbfs != FL(0.0)) {
          dbfs = csound->Get0dBFS(csound);
      }
      return OK;
  }
  int kontrol(CSOUND *csound)
  {
      *kamplitude = dbfs * std::pow( (im * (*kvelocity + ib) ), MYFLT(2.0) ) * onedrms;
      return OK;
  }
};

class IAMPMIDID : public OpcodeBase<IAMPMIDID>
{
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
  IAMPMIDID() :
    iamplitude(0),
    ivelocity(0),
    irdb(0),
    iuse0dbfs(0),
    ir(0),
    im(0),
    ib(0),
    onedrms(0),
    dbfs(1)
  {}
  int init(CSOUND *csound)
  {
      // Convert RMS power to amplitude (assuming a sinusoidal signal).
      onedrms = MYFLT(1.0) / MYFLT(0.707);
      // Convert dynamic range in decibels to RMS dynamic range.
      ir = std::pow( MYFLT(10.0), *irdb / MYFLT(20.0) );
      // Solve for coefficients of the linear conversion function given
      // RMS dynamic range.
      ib = MYFLT(127.0) / ( MYFLT(126.0) * std::sqrt(ir) ) -
        MYFLT(1.0) / MYFLT(126.0);
      im = ( MYFLT(1.0) - ib ) / MYFLT(127.0);
      if (*iuse0dbfs != FL(0.0)) {
          dbfs = csound->Get0dBFS(csound);
      }
      *iamplitude = dbfs * std::pow( (im * (*ivelocity + ib) ), MYFLT(2.0) ) * onedrms;
      return OK;
  }
  int noteoff(CSOUND *)
  {
    return OK;
  }
};

extern "C" {

#ifndef PNACL
  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
      return 0;
  }
#endif

  PUBLIC int csoundModuleInit_ampmidid(CSOUND *csound)
  {
      int status = csound->AppendOpcode(csound,
                                        (char*)"ampmidid.k",
                                        sizeof(KAMPMIDID),
                                        0,
                                        3,
                                        (char*)"k",
                                        (char*)"kio",
                                        (int(*)(CSOUND*,void*)) KAMPMIDID::init_,
                                        (int(*)(CSOUND*,void*)) KAMPMIDID::kontrol_,
                                        (int (*)(CSOUND*,void*)) 0);
      status |= csound->AppendOpcode(csound,
                                     (char*)"ampmidid.i",
                                     sizeof(IAMPMIDID),
                                        0,
                                     1,
                                     (char*)"i",
                                     (char*)"iio",
                                     (int (*)(CSOUND*,void*)) IAMPMIDID::init_,
                                     (int (*)(CSOUND*,void*)) 0,
                                     (int (*)(CSOUND*,void*)) 0);
      status |= csound->AppendOpcode(csound,
                                        (char*)"ampmidid",
                                        0xffff,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0);
      return status;
  }

#ifndef PNACL
  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
      return csoundModuleInit_ampmidid(csound);
  }


  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
      return 0;
  }
#endif
}
