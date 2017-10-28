/*
    csPerfThread.cpp:

    Copyright (C) 2005 Istvan Varga

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

#include <iostream>
#include <exception>

#include "csound.hpp"
#include "csPerfThread.hpp"
#include <sndfile.h>

// ----------------------------------------------------------------------------

/**
 * Base class for event messages.
 */

class CsoundPerformanceThreadMessage {
 protected:
    CsoundPerformanceThread *pt_;
    void SetPaused(int state)
    {
      pt_->paused = state;
    }
    int GetPaused()
    {
      return pt_->paused;
    }
    recordData_t * getRecordData()
    {
      return &(pt_->recordData);
    }
    void lockRecord()
    {
        csoundLockMutex(pt_->recordLock);
    }
    void unlockRecord()
    {
        csoundUnlockMutex(pt_->recordLock);
    }

    void QueueMessage(CsoundPerformanceThreadMessage *msg)
    {
      pt_->QueueMessage(msg);
    }

 public:
    CsoundPerformanceThreadMessage *nxt;
    virtual int run() = 0;
    CsoundPerformanceThreadMessage(CsoundPerformanceThread *pt)
    {
      pt_ = pt;
      nxt = (CsoundPerformanceThreadMessage*) 0;
    }
    virtual ~CsoundPerformanceThreadMessage() {}
};

/**
 * Unpause performance
 */

class CsPerfThreadMsg_Play : public CsoundPerformanceThreadMessage {
 public:
    CsPerfThreadMsg_Play(CsoundPerformanceThread *pt)
    : CsoundPerformanceThreadMessage(pt) {}
    int run()
    {
      SetPaused(0);
      return 0;
    }
    ~CsPerfThreadMsg_Play() {}
};

/**
 * Pause performance
 */

class CsPerfThreadMsg_Pause : public CsoundPerformanceThreadMessage {
 public:
    CsPerfThreadMsg_Pause(CsoundPerformanceThread *pt)
    : CsoundPerformanceThreadMessage(pt) {}
    int run()
    {
      SetPaused(1);
      return 0;
    }
    ~CsPerfThreadMsg_Pause() {}
};

/**
 * Toggle pause mode
 */

class CsPerfThreadMsg_TogglePause : public CsoundPerformanceThreadMessage {
 public:
    CsPerfThreadMsg_TogglePause(CsoundPerformanceThread *pt)
    : CsoundPerformanceThreadMessage(pt) {}
    int run()
    {
      SetPaused(GetPaused() ? 0 : 1);
      return 0;
    }
    ~CsPerfThreadMsg_TogglePause() {}
};

extern "C" {
  static uintptr_t recordThread_(void *recordData_)
  {
    recordData_t *recordData = (recordData_t *)recordData_;
    int retval = 0;
    const int bufsize = 4096;
    MYFLT buf[bufsize];
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    while (recordData->running) {
                csoundLockMutex (recordData->mutex);
        csoundCondWait(recordData->condvar, recordData->mutex);
        int sampsread;
        do {
            sampsread = csoundReadCircularBuffer(NULL, recordData->cbuf,
                                                 buf, bufsize);
#ifdef USE_DOUBLE
            sf_write_double((SNDFILE *) recordData->sfile,
                            buf, sampsread);
#else
            sf_write_float((SNDFILE *) recordData->sfile,
                           buf, sampsread);
#endif
        } while(sampsread != 0);
                csoundUnlockMutex (recordData->mutex);
    }
    return (uintptr_t) ((unsigned int) retval);
  }
}

