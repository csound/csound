/*
    CsoundPerformance.cpp:
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

#include "CsoundGUI.hpp"

#include "csdl.h"
#include "csPerfThread.hpp"

class CsoundPerformance_NoThread : public CsoundPerformance {
 private:
    int     kcnt;
 public:
    CsoundPerformance_NoThread(CSOUND *);
    CsoundPerformance_NoThread(Csound *);
    ~CsoundPerformance_NoThread();
    int Compile(std::vector<std::string>&);
    int Perform();
    void Play();
    void Pause();
    void Stop();
    void Rewind();
    void SetScoreOffsetSeconds(double, bool);
    void AdvanceScoreTime(double);
    double GetScoreTime();
};

class CsoundPerformance_Thread : public CsoundPerformance {
 private:
    CsoundPerformanceThread *pt;
 public:
    CsoundPerformance_Thread(CSOUND *);
    CsoundPerformance_Thread(Csound *);
    ~CsoundPerformance_Thread();
    int Compile(std::vector<std::string>&);
    int Perform();
    void Play();
    void Pause();
    void Stop();
    void Rewind();
    void SetScoreOffsetSeconds(double, bool);
    void AdvanceScoreTime(double);
    double GetScoreTime();
};

CsoundPerformance *CreateCsoundPerformance(CSOUND *csound, bool enableThreads)
{
    if (enableThreads)
      return (CsoundPerformance*) (new CsoundPerformance_Thread(csound));
    return (CsoundPerformance*) (new CsoundPerformance_NoThread(csound));
}

CsoundPerformance *CreateCsoundPerformance(Csound *csound, bool enableThreads)
{
    if (enableThreads)
      return (CsoundPerformance*) (new CsoundPerformance_Thread(csound));
    return (CsoundPerformance*) (new CsoundPerformance_NoThread(csound));
}

// ----------------------------------------------------------------------------

CsoundPerformance::CsoundPerformance(CSOUND *csound)
{
    this->csound = csound;
    status = -1;
    paused = false;
    usingThreads_ = true;
    scoreOffset = 0.0;
    scoreTime = 0.0;
}

CsoundPerformance::CsoundPerformance(Csound *csound)
{
    this->csound = csound->GetCsound();
    status = -1;
    paused = false;
    usingThreads_ = true;
    scoreOffset = 0.0;
    scoreTime = 0.0;
}

CsoundPerformance::~CsoundPerformance()
{
    csoundReset(csound);
}

int CsoundPerformance::Compile(std::vector<std::string>& argList)
{
    std::vector<char *> argv;
    int                 retval;

    for (int i = 0; i < (int) argList.size(); i++)
      argv.push_back(const_cast<char *>(argList[i].c_str()));
    argv.push_back((char*) 0);
    retval = csoundCompile(csound, (int) argList.size(), &(argv.front()));
    status = retval;
    argv.clear();
    if (retval == 0) {
      scoreOffset = (double) csoundGetScoreOffsetSeconds(csound);
      scoreTime = scoreOffset;
    }

    return retval;
}

// ----------------------------------------------------------------------------

CsoundPerformance_NoThread::CsoundPerformance_NoThread(CSOUND *csound)
    : CsoundPerformance(csound)
{
    usingThreads_ = false;
    kcnt = 0;
}

CsoundPerformance_NoThread::CsoundPerformance_NoThread(Csound *csound)
    : CsoundPerformance(csound)
{
    usingThreads_ = false;
    kcnt = 0;
}

CsoundPerformance_NoThread::~CsoundPerformance_NoThread()
{
}

int CsoundPerformance_NoThread::Compile(std::vector<std::string>& argList)
{
    int     retval;

    csoundCreateGlobalVariable(csound, "FLTK_Flags", sizeof(int));
    *((int*) csoundQueryGlobalVariable(csound, "FLTK_Flags")) = 28;
    retval = CsoundPerformance::Compile(argList);
    if (retval == 0) {
      kcnt = (int) ((double) csoundGetKr(csound) / 50.0 + 0.5);
    }

    return retval;
}

int CsoundPerformance_NoThread::Perform()
{
    if (status == 0) {
      if (paused) {
        csoundSleep(20);
      }
      else {
        int i = 0;
        do {
          status = csoundPerformKsmps(csound);
        } while (++i < kcnt && status == 0);
      }
    }

    return status;
}

void CsoundPerformance_NoThread::Play()
{
    paused = false;
}

void CsoundPerformance_NoThread::Pause()
{
    scoreTime = GetScoreTime();
    paused = true;
}

void CsoundPerformance_NoThread::Stop()
{
    if (status == 0) {
      status = 1;
      csoundCleanup(csound);
    }
}

void CsoundPerformance_NoThread::Rewind()
{
#if 0
    if (GetScoreTime() > scoreOffset) {
      scoreTime = scoreOffset;
      csoundSetScoreOffsetSeconds(csound, (MYFLT) scoreOffset);
    }
#else
    scoreTime = (MYFLT) 0;
    csoundSetScoreOffsetSeconds(csound, (MYFLT) 0);
#endif
}

void CsoundPerformance_NoThread::SetScoreOffsetSeconds(double scoreOffset,
                                                       bool applyNow)
{
    this->scoreOffset = scoreOffset;
    if (applyNow) {
      scoreTime = scoreOffset;
      csoundSetScoreOffsetSeconds(csound, (MYFLT) scoreOffset);
    }
}

void CsoundPerformance_NoThread::AdvanceScoreTime(double timeVal)
{
    scoreTime = GetScoreTime() + timeVal;
    csoundSetScoreOffsetSeconds(csound, (MYFLT) scoreTime);
}

double CsoundPerformance_NoThread::GetScoreTime()
{
    if (status != 0)
      return -1.0;
    if (paused)
      return scoreTime;
    return csoundGetScoreTime(csound);
}

// ----------------------------------------------------------------------------

CsoundPerformance_Thread::CsoundPerformance_Thread(CSOUND *csound)
    : CsoundPerformance(csound)
{
    pt = (CsoundPerformanceThread*) 0;
}

CsoundPerformance_Thread::CsoundPerformance_Thread(Csound *csound)
    : CsoundPerformance(csound)
{
    pt = (CsoundPerformanceThread*) 0;
}

CsoundPerformance_Thread::~CsoundPerformance_Thread()
{
    if (pt)
      delete pt;
}

int CsoundPerformance_Thread::Compile(std::vector<std::string>& argList)
{
    int     retval;

    // disable FLTK graphs, but allow for (experimental) use of widget opcodes
    csoundCreateGlobalVariable(csound, "FLTK_Flags", sizeof(int));
    *((int*) csoundQueryGlobalVariable(csound, "FLTK_Flags")) = 274;
    retval = CsoundPerformance::Compile(argList);
    if (retval == 0) {
      pt = new CsoundPerformanceThread(csound);
//       pt->SetRTPriority(60);
      pt->Play();
    }

    return retval;
}

int CsoundPerformance_Thread::Perform()
{
    if (status == 0) {
      status = pt->GetStatus();
      if (status != 0) {
        status = pt->Join();
      }
    }
    return status;
}

void CsoundPerformance_Thread::Play()
{
    if (paused) {
      paused = false;
      pt->Play();
    }
}

void CsoundPerformance_Thread::Pause()
{
    if (!paused) {
      pt->Pause();
      pt->FlushMessageQueue();
      scoreTime = GetScoreTime();
      paused = true;
    }
}

void CsoundPerformance_Thread::Stop()
{
    pt->Stop();
    status = pt->Join();
}

void CsoundPerformance_Thread::Rewind()
{
#if 0
    if (GetScoreTime() > scoreOffset) {
      scoreTime = scoreOffset;
      pt->SetScoreOffsetSeconds(scoreOffset);
    }
#else
    scoreTime = (MYFLT) 0;
    pt->SetScoreOffsetSeconds(0.0);
#endif
}

void CsoundPerformance_Thread::SetScoreOffsetSeconds(double scoreOffset,
                                                     bool applyNow)
{
    this->scoreOffset = scoreOffset;
    if (applyNow) {
      scoreTime = scoreOffset;
      pt->SetScoreOffsetSeconds(scoreOffset);
    }
}

void CsoundPerformance_Thread::AdvanceScoreTime(double timeVal)
{
    bool    saved_pauseState;

    saved_pauseState = paused;
    Pause();
    scoreTime += timeVal;
    pt->SetScoreOffsetSeconds(scoreTime);
    if (!saved_pauseState)
      Play();
}

double CsoundPerformance_Thread::GetScoreTime()
{
    if (status != 0 || pt->GetStatus() != 0)
      return -1.0;
    if (paused)
      return scoreTime;
    // FIXME: this may be unsafe
    return csoundGetScoreTime(csound);
}

