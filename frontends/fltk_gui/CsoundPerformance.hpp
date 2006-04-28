/*
    CsoundPerformance.hpp:
    Copyright (C) 2006 Istvan Varga

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef CSOUNDPERFORMANCE_HPP
#define CSOUNDPERFORMANCE_HPP

#include "csound.hpp"

#include <iostream>
#include <vector>

class CsoundPerformance {
 protected:
    CSOUND  *csound;
    int     status;
    bool    paused;
    bool    usingThreads_;
    double  scoreOffset;
    double  scoreTime;
 public:
    CsoundPerformance(CSOUND *);
    CsoundPerformance(Csound *);
    virtual ~CsoundPerformance();
    virtual int Compile(std::vector<std::string>&);
    virtual int Perform() = 0;
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual void Rewind() = 0;
    virtual void SetScoreOffsetSeconds(double, bool) = 0;
    virtual void AdvanceScoreTime(double) = 0;
    virtual double GetScoreTime() = 0;
    bool UsingThreads()
    {
      return usingThreads_;
    }
};

CsoundPerformance *CreateCsoundPerformance(CSOUND *csound, bool enableThreads);
CsoundPerformance *CreateCsoundPerformance(Csound *csound, bool enableThreads);

#endif  // CSOUNDPERFORMANCE_HPP