class CsPerfThreadMsg_Record: public CsoundPerformanceThreadMessage {
public:
    CsPerfThreadMsg_Record(CsoundPerformanceThread *pt,
                           std::string filename,
                           int samplebits = 16,
                           int numbufs = 4)
    : CsoundPerformanceThreadMessage(pt)
    {
        this->filename = filename;
        CsoundPerformanceThreadMessage::lockRecord();
        recordData_t *recordData = CsoundPerformanceThreadMessage::getRecordData();
        if (recordData->running) {
            CsoundPerformanceThreadMessage::unlockRecord();
            return;
        }
        CSOUND * csound = pt_->GetCsound();
        if (!csound) {
            return;
        }
        int bufsize = csoundGetOutputBufferSize(csound)
                * csoundGetNchnls(csound) * numbufs;
        recordData->cbuf = csoundCreateCircularBuffer(csound,
                                                 bufsize,
                                                 sizeof(MYFLT));

        if (!recordData->cbuf) {
          csoundMessage(csound, "Could create recording buffer.");
          return;
        }

        SF_INFO sf_info;
        sf_info.samplerate = csoundGetSr(csound);
        sf_info.channels = csoundGetNchnls(csound);
        switch (samplebits) {
        case 32:
            sf_info.format = SF_FORMAT_FLOAT;
            break;
        case 24:
            sf_info.format = SF_FORMAT_PCM_24;
            break;
        case 16:
        default:
            sf_info.format = SF_FORMAT_PCM_16;
            break;
        }

        sf_info.format |= SF_FORMAT_WAV;

        recordData->sfile = (void *) sf_open(filename.c_str(),
                                                 SFM_WRITE,
                                                 &sf_info);
        if (!recordData->sfile) {
          csoundMessage(csound, "Could not open file for recording.");
          csoundDestroyCircularBuffer(csound, recordData->cbuf);
          return;
        }
        sf_command((SNDFILE *) recordData->sfile, SFC_SET_CLIPPING,
                   NULL, SF_TRUE);

        recordData->running = true;
        recordData->thread = csoundCreateThread(recordThread_, (void*) recordData);


        CsoundPerformanceThreadMessage::unlockRecord();
    }
    int run()
    {
      return 0;
    }
    ~CsPerfThreadMsg_Record() {
    }
private:
    std::string filename;

};

class CsPerfThreadMsg_StopRecord: public CsoundPerformanceThreadMessage {
public:
    CsPerfThreadMsg_StopRecord(CsoundPerformanceThread *pt)
      : CsoundPerformanceThreadMessage(pt) {}
    int run()
    {

      CsoundPerformanceThreadMessage::lockRecord();
      recordData_t *recordData = CsoundPerformanceThreadMessage::getRecordData();
      if (recordData->running) {
          recordData->running = false;
          csoundJoinThread(recordData->thread);
          sf_close((SNDFILE *) recordData->sfile);
      }

      CsoundPerformanceThreadMessage::unlockRecord();

      return 0;
    }
    ~CsPerfThreadMsg_StopRecord(){}
private:
  //    bool start;
};


/**
 * Stop performance (cannot be continued)
 */

class CsPerfThreadMsg_Stop : public CsoundPerformanceThreadMessage {
 public:
    CsPerfThreadMsg_Stop(CsoundPerformanceThread *pt)
    : CsoundPerformanceThreadMessage(pt)
    {
      CsoundPerformanceThreadMessage::QueueMessage(
                                         new CsPerfThreadMsg_StopRecord(pt));
    }
    int run()
    {
      return 1;
    }
    ~CsPerfThreadMsg_Stop() {}
};

/**
 * Score event message
 *
 * absp2mode: if non-zero, start times are measured from the beginning of
 *            performance, instead of the current time
 * opcod:     score opcode (e.g. 'i' for a note event)
 * pcnt:      number of p-fields
 * *p:        array of p-fields, p[0] is p1
 */

