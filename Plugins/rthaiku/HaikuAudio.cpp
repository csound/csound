/*
  HaikuAudio.cpp:

  Haiku real-time audio handling for Csound5

  Copyright (C) 2012-2019 Peter J. Goodeve

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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <media/BufferGroup.h>
#include <media/Buffer.h>
#include <media/TimeSource.h>
#include <media/MediaDefs.h>
#include <media/MediaRoster.h>
#include <media/BufferProducer.h>
#include <media/MediaEventLooper.h>

#include "haiku_audio.h"

#define XDPRINTF(x)
#define DPRINTF(x)
//#define DPRINTF(x) printf x


// ----------------

// A few global settings for convenience:

// change this to alter default
static BMediaNode::run_mode gRunMode = BMediaNode::B_RECORDING;
/// These have been kept for now:
static size_t gBufferSize = 1024;       // specified by default
static bigtime_t gAddLatency = 0;       // Added to computed latency
static int gXtraBuffers = 0;

///////////////////////////////////////////////////////////////////


class AudioProducer : public BBufferProducer,
  public BMediaEventLooper
{
public:
        AudioProducer(struct Generator *_gen, float sampleRate,
                      int nchans, size_t bufferSize);
        ~AudioProducer();

/// BMediaNode abstract method instantiation:
        BMediaAddOn* AddOn(     int32* internal_id) const {return NULL;}

///  BBufferProducer inheritance:
//    Abstract method instantiations:

        status_t FormatSuggestionRequested(
                media_type type,
                int32 quality,
                media_format* format);

        status_t FormatProposal(
                const media_source& output,
                media_format* format);

        status_t FormatChangeRequested(
                const media_source& source,
                const media_destination& destination,
                media_format* io_format,
                int32* _deprecated_) {return B_ERROR;}

        status_t GetNextOutput( /* cookie starts as 0 */
                int32* cookie,
                media_output* out_output);

        status_t DisposeOutputCookie(int32 cookie) {return B_OK;}

        status_t SetBufferGroup(
                const media_source& for_source,
                BBufferGroup* group);

        status_t PrepareToConnect(
                const media_source& what,
                const media_destination& where,
                media_format* format,
                media_source* out_source,
                char* out_name);

        void Connect(
                status_t error,
                const media_source& source,
                const media_destination& destination,
                const media_format& format,
                char* io_name);

        void Disconnect(
                const media_source& what,
                const media_destination& where);

        void LateNoticeReceived(
                const media_source& what,
                bigtime_t how_much,
                bigtime_t performance_time);

        void EnableOutput(
                const media_source & what,
                bool enabled,
                int32* _deprecated_);

/// End of BBufferProducer Abstract instantiations ///

        status_t GetLatency(
                bigtime_t* out_latency);

        status_t HandleMessage(
                int32 message,
                const void* data,
                size_t size);

        void LatencyChanged(
                const media_source& source,
                const media_destination& destination,
                bigtime_t new_latency,
                uint32 flags);


///  BMediaLooper inheritance:
/// Abstract instantiation:
        void HandleEvent(
                const media_timed_event* event,
                bigtime_t lateness,
                bool realTimeEvent = false);
////////////////////////////////////////

        void NodeRegistered();

        void Start(bigtime_t performance_time);

        void SetRunMode(run_mode mode);

private:
        void AllocateBuffers();
        BBuffer* FillNextBuffer(bigtime_t event_time);

        Generator *gen;

        BBufferGroup* mBufferGroup;
        bigtime_t mLatency, mInternalLatency;
        media_output mOutput;
        bool mOutputEnabled;
        media_format mPreferredFormat;

        size_t mBufferSize;
        BMediaNode::run_mode mRunMode;
        int mChans;

        uint64 mSamplesSent;
        bigtime_t mStartTime;

};

////////////////////////////////////////////////////////////////


