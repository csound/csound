/*
  trigEnvSegs.cpp: trigger versions of linseg and expseg

  Copyright (C) 2021 Rory Walsh
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

#include <plugin.h>
#include <cmath>
#include <vector>

// Both envelope shapes use the same trigger and segment timing.
template <bool Exponential>
struct TrigSegments : csnd::Plugin<1, 64>
{
    int32_t init()
    {
        if (in_count() < 4 || in_count() % 2 != 0)
            return csound->init_error(
                "expected a trigger and value/duration/value pairs");

        values.clear();
        durations.clear();
        increments.clear();
        for (uint32_t i = 1; i < in_count(); i += 2)
        {
            MYFLT value = inargs[i];
            if (Exponential &&
                (value == 0 || (value > 0) != (inargs[1] > 0)))
                return csound->init_error(
                    "exponential values must be nonzero and have the same sign");
            values.push_back(value);
            if (i + 1 < in_count())
            {
                if (inargs[i + 1] < 0)
                    return csound->init_error("segment duration must not be negative");
                durations.push_back(std::ceil(inargs[i + 1] * sr()));
            }
        }
        for (size_t i = 0; i < durations.size(); ++i)
        {
            // A zero-length segment jumps to its endpoint without interpolation.
            MYFLT increment = 0;
            if (durations[i] > 0)
                increment = Exponential
                    ? std::pow(values[i + 1] / values[i], 1.0 / durations[i])
                    : (values[i + 1] - values[i]) / durations[i];
            increments.push_back(increment);
        }
        playing = false;
        segment = 0;
        remaining = 0;
        outValue = values[0];
        outargs[0] = outValue;
        return OK;
    }

    int32_t kperf() { return perform<false>(); }
    int32_t aperf() { return perform<true>(); }

    template <bool Audio> int32_t perform()
    {
        // The trigger is k-rate: restart once for the whole active block.
        if (offset < nsmps && inargs[0] == 1)
        {
            segment = 0;
            remaining = durations[0];
            outValue = values[0];
            playing = true;
        }
        MYFLT *output = outargs(0);
        for (uint32_t i = offset; i < nsmps; ++i)
        {
            if (playing)
            {
                while (remaining <= 0 && segment < durations.size())
                {
                    outValue = values[segment + 1];
                    ++segment;
                    if (segment < durations.size())
                        remaining = durations[segment];
                }
                if (segment < durations.size())
                {
                    if (Exponential)
                        outValue *= increments[segment];
                    else
                        outValue += increments[segment];
                    remaining -= 1;
                    if (remaining <= 0)
                        outValue = values[segment + 1];
                }
                else
                    playing = false;
            }
            if (Audio)
                output[i] = outValue;
        }
        if (!Audio)
            output[0] = outValue;
        return OK;
    }

    bool playing;
    size_t segment;
    double remaining;
    MYFLT outValue;
    std::vector<MYFLT> values, increments;
    std::vector<double> durations;
};

using TrigLinseg = TrigSegments<false>;
using TrigExpseg = TrigSegments<true>;

static void onload (csnd::Csound* csound)
{
    csnd::plugin<TrigExpseg> (csound, "trigExpseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigExpseg> (csound, "trigExpseg.kk", "k", "km", csnd::thread::ik);
    csnd::plugin<TrigLinseg> (csound, "trigLinseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigLinseg> (csound, "trigLinseg.kk", "k", "km", csnd::thread::ik);
    csnd::plugin<TrigExpseg> (csound, "trigexpseg.aa", "a", "km", csnd::thread::ia); /* alias */
    csnd::plugin<TrigExpseg> (csound, "trigexpseg.kk", "k", "km", csnd::thread::ik); /* alias */
    csnd::plugin<TrigLinseg> (csound, "triglinseg.aa", "a", "km", csnd::thread::ia); /* alias */
    csnd::plugin<TrigLinseg> (csound, "triglinseg.kk", "k", "km", csnd::thread::ik); /* alias */
}

#ifdef BUILD_PLUGINS
#include <modload.h>
void csnd::on_load(csnd::Csound *csound) {
    onload(csound);
}
#else
extern "C" int32_t trigEnv_init_modules(CSOUND *csound) {
    onload((csnd::Csound *)csound);
    return OK;
  }
#endif