class CsPerfThreadMsg_ScoreEvent : public CsoundPerformanceThreadMessage {
 private:
    char    opcod;
    int     absp2mode;
    int     pcnt;
    MYFLT   *pp;
    MYFLT   p[10];
 public:
    CsPerfThreadMsg_ScoreEvent(CsoundPerformanceThread *pt,
                               int absp2mode, char opcod,
                               int pcnt, const MYFLT *p)
    : CsoundPerformanceThreadMessage(pt)
    {
      this->opcod = opcod;
      this->absp2mode = absp2mode;
      this->pcnt = pcnt;
      if (pcnt <= 10)
        this->pp = &(this->p[0]);
      else
        this->pp = new MYFLT[(unsigned int) pcnt];
      for (int i = 0; i < pcnt; i++)
        this->pp[i] = p[i];
    }
    int run() {
      CSOUND  *csound = pt_->GetCsound();
      if (absp2mode && pcnt > 1) {
        double  p2 = (double) pp[1] - csoundGetScoreTime(csound);
        if (p2 < 0.0) {
          if (pcnt > 2 && pp[2] >= (MYFLT) 0 &&
              (opcod == 'a' || opcod == 'i')) {
            pp[2] = (MYFLT) ((double) pp[2] + p2);
            if (pp[2] <= (MYFLT) 0)
              return 0;
          }
          p2 = 0.0;
        }
        pp[1] = (MYFLT) p2;
      }
      if (csoundScoreEvent(csound, opcod, pp, (long) pcnt) != 0)
        csoundMessageS(csound, CSOUNDMSG_WARNING,
                       "WARNING: could not create score event\n");
      return 0;
    }
    ~CsPerfThreadMsg_ScoreEvent()
    {
      if (pcnt > 10)
        delete[] pp;
    }
};

/**
 * Score event message as a string
 */

class CsPerfThreadMsg_InputMessage : public CsoundPerformanceThreadMessage {
 private:
    int     len;
    char    *sp;
    char    s[128];
 public:
    CsPerfThreadMsg_InputMessage(CsoundPerformanceThread *pt, const char *s)
    : CsoundPerformanceThreadMessage(pt)
    {
      len = (int) strlen(s);
      if (len < 128)
        this->sp = &(this->s[0]);
      else
        this->sp = new char[(unsigned int) (len + 1)];
      strcpy(this->sp, s);
    }
    int run()
    {
      csoundInputMessage(pt_->GetCsound(), sp);
      return 0;
    }
    ~CsPerfThreadMsg_InputMessage()
    {
      if (len >= 128)
        delete[] sp;
    }
};

/**
 * Seek to the specified score time
 */

class CsPerfThreadMsg_SetScoreOffsetSeconds
      : public CsoundPerformanceThreadMessage {
private:
    double  timeVal;
public:
    CsPerfThreadMsg_SetScoreOffsetSeconds(CsoundPerformanceThread *pt,
                                          double timeVal)
    : CsoundPerformanceThreadMessage(pt)
    {
      this->timeVal = timeVal;
    }
    int run()
    {
      csoundSetScoreOffsetSeconds(pt_->GetCsound(), (MYFLT) timeVal);
      return 0;
    }
    ~CsPerfThreadMsg_SetScoreOffsetSeconds() {}
};

// ----------------------------------------------------------------------------

/**
 * Performs the score until end of score, error, or receiving a stop event.
 * Returns a negative value on error.
 */

