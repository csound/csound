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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include <plugin.h>
#include <vector>
#include <numeric>

// linseg type opcode with trigger mechanism
struct TrigLinseg : csnd::Plugin<1, 64>
{
    int32_t init()
    {
        uint32_t argCnt = 1;
        totalLength = 0;
        samplingRate = this->sr();
        playEnv = 0;
        counter = 0;
        outargs[0] = inargs[1];
        segment = 0;
        outValue = 0;
        values.clear();
        durations.clear();

        while (argCnt < in_count())
        {
            if (argCnt % 2 == 0)
                durations.push_back (inargs[argCnt]*samplingRate);
            else
                values.push_back (inargs[argCnt]);

            argCnt++;
        }

        //values.push_back(inargs[argCnt - 1]);

        incr = (values[1] - values[0]) / durations[0];
        totalLength = std::accumulate (durations.begin(), durations.end(), 0);
        return OK;
    }

    int32_t kperf()
    {
       for (uint32_t i = offset; i < nsmps; i++)
            outargs[0] = envGenerator (1);
        return OK;
    }


    int32_t aperf()
    {
        for (uint32_t i = offset; i < nsmps; i++)
            outargs (0)[i] = envGenerator (1);

        return OK;
    }

    MYFLT envGenerator (int32_t sampIncr)
    {
        // trigger envelope
        if (inargs[0] == 1)
          {
            incr = (values[1] - values[0]) / durations[0];
            outValue = inargs[1];
            playEnv = 1;
          }


        if (playEnv == 1 && segment < durations.size())
        {
            if (counter < durations[segment])
            {
                outValue += incr;
                counter += sampIncr;
                        }
            else
            {
                segment++;
                counter = 0;
                if (segment < durations.size())
                  incr = (values[segment + 1] - values[segment]) / durations[segment];
            }
        }
        else
        {
            playEnv = 0;
            counter = 0;
            segment = 0;
            outValue = values[values.size() - 1];
        }

        return outValue;
    }

    uint32_t samplingRate, playEnv, counter, totalLength, segment;
    MYFLT outValue, incr;
    std::vector<MYFLT> values;
    std::vector<MYFLT> durations;
};

// expseg type opcode with trigger mechanism
struct TrigExpseg : csnd::Plugin<1, 64>
{
    int32_t init()
    {
        uint32_t argCnt = 1;
        samplingRate = this->sr();
        playEnv = 0;
        counter = 0;
        outargs[0] = inargs[1];
        segment = 0;
        outValue = inargs[1];
        values.clear();
        durations.clear();

        while (argCnt < in_count())
        {
            if (argCnt % 2 == 0)
                durations.push_back (inargs[argCnt]*samplingRate);
            else
            {
                if (inargs[argCnt] <= 0.0)
                {
                    csound->message ("iVal is 0");
                    return NOTOK;
                }

                values.push_back (inargs[argCnt]);
            }

            argCnt++;
        }

        incr = pow (values[1] / values[0], 1 / (durations[0]));

        return OK;
    }

    int32_t kperf()
    {
        for (uint32_t i = offset; i < nsmps; i++)
            outargs[0] = envGenerator (1);
        return OK;
    }


    int32_t aperf()
    {
        for (uint32_t i = offset; i < nsmps; i++)
            outargs (0)[i] = envGenerator (1);

        return OK;
    }

    MYFLT envGenerator (int32_t sampIncr)
    {
        // trigger envelope
        if (inargs[0] == 1)
          {
            incr = pow(values[1] / values[0], 1 / (durations[0]));
            outValue = inargs[1];
            playEnv = 1;
          }


        if (playEnv == 1 && segment < durations.size())
        {
            if (counter < durations[segment])
            {
                outValue *= incr;
                counter += sampIncr;
            }
            else
            {
                segment++;
                counter = 0;
                if(segment < durations.size())
                  incr = pow (values[segment + 1] / values[segment], 1 / (durations[segment]));
            }
        }
        else
        {
            playEnv = 0;
            counter = 0;
            segment = 0;
            outValue = values[values.size() - 1];
        }

        return outValue;
    }

    uint32_t samplingRate, playEnv, counter, segment;
    MYFLT outValue, incr;
    std::vector<MYFLT> values;
    std::vector<MYFLT> durations;
};



static void onload (csnd::Csound* csound)
{
    csnd::plugin<TrigExpseg> (csound, "trigExpseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigExpseg> (csound, "trigExpseg.kk", "k", "km", csnd::thread::ik);
    csnd::plugin<TrigLinseg> (csound, "trigLinseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigLinseg> (csound, "trigLinseg.kk", "k", "km", csnd::thread::ik);
    csnd::plugin<TrigExpseg> (csound, "trigexpseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigExpseg> (csound, "trigexpseg.kk", "k", "km", csnd::thread::ik);
    csnd::plugin<TrigLinseg> (csound, "triglinseg.aa", "a", "km", csnd::thread::ia);
    csnd::plugin<TrigLinseg> (csound, "triglinseg.kk", "k", "km", csnd::thread::ik);
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