AudioProducer::AudioProducer(Generator *_gen, float sampleRate,
                             int nchans, size_t bufferSize)
        :       BMediaNode("Csound"),
                BBufferProducer(B_MEDIA_RAW_AUDIO),
                BMediaEventLooper(),
                gen(_gen),
                mBufferGroup(NULL),
                mLatency(0),
                mInternalLatency(0),
                mOutputEnabled(true),
                mBufferSize(bufferSize),
                mRunMode(gRunMode),
                mChans(nchans)
{
        mPreferredFormat.type = B_MEDIA_RAW_AUDIO;
        mPreferredFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
        mPreferredFormat.u.raw_audio.channel_count = mChans;
        mPreferredFormat.u.raw_audio.frame_rate = sampleRate; // measured in Hertz
        mPreferredFormat.u.raw_audio.byte_order =
          (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;

        if (mBufferSize)
                mPreferredFormat.u.raw_audio.buffer_size = mBufferSize;
        else
                mPreferredFormat.u.raw_audio.buffer_size
                 = media_raw_audio_format::wildcard.buffer_size;

        mOutput.destination = media_destination::null;
        mOutput.format = mPreferredFormat;
}


AudioProducer::~AudioProducer()
{
        Quit();
}


status_t
AudioProducer::FormatSuggestionRequested(media_type type, int32 /*quality*/,
                                         media_format* format)
{
        if (!format) return B_BAD_VALUE;

        *format = mPreferredFormat;
        if (type == B_MEDIA_UNKNOWN_TYPE) type = B_MEDIA_RAW_AUDIO;
        if (type != B_MEDIA_RAW_AUDIO) return B_MEDIA_BAD_FORMAT;
        else return B_OK;
}


status_t
AudioProducer::FormatProposal(const media_source& output, media_format* format)
{
        if (output != mOutput.source)
                return B_MEDIA_BAD_SOURCE;

        media_type requestedType = format->type;
        *format = mPreferredFormat;
        if ((requestedType != B_MEDIA_UNKNOWN_TYPE) &&
            (requestedType != B_MEDIA_RAW_AUDIO))
                return B_MEDIA_BAD_FORMAT;
        else return B_OK;     // raw audio or wildcard type, either is okay by us
}


status_t
AudioProducer::GetNextOutput(int32* cookie, media_output* out_output)
{
        if (0 == *cookie)
        {
                *out_output = mOutput;
                *cookie += 1;
                return B_OK;
        }
        else return B_BAD_INDEX;
}


status_t
AudioProducer::SetBufferGroup(const media_source& for_source,
                              BBufferGroup* newGroup)
{
        if (for_source != mOutput.source) return B_MEDIA_BAD_SOURCE;

        if (newGroup == mBufferGroup) return B_OK;

        delete mBufferGroup;            // waits for all buffers to recycle
        if (newGroup != NULL)
                mBufferGroup = newGroup;
        else
        {
                size_t size = mOutput.format.u.raw_audio.buffer_size;
                int32 count =
                  int32(mLatency / BufferDuration() + 1 + 1 + gXtraBuffers);
                mBufferGroup = new BBufferGroup(size, count);
        }
        return B_OK;
}


status_t
AudioProducer::GetLatency(bigtime_t* out_latency)
{
        *out_latency = EventLatency() + SchedulingLatency();
        return B_OK;
}


status_t
AudioProducer::PrepareToConnect(const media_source& what,
                                const media_destination& where,
                                media_format* format, media_source* out_source,
                                char* out_name)
{
        if (what != mOutput.source) return B_MEDIA_BAD_SOURCE;
        if (mOutput.destination != media_destination::null)
          return B_MEDIA_ALREADY_CONNECTED;
        if (format->type != B_MEDIA_RAW_AUDIO)
                return B_MEDIA_BAD_FORMAT;
        else if (format->u.raw_audio.format !=
                 media_raw_audio_format::B_AUDIO_FLOAT)
                return B_MEDIA_BAD_FORMAT;

        if (format->u.raw_audio.buffer_size ==
            media_raw_audio_format::wildcard.buffer_size)
                format->u.raw_audio.buffer_size = mBufferSize;

        mOutput.destination = where;
        mOutput.format = *format;
        *out_source = mOutput.source;
        strncpy(out_name, mOutput.name, B_MEDIA_NAME_LENGTH);
        return B_OK;
}


void
AudioProducer::Connect(status_t error, const media_source& source,
                       const media_destination& destination,
                       const media_format& format, char* io_name)
{
        if (error)
        {
                mOutput.destination = media_destination::null;
                mOutput.format = mPreferredFormat;
                return;
        }
        mOutput.destination = destination;
        mOutput.format = format;
        strncpy(io_name, mOutput.name, B_MEDIA_NAME_LENGTH);

        media_node_id id;
        FindLatencyFor(mOutput.destination, &mLatency, &id);

        size_t samplesPerBuffer
                = mOutput.format.u.raw_audio.buffer_size / (sizeof(float)*mChans);

        SetEventLatency(mLatency + gAddLatency);    // ignore internal time for now

        {
                bigtime_t realtime, perftime;
                float drift;
                TimeSource()->GetTime(&perftime, &realtime, & drift);
        }
        bigtime_t duration
                = bigtime_t(1000000) * samplesPerBuffer /
                  bigtime_t(mOutput.format.u.raw_audio.frame_rate);
        SetBufferDuration(duration);
        if (!mBufferGroup) AllocateBuffers();
}


void
AudioProducer::Disconnect(const media_source& what, const media_destination& where)
{
        DPRINTF(("trying to disconnect AudioProducer\n");)
        if ((where == mOutput.destination) && (what == mOutput.source))
        {
                mOutput.destination = media_destination::null;
                mOutput.format = mPreferredFormat;
                delete mBufferGroup;
                mBufferGroup = NULL;
        }
}


void
AudioProducer::LateNoticeReceived(const media_source& what, bigtime_t how_much,
                                  bigtime_t performance_time)
{
        if (what == mOutput.source)
        {
                if (RunMode() == B_RECORDING)
                {
                        // nothing to do
                }
                else if (RunMode() == B_INCREASE_LATENCY)
                {
                        mInternalLatency += how_much;
                        SetEventLatency(mLatency + mInternalLatency);
                }
                else
                {
                        size_t nSamples
                          = mOutput.format.u.raw_audio.buffer_size /
                          (sizeof(float)*mChans);
                        mSamplesSent += nSamples;
                }
        }
}


void
AudioProducer::EnableOutput(const media_source& what, bool enabled,
                            int32* _deprecated_)
{
        if (what == mOutput.source)
                mOutputEnabled = enabled;
}


status_t
AudioProducer::HandleMessage(int32 message, const void* data, size_t size)
{
        return B_ERROR;
}


void
AudioProducer::LatencyChanged(const media_source& source,
                              const media_destination& destination,
                              bigtime_t new_latency, uint32 flags)
{
        if ((source == mOutput.source) && (destination == mOutput.destination))
        {
                mLatency = new_latency;
                SetEventLatency(mLatency + mInternalLatency);
        }
}


void
AudioProducer::NodeRegistered()
{
        SetPriority(B_REAL_TIME_PRIORITY);
        Run();
        mOutput.source.port = ControlPort();
        mOutput.source.id = 0;
        mOutput.node = Node();
        ::strcpy(mOutput.name, "Csound Output");
}


void
AudioProducer::Start(bigtime_t performance_time)
{
        BMediaRoster::Roster()->SetRunModeNode(Node(), mRunMode);
        BMediaEventLooper::Start(performance_time);
}


void
AudioProducer::SetRunMode(run_mode mode)
{
        if (B_OFFLINE == mode)
                ReportError(B_NODE_FAILED_SET_RUN_MODE);
}


void
AudioProducer::HandleEvent(const media_timed_event* event,
                           bigtime_t lateness, bool realTimeEvent)
{
        switch (event->type)
        {
        case BTimedEventQueue::B_START:
                if (RunState() != B_STARTED)
                {
                        mSamplesSent = 0;
                        mStartTime = event->event_time;
                        media_timed_event
                          firstBufferEvent(mStartTime,
                                           BTimedEventQueue::B_HANDLE_BUFFER);
                        EventQueue()->AddEvent(firstBufferEvent);
                }
                break;

        case BTimedEventQueue::B_STOP:
                DPRINTF(("Stop event arrived\n");)
                EventQueue()->FlushEvents(0, BTimedEventQueue::B_ALWAYS,
                                          true, BTimedEventQueue::B_HANDLE_BUFFER);
                break;

        case BTimedEventQueue::B_HANDLE_BUFFER:
                {
                  if ((RunState() == BMediaEventLooper::B_STARTED)
                      && (mOutput.destination != media_destination::null))
                    {
                      BBuffer* buffer = FillNextBuffer(event->event_time);
                      if (buffer)
                        {
                          status_t err = B_ERROR;
                          if (mOutputEnabled)
                            err = SendBuffer(buffer, mOutput.source,
                                             mOutput.destination);
                          if (err) buffer->Recycle();
                        }
                      size_t nSamples =
                        mOutput.format.u.raw_audio.buffer_size /
                        (sizeof(float)*mChans);
                      mSamplesSent += nSamples;

                      bigtime_t nextEvent = mStartTime +
                        bigtime_t(double(mSamplesSent) /
                                  double(mOutput.format.u.raw_audio.frame_rate)
                                  * 1000000.0);
                      media_timed_event
                        nextBufferEvent(nextEvent,
                                        BTimedEventQueue::B_HANDLE_BUFFER);
                      EventQueue()->AddEvent(nextBufferEvent);
                    }
                }
                break;

        default:
                break;
        }
}


void
AudioProducer::AllocateBuffers()
{
        size_t size = mOutput.format.u.raw_audio.buffer_size;
        int32 count = int32(mLatency / BufferDuration() + 1 + 1 + gXtraBuffers);
        mBufferGroup = new BBufferGroup(size, count);
}


static int nclr = 0;

BBuffer*
AudioProducer::FillNextBuffer(bigtime_t event_time)
{
        BBuffer* buf =
          mBufferGroup->RequestBuffer(mOutput.format.u.raw_audio.buffer_size,
                                      BufferDuration());
        if (!buf) {
                DPRINTF(("Couldn't get buffer!\n");)
                return NULL;
        }

        // number of samples (not frames) to be copied
        int32 numSamples = gen->mXferSize / gen->mSampleSize;
        float* data = (float*) buf->Data();

        if (gen->mDataBuf) {
          for (int32 indx=0; indx < numSamples; indx++)
            *data++ = (float)gen->mDataBuf[indx];
          gen->mDataBuf = NULL;
          release_sem(gen->cs_sem);
        } else {// I guess we've finished, so clear the buffers
          memset(data, 0, mOutput.format.u.raw_audio.buffer_size);
          DPRINTF(("cleared buffer %d\n", ++nclr);)
        }

        media_header* hdr = buf->Header();
        hdr->type = B_MEDIA_RAW_AUDIO;
        hdr->size_used = mOutput.format.u.raw_audio.buffer_size;
        hdr->time_source = TimeSource()->ID();

        bigtime_t stamp;
        if (RunMode() == B_RECORDING)
          stamp = event_time;
        else
          stamp = mStartTime + bigtime_t(double(mSamplesSent) /
                                         double(mOutput.format.u.raw_audio.frame_rate) * 1000000.0);
        hdr->start_time = stamp;

        return buf;
}



///////////////////////////////////////////////////

struct Generator_Private {
        media_node producer, consumer;
        media_source source;
        media_destination destination;
        media_format format;
        class AudioProducer* mAudioNode;
        bool mIsConnected;
        bool mIsRunning;
        media_node mTimeSource;
};


static void ErrorCheck(status_t err, const char* msg)
{
        if (err)
        {
                fprintf(stderr, "* FATAL ERROR (%s): %s\n", strerror(err), msg);
                exit(1);
        }
}


Generator::Generator(float sampleRate, int nchans, size_t bufferSize,
                     int32 sampleSize) :
        mBufSize(bufferSize),
        mFrameRate(sampleRate),
        mChans(nchans),
        mSampleSize(sampleSize),
        mDataBuf(NULL),
        mXferSize(mBufSize),
        state(new Generator_Private())
{
        cs_sem = create_sem(0, "Csound_wait");  // intially locked
}


Generator::~Generator()
{
        BMediaRoster* r = BMediaRoster::Roster();
        status_t err;
        DPRINTF(("destructing Generator\n");)

        if (state->mIsRunning) {
                DPRINTF(("stopping producer node\n");)
                err = r->StopNode(state->producer, 0, true);
                if (err)
                {
                        fprintf(stderr, "* Error stopping producer:  %ld (%s)\n",
                                err, strerror(err));
                }
        }
        if (state->mIsConnected)
        {
                DPRINTF(("disconnecting Generator\n");)
                err = r->Disconnect(state->producer.node, state->source,
                        state->consumer.node, state->destination);
                if (err)
                {
                        fprintf(stderr, "* Error disconnecting nodes:  %ld (%s)\n",
                                err, strerror(err));
                }

                DPRINTF(("releasing nodes\n");)
                r->ReleaseNode(state->producer);
                r->ReleaseNode(state->consumer);
                delete state->mAudioNode;
        }
        DPRINTF(("Generator destructed\n");)
}


int Generator::RunAudio() {
        status_t err;

        BMediaRoster* r = BMediaRoster::Roster();

        state->mAudioNode = new AudioProducer(this, mFrameRate, mChans, mBufSize);

        err = r->RegisterNode(state->mAudioNode);
        ErrorCheck(err, "unable to register Csound node!\n");
        r->GetNodeFor(state->mAudioNode->Node().node, &state->producer);

        err = r->GetAudioMixer(&state->consumer);
        ErrorCheck(err, "unable to get the system mixer");

        r->GetTimeSource(&state->mTimeSource);
        r->SetTimeSourceFor(state->producer.node, state->mTimeSource.node);

        media_input mixerInput;
        media_output soundOutput;
        int32 count = 1;
        err = r->GetFreeOutputsFor(state->producer, &soundOutput, 1, &count);
        ErrorCheck(err, "unable to get a free output from the producer node");
        count = 1;
        err = r->GetFreeInputsFor(state->consumer, &mixerInput, 1, &count);
        ErrorCheck(err, "unable to get a free input to the mixer");

        media_format format;
        format.type = B_MEDIA_RAW_AUDIO;
        format.u.raw_audio = media_raw_audio_format::wildcard;
        err = r->Connect(soundOutput.source, mixerInput.destination,
                         &format, &soundOutput, &mixerInput);
        ErrorCheck(err, "unable to connect nodes");

        state->format = format;
        state->source = soundOutput.source;
        state->destination = mixerInput.destination;
        state->mIsConnected = true;

        r->SetRunModeNode(state->producer, gRunMode);

        BTimeSource* ts = r->MakeTimeSourceFor(state->producer);
        if (!ts)
        {
                fprintf(stderr,
                        "* ERROR - MakeTimeSourceFor(producer) returned NULL!\n");
                exit(1);
        }

        bigtime_t latency = 0;
        r->GetLatencyFor(state->producer, &latency);
        r->StartNode(state->producer, ts->Now() + latency);
        ts->Release();
        state->mIsRunning = true;
        return 0;       // always for now...
}