int CsoundPerformanceThread::Perform()
{
    int retval = 0;
    do {
      while (firstMessage) {
        csoundLockMutex(queueLock);
        do {
          CsoundPerformanceThreadMessage *msg;
          // get oldest message
          msg = (CsoundPerformanceThreadMessage*) firstMessage;
          if (!msg)
            break;
          // unlink from FIFO
          firstMessage = msg->nxt;
          if (!msg->nxt)
            lastMessage = (CsoundPerformanceThreadMessage*) 0;
          // process and destroy message
          retval = msg->run();
          delete msg; // TODO: This should be moved out of the Perform function
        } while (!retval);
        if (paused)
          csoundWaitThreadLock(pauseLock, (size_t) 0);
        // mark queue as empty
        csoundNotifyThreadLock(flushLock);
        csoundUnlockMutex(queueLock);
        // if error or end of score, return now
        if (retval)
          goto endOfPerf;
        // fprintf(stderr, "Error or end of score, returning now.");
        // if paused, wait until a new message is received, then loop back
        if (!paused)
          break;
        // VL: if this is paused, then it will double lock.
        csoundWaitThreadLockNoTimeout(pauseLock);
        csoundNotifyThreadLock(pauseLock);
      }
      if(processcallback != NULL)
           processcallback(cdata);
      retval = csoundPerformKsmps(csound);
      if (recordData.running) {
          MYFLT *spout = csoundGetSpout(csound);
          int len = csoundGetKsmps(csound) * csoundGetNchnls(csound);
          if (csoundGet0dBFS(csound) != 1.0) {
              MYFLT zdbfs = csoundGet0dBFS(csound);
              MYFLT *modspout = spout;
              for (int i = 0; i < len; i++) {
                  *modspout /= zdbfs;
                  modspout++;
              }
          }
          int written = csoundWriteCircularBuffer(NULL, recordData.cbuf,
                                                  spout, len);
          if (written != len) {
              csoundMessage(csound, "perfThread record buffer overrun.\n");
          }
      }
      csoundCondSignal(recordData.condvar); // Needs to be outside the if
                              // for the case where stop record was requested
    } while (!retval);
 endOfPerf:
    status = retval;
    csoundCleanup(csound);
    // delete any pending messages
    csoundLockMutex(queueLock);
    {
      CsoundPerformanceThreadMessage *msg;
      msg = (CsoundPerformanceThreadMessage*) firstMessage;
      firstMessage = (CsoundPerformanceThreadMessage*) 0;
      lastMessage = (CsoundPerformanceThreadMessage*) 0;
      while (msg) {
        CsoundPerformanceThreadMessage *nxt = msg->nxt;
        delete msg;
        msg = nxt;
      }
    }
    csoundNotifyThreadLock(flushLock);
    csoundUnlockMutex(queueLock);
    //running = 0;
    return retval;
}

class CsPerfThread_PerformScore {
 private:
    CsoundPerformanceThread *pt;
 public:
    int Perform()
    {
      return pt->Perform();
    }
    CsPerfThread_PerformScore(void *p)
    {
      pt = (CsoundPerformanceThread*) p;
    }
    ~CsPerfThread_PerformScore()
    {
    }
};

extern "C" {
  static uintptr_t csoundPerformanceThread_(void *userData)
  {
    CsPerfThread_PerformScore p(userData);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
    // perform the score
    int retval = p.Perform();
    // return positive value if stopped or end of score, and negative on error
    return (uintptr_t) ((unsigned int) retval);
  }
}

void CsoundPerformanceThread::csPerfThread_constructor(CSOUND *csound_)
{
    csound = csound_;
    firstMessage = (CsoundPerformanceThreadMessage*) 0;
    lastMessage = (CsoundPerformanceThreadMessage*) 0;
    queueLock = (void*) 0;
    pauseLock = (void*) 0;
    flushLock = (void*) 0;
    recordLock = (void *) 0;
    perfThread = (void*) 0;
    paused = 1;
    status = CSOUND_MEMORY;
    cdata = 0;
    processcallback = 0;
    running = 0;
    queueLock = csoundCreateMutex(0);
    if (!queueLock)
      return;
    pauseLock = csoundCreateThreadLock();
    if (!pauseLock)
      return;
    flushLock = csoundCreateThreadLock();
    if (!flushLock)
      return;
    recordLock = csoundCreateMutex(0);
    if (!recordLock)
      return;
    try {
      lastMessage = new CsPerfThreadMsg_Pause(this);
    }
    catch (std::bad_alloc&) {
      return;
    }
    firstMessage = lastMessage;
    recordData.cbuf = NULL;
    recordData.sfile = NULL;
    recordData.thread = NULL;
    recordData.running = false;

        recordData.mutex = csoundCreateMutex (0);
        recordData.condvar = csoundCreateCondVar();

    perfThread = csoundCreateThread(csoundPerformanceThread_, (void*) this);
    if (perfThread) {
      status = 0;
      running = 1;
    }
}

// ----------------------------------------------------------------------------

/**
 * CsoundPerformanceThread(Csound *)
 * CsoundPerformanceThread(CSOUND *)
 *
 * Performs a score in a separate thread until the end of score is reached,
 * the playback (which is paused by default) is stopped by calling
 * CsoundPerformanceThread::Stop(), or an error occurs.
 * The constructor takes a Csound instance pointer as argument; it assumes
 * that csoundCompile() was called successfully before creating the
 * performance thread. Once the playback is stopped for one of the above
 * mentioned reasons, the performance thread calls csoundCleanup() and
 * returns.
 */

CsoundPerformanceThread::CsoundPerformanceThread(Csound *csound)
{
    csPerfThread_constructor(csound->GetCsound());
}

CsoundPerformanceThread::CsoundPerformanceThread(CSOUND *csound)
{
    csPerfThread_constructor(csound);
}

CsoundPerformanceThread::~CsoundPerformanceThread()
{
    // stop performance if it is still running
    if (!status)
      this->Stop();     // FIXME: should handle memory errors here
    this->Join();
    if (queueLock) {
        csoundDestroyMutex(queueLock);
    }
    if (pauseLock) {
        csoundDestroyMutex(pauseLock);
    }
    if (flushLock) {
        csoundDestroyMutex(flushLock);
    }
    if (recordLock) {
        csoundDestroyMutex(recordLock);

    }
}

// ----------------------------------------------------------------------------

void CsoundPerformanceThread::QueueMessage(CsoundPerformanceThreadMessage *msg)
{
    if (status) {
      delete msg;
      return;
    }
    csoundLockMutex(queueLock);
    // link message into FIFO
    if (!lastMessage)
      firstMessage = msg;
    else
      lastMessage->nxt = msg;
    lastMessage = msg;
    // mark queue as non-empty
    csoundWaitThreadLock(flushLock, (size_t) 0);
    // wake up from pause
    csoundNotifyThreadLock(pauseLock);
    csoundUnlockMutex(queueLock);
}

void CsoundPerformanceThread::Play()
{
    QueueMessage(new CsPerfThreadMsg_Play(this));
}

void CsoundPerformanceThread::Pause()
{
    QueueMessage(new CsPerfThreadMsg_Pause(this));
}

void CsoundPerformanceThread::TogglePause()
{
    QueueMessage(new CsPerfThreadMsg_TogglePause(this));
}

void CsoundPerformanceThread::Stop()
{
    QueueMessage(new CsPerfThreadMsg_Stop(this));
}

void CsoundPerformanceThread::Record(std::string filename,
                                     int samplebits,
                                     int numbufs)
{
    QueueMessage(new CsPerfThreadMsg_Record(this, filename,
                                            samplebits, numbufs));
}

void CsoundPerformanceThread::StopRecord()
{
    QueueMessage(new CsPerfThreadMsg_StopRecord(this));
}

void CsoundPerformanceThread::ScoreEvent(int absp2mode, char opcod,
                                         int pcnt, const MYFLT *p)
{
    QueueMessage(new CsPerfThreadMsg_ScoreEvent(this,
                                                absp2mode, opcod, pcnt, p));
}

void CsoundPerformanceThread::InputMessage(const char *s)
{
    QueueMessage(new CsPerfThreadMsg_InputMessage(this, s));
}

void CsoundPerformanceThread::SetScoreOffsetSeconds(double timeVal)
{
    QueueMessage(new CsPerfThreadMsg_SetScoreOffsetSeconds(this, timeVal));
}

int CsoundPerformanceThread::Join()
{
    int retval;
    retval = status;

    if (perfThread) {
      retval = csoundJoinThread(perfThread);
      perfThread = (void*) 0;
    }
    if (recordData.running) {
        recordData.running = false;
        csoundCondSignal(recordData.condvar);
        csoundJoinThread(recordData.thread);
    }

    // delete any pending messages
    {
      CsoundPerformanceThreadMessage *msg;
      msg = (CsoundPerformanceThreadMessage*) firstMessage;
      firstMessage = (CsoundPerformanceThreadMessage*) 0;
      lastMessage = (CsoundPerformanceThreadMessage*) 0;
      while (msg) {
        CsoundPerformanceThreadMessage *nxt = msg->nxt;
        delete msg;
        msg = nxt;
      }
    }
    // delete all thread locks
    if (queueLock) {
      csoundDestroyMutex(queueLock);
      queueLock = (void*) 0;
    }
    if (pauseLock) {
      csoundNotifyThreadLock(pauseLock);
      csoundDestroyThreadLock(pauseLock);
      pauseLock = (void*) 0;
    }
    if (flushLock) {
      csoundNotifyThreadLock(flushLock);
      csoundDestroyThreadLock(flushLock);
      flushLock = (void*) 0;
    }

    running = 0;
    return retval;
}

/**
 * Waits until all pending messages (pause, send score event, etc.)
 * are actually received by the performance thread.
 */

void CsoundPerformanceThread::FlushMessageQueue()
{
    if (firstMessage) {
      csoundWaitThreadLockNoTimeout(flushLock);
      csoundNotifyThreadLock(flushLock);
    }
}


// This section has been added to have a C layer exposing
// CsoundPerformanceThread, to facilitate the wrapping of
// CsoundPerformanceThread through FFI libraries.
extern "C" {

#ifndef PUBLIK
#if (defined(WIN32) || defined(_WIN32))
#  define PUBLIK        __declspec(dllexport)
#elif defined(__GNUC__) //&& !defined(__MACH__)
#  define PUBLIK        __attribute__ ( (visibility("default")) )
#else
#  define PUBLIK
#endif
#endif

typedef void* Cpt;

PUBLIK Cpt NewCsoundPT(CSOUND *csound)
{
  CsoundPerformanceThread *pt = new CsoundPerformanceThread(csound);
  return (void *)pt;
}

PUBLIK void DeleteCsoundPT(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  delete cpt;
}

PUBLIK int CsoundPTisRunning(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  return cpt->isRunning();
}

PUBLIK void *CsoundPTgetProcessCB(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  return cpt->GetProcessCallback();
}

PUBLIK void CsoundPTsetProcessCB(Cpt pt, void (*callback)(void *), void *cbData)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->SetProcessCallback(callback, cbData);
}

PUBLIK CSOUND *CsoundPTgetCsound(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  return cpt->GetCsound();
}

PUBLIK int CsoundPTgetStatus(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  return cpt->GetStatus();
}

PUBLIK void CsoundPTplay(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->Play();
}

PUBLIK void CsoundPTpause(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->Pause();
}

PUBLIK void CsoundPTtogglePause(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->TogglePause();
}

PUBLIK void CsoundPTstop(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->Stop();
}

PUBLIK void CsoundPTrecord(Cpt pt, const char *filename, int samplebits, int numbufs)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  std::string fname(filename);
  cpt->Record(fname, samplebits, numbufs);
}

PUBLIK void CsoundPTstopRecord(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->StopRecord();
}

PUBLIK void CsoundPTscoreEvent(Cpt pt, int absp2mode, char opcod, int pcnt, MYFLT *p)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->ScoreEvent(absp2mode, opcod, pcnt, p);
}

PUBLIK void CsoundPTinputMessage(Cpt pt, const char *s)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->InputMessage(s);
}

PUBLIK void CsoundPTsetScoreOffsetSeconds(Cpt pt, double timeVal)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->SetScoreOffsetSeconds(timeVal);
}

PUBLIK int CsoundPTjoin(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  return cpt->Join();
}

PUBLIK void CsoundPTflushMessageQueue(Cpt pt)
{
  CsoundPerformanceThread *cpt = (CsoundPerformanceThread *)pt;
  cpt->FlushMessageQueue();
}

} // extern "C"

