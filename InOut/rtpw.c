/*
  rtpw.c: pipewire audio IO backend

  Copyright (C) 2025 V Lazzarini

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
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>
#include <spa/utils/ringbuffer.h>
#include <pthread.h>
#include <stdlib.h>
#include "csdl.h"

#ifdef USE_DOUBLE
#define CS_FLOAT_FORMAT SPA_AUDIO_FORMAT_F64
#else
#define CS_FLOAT_FORMAT SPA_AUDIO_FORMAT_F32
#endif

typedef struct {
  struct pw_thread_loop *loop;
  struct pw_loop *cloop;
  struct pw_stream *stream;
  struct spa_pod_builder b;
  struct spa_ringbuffer ring;
  struct spa_audio_info format;
  uint8_t pbuffer[1024];
  uint8_t *cbuffer;
  int32_t nchnls;
  cs_float sr;      /* requested stream rate, <= 0 if following the system */
  cs_float sysr;    /* sampling rate negotiated with PipeWire */
  cs_float graphr;  /* rate of the PipeWire graph clock (the system rate) */
  int32_t buframes;
  int32_t cbflag;
  int32_t drained;
  int32_t is_input;
  int32_t published;     /* handed to Csound; channel layout must not change */
  int32_t format_error;  /* negotiated format has an unsupported channel count */
  enum pw_stream_state state;
  uint32_t progress_seq;
  CSOUND *csound;
} RTPW;

/* The PipeWire graph rate is distinct from the client stream's negotiated
   rate; PipeWire may resample between them. */
static cs_float rtpw_system_sr(RTPW *rtpw) {
  return rtpw->graphr > 0 ? rtpw->graphr : -1.0f;
}

static int32_t rtpw_rate_matches(cs_float actual, cs_float required) {
  cs_float diff = actual - required;
  if (diff < 0)
    diff = -diff;
  return actual > 0 && required > 0 && diff < FL(0.01);
}

/* PipeWire rates are integers. Use the value actually offered to PipeWire
   when validating the negotiated rate as well. */
static uint32_t rtpw_offered_rate(cs_float rate) {
  return rate > 0 ? (uint32_t) (rate + FL(0.5)) : 0;
}

/* Validate the negotiated client rate against Csound's chosen rate. In
   system-rate mode, the graph rate is the chosen rate. */
static int32_t rtpw_validate_rates(RTPW *rtpw, cs_float *system_sr) {
  cs_float graph_sr = rtpw_system_sr(rtpw);
  cs_float required_sr = rtpw->sr > 0 ?
                         (cs_float) rtpw_offered_rate(rtpw->sr) : graph_sr;

  if (!rtpw_rate_matches(rtpw->sysr, required_sr))
    return 0;
  rtpw->sr = required_sr;
  *system_sr = graph_sr;
  return 1;
}

/* Wait for format negotiation and the graph clock. The timeout is restarted
   when PipeWire reports meaningful progress, rather than being a fixed
   deadline from the start of the connection. */
static int32_t rtpw_wait_ready(RTPW *rtpw, int32_t timeout_ms) {
  struct pw_time t;
  struct timespec deadline;
  uint32_t progress;
  int32_t ready = 0, err;

  if (pw_thread_loop_get_time(rtpw->loop, &deadline,
                              (int64_t) timeout_ms * 1000000) < 0)
    return 0;

  pw_thread_loop_lock(rtpw->loop);
  progress = rtpw->progress_seq;
  while (!ready && rtpw->state != PW_STREAM_STATE_ERROR) {
    if (rtpw->state == PW_STREAM_STATE_STREAMING && rtpw->graphr <= 0 &&
        pw_stream_get_time_n(rtpw->stream, &t, sizeof(t)) >= 0 &&
        t.rate.num != 0 && t.rate.denom != 0)
      rtpw->graphr = (cs_float) t.rate.denom / (cs_float) t.rate.num;

    ready = !rtpw->format_error && rtpw->sysr > 0 && rtpw->graphr > 0;
    if (ready || rtpw->format_error || rtpw->state == PW_STREAM_STATE_ERROR)
      break;

    if (progress != rtpw->progress_seq) {
      progress = rtpw->progress_seq;
      if (pw_thread_loop_get_time(rtpw->loop, &deadline,
                                  (int64_t) timeout_ms * 1000000) < 0)
        break;
      continue;
    }
    err = pw_thread_loop_timed_wait_full(rtpw->loop, &deadline);
    if (err == -EINTR)
      continue;
    if (err != 0)
      break;
  }
  pw_thread_loop_unlock(rtpw->loop);
  return ready;
}

/* Report why rtpw_wait_ready() failed. */
static void rtpw_report_open_error(CSOUND *csound, RTPW *rtpw,
                                   const char *direction) {
  if (rtpw->format_error)
    csound->ErrorMsg(csound, Str("rtpw: %s stream negotiated %d channels; "
                                 "expected %d\n"), direction,
                     (int32_t) rtpw->format.info.raw.channels, rtpw->nchnls);
  else
    csound->ErrorMsg(csound, Str("rtpw: timed out waiting for the %s "
                                 "stream and graph rate\n"), direction);
}


static void rtpw_out_callback(void *p) {
  RTPW *rtpw = (RTPW *) p;
  struct pw_buffer *pwbuf;
  struct spa_buffer *spabuf;
  uint8_t *bufp;
  uint32_t i, rem, sil;
  int32_t n, frames, fbytes = (int32_t) (rtpw->nchnls*sizeof(cs_float));

  if ((pwbuf = pw_stream_dequeue_buffer(rtpw->stream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  spabuf = pwbuf->buffer;
  if ((bufp = spabuf->datas[0].data) == NULL) return;
  frames = spabuf->datas[0].maxsize / fbytes;

  n = spa_ringbuffer_get_read_index(&rtpw->ring, &i);
  if (pwbuf->requested)
    frames = SPA_MIN((int32_t)pwbuf->requested,frames);
  rem = n > 0 ? SPA_MIN(n,frames) : 0;

  sil = frames - rem;
  if(rem > 0){
    spa_ringbuffer_read_data(&rtpw->ring, rtpw->cbuffer,
			     rtpw->buframes * fbytes,
                             (i % rtpw->buframes) * fbytes,
                             bufp, rem * fbytes);
    spa_ringbuffer_read_update(&rtpw->ring, i + rem);
  }
  if(sil  > 0){
    CSOUND *csound = rtpw->csound;
    csound->Warning(csound, Str("%d dropped output frames\n"), sil);
    memset(SPA_PTROFF(bufp, rem*fbytes, void), 0, sil*fbytes);
  }
  spabuf->datas[0].chunk->offset = 0;
  spabuf->datas[0].chunk->stride = fbytes;
  spabuf->datas[0].chunk->size = frames*fbytes;
  pw_stream_queue_buffer(rtpw->stream, pwbuf);
  spa_system_eventfd_write(rtpw->cloop->system, rtpw->cbflag, 1);
}

static void rtpw_drained(void *p) {
  RTPW *rtpw = (RTPW *) p;
  rtpw->drained = 1;
  pw_thread_loop_signal(rtpw->loop, false);
}

static void rtpw_state_changed(void *p, enum pw_stream_state old,
                               enum pw_stream_state state, const char *error) {
  RTPW *rtpw = (RTPW *) p;
  IGN(old);
  IGN(error);
  rtpw->state = state;
  rtpw->progress_seq++;
  pw_thread_loop_signal(rtpw->loop, false);
}

static void rtpw_play(CSOUND *csound, const cs_float *outbuf, int32_t nbytes){
  RTPW *rtpw = (RTPW *) *csound->GetRtPlayUserData(csound);
  /* Output setup rejects a negotiated channel count that differs from
     parm->nChannels, so nchnls is also the channel count of outbuf. */
  int32_t nframes = nbytes/(sizeof(cs_float)*rtpw->nchnls);
  int32_t rem, fbytes = sizeof(cs_float)*rtpw->nchnls, n;
  uint32_t i;
  uint64_t cnt;

  while(nframes > 0) {
    while (1) {
      n = spa_ringbuffer_get_write_index(&rtpw->ring, &i);
      spa_assert(n >= 0);
      spa_assert(n <= rtpw->buframes);
      rem = rtpw->buframes - n;
      if (rem > 0) break;
      spa_system_eventfd_read(rtpw->cloop->system, rtpw->cbflag, &cnt);
    }
    if(rem > nframes) rem = nframes;
    spa_ringbuffer_write_data(&rtpw->ring,rtpw->cbuffer,rtpw->buframes*fbytes,
			      (i%rtpw->buframes)*fbytes,outbuf,rem*fbytes);
    nframes -= rem;
    outbuf += rem*rtpw->nchnls;
    spa_ringbuffer_write_update(&rtpw->ring, i + rem);
  }
}

static void parm_callback(void *p, uint32_t id, const struct spa_pod *param);
static void rtpw_close_one(CSOUND *csound, void **data, int32_t output);
static const struct pw_stream_events stream_events_out = {
  PW_VERSION_STREAM_EVENTS,
  .state_changed = rtpw_state_changed,
  .process = rtpw_out_callback,
  .param_changed = parm_callback,
  .drained = rtpw_drained,
};

static int32_t list_outputs(CSOUND *csound);
/**
   open pipewire for output
*/
static int32_t rtpw_open_out(CSOUND *csound, const csRtAudioParams *parm) {
  void **p;
  struct pw_properties *props;
  const struct spa_pod *params[1];
  RTPW *rtpw;
  cs_float sysr, required_sr;
  int32_t valid, format_error;

  if (csound->GetDebug(csound))
    list_outputs(csound);
  p = (void**) csound->GetRtPlayUserData(csound);
  if(*p != NULL) return 0;
  if (UNLIKELY(parm->nChannels < 1)) {
    csound->ErrorMsg(csound, Str("rtpw: invalid number of output channels\n"));
    return CSOUND_ERROR;
  }
  rtpw = (RTPW *) csound->Calloc(csound, sizeof(RTPW));
  rtpw->cbuffer = csound->Calloc(csound,sizeof(cs_float)*parm->bufSamp_HW*
				 parm->nChannels);
  rtpw->nchnls = parm->nChannels;
  rtpw->sr = parm->sampleRate;
  rtpw->sysr = -1.0f;
  rtpw->graphr = -1.0f;
  rtpw->is_input = 0;
  rtpw->state = PW_STREAM_STATE_UNCONNECTED;
  rtpw->buframes = parm->bufSamp_HW;
  rtpw->csound = csound;
  csound->DebugMsg(csound, Str("hw frames: %d sw frames: %d\n"), rtpw->buframes,
		  parm->bufSamp_SW);

  rtpw->loop = pw_thread_loop_new("csound-out", NULL);
  rtpw->cloop = pw_thread_loop_get_loop(rtpw->loop);
  pw_thread_loop_lock(rtpw->loop);

  props = pw_properties_new(PW_KEY_MEDIA_TYPE,"Audio",
			    PW_KEY_MEDIA_CATEGORY,"Playback",
			    PW_KEY_MEDIA_ROLE,"Music",NULL);
  if(parm->devName != NULL)
    pw_properties_set(props, PW_KEY_TARGET_OBJECT, parm->devName);

  rtpw->stream = pw_stream_new_simple(rtpw->cloop,
				      "csound-out", props,
				      &stream_events_out,rtpw);


  rtpw->b = SPA_POD_BUILDER_INIT(rtpw->pbuffer, sizeof(rtpw->pbuffer));
  /* a non-positive rate means the stream should follow the system rate */
  params[0] = spa_format_audio_raw_build(&rtpw->b, SPA_PARAM_EnumFormat,
					 &SPA_AUDIO_INFO_RAW_INIT
					 (.format = CS_FLOAT_FORMAT,
					  .channels = parm->nChannels,
					  .rate =
					    rtpw_offered_rate(parm->sampleRate)));
  rtpw->cbflag =
    spa_system_eventfd_create(rtpw->cloop->system, SPA_FD_CLOEXEC);
  spa_ringbuffer_init(&rtpw->ring);
  pw_stream_connect(rtpw->stream,
		    PW_DIRECTION_OUTPUT,
		    PW_ID_ANY,
		    PW_STREAM_FLAG_AUTOCONNECT |
		    PW_STREAM_FLAG_MAP_BUFFERS |
		    PW_STREAM_FLAG_RT_PROCESS,
		    params, 1);
  pw_thread_loop_start(rtpw->loop);
  pw_thread_loop_unlock(rtpw->loop);

  /* Wait for both the negotiated format and the graph clock. */
  if (UNLIKELY(!rtpw_wait_ready(rtpw, 2000))) {
    rtpw_report_open_error(csound, rtpw, "output");
    rtpw_close_one(csound, (void **) &rtpw, 1);
    return CSOUND_ERROR;
  }
  required_sr = rtpw->sr > 0 ? (cs_float) rtpw_offered_rate(rtpw->sr)
                             : rtpw->graphr;
  /* rtpw_wait_ready() releases the loop lock: a format callback could
     still set format_error before the stream is published. publish
     under the lock so csound never receives such a stream. */
  pw_thread_loop_lock(rtpw->loop);
  format_error = rtpw->format_error;
  valid = !format_error && rtpw_validate_rates(rtpw, &sysr);
  if (valid)
    rtpw->published = 1;
  pw_thread_loop_unlock(rtpw->loop);
  if (UNLIKELY(!valid)) {
    if (format_error)
      rtpw_report_open_error(csound, rtpw, "output");
    else
      csound->ErrorMsg(csound,
		       Str("rtpw: negotiated output rate %.2f does not match "
			   "required rate %.2f\n"), rtpw->sysr, required_sr);
    rtpw_close_one(csound, (void **) &rtpw, 1);
    return CSOUND_ERROR;
  }
  csound->GetSystemSr(csound, sysr);
  *p = (void *) rtpw;
  return CSOUND_SUCCESS;
}

static void rtpw_in_callback(void *p) {
  RTPW *rtpw = (RTPW *) p;
  struct pw_buffer *pwbuf;
  struct spa_buffer *spabuf;
  uint8_t *bufp;
  uint32_t i;
  int32_t rem, n, frames, fbytes = (int32_t) (rtpw->nchnls*sizeof(cs_float));

  if ((pwbuf = pw_stream_dequeue_buffer(rtpw->stream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  spabuf = pwbuf->buffer;
  if ((bufp = spabuf->datas[0].data) == NULL) return;
  if (UNLIKELY(rtpw->nchnls < 1)) return;
  frames = (int32_t) (spabuf->datas[0].chunk->size / (uint32_t) fbytes);
  if (UNLIKELY(frames > (int32_t) (spabuf->datas[0].maxsize /
				   (uint32_t) fbytes)))
    frames = (int32_t) (spabuf->datas[0].maxsize / (uint32_t) fbytes);

  while(frames > 0) {
    n = spa_ringbuffer_get_write_index(&rtpw->ring, &i);
    if(n < 0 || n > rtpw->buframes) break; // under/overrun
    rem = rtpw->buframes - n;

    if(rem > 0){
      if(rem > frames) rem = frames;
      spa_ringbuffer_write_data(&rtpw->ring, rtpw->cbuffer,
				rtpw->buframes * fbytes,
				(i % rtpw->buframes) * fbytes,
				bufp, rem * fbytes);
      spa_ringbuffer_write_update(&rtpw->ring, i + rem);
      frames -= rem;
      bufp += rem*fbytes;
    } else break; // no more space
  }
  pw_stream_queue_buffer(rtpw->stream, pwbuf);
}


static int32_t rtpw_record(CSOUND *csound, cs_float *inbuf, int32_t nbytes) {
  RTPW *rtpw = (RTPW *) *csound->GetRtRecordUserData(csound);
  uint32_t i, rem, nframes;
  int32_t n, fbytes = rtpw->nchnls*sizeof(cs_float);
  int32_t wbytes = 0;
  int32_t nchnls_i = (int32_t) csound->GetNchnls_i(csound);

  if (UNLIKELY(nchnls_i < 1 || rtpw->nchnls < 1))
    return 0;
  nframes = (uint32_t) (nbytes/(nchnls_i*sizeof(cs_float)));


  if(nchnls_i == rtpw->nchnls) {
    while(nframes > 0) {
      n = spa_ringbuffer_get_read_index(&rtpw->ring, &i);
      if (n <= 0) {   /* underrun: pad with silence */
	memset(inbuf, 0, nframes * fbytes);
	wbytes += (int32_t) nframes * fbytes;
	break;
      }
      rem = SPA_MIN(n, nframes);
      spa_ringbuffer_read_data(&rtpw->ring, rtpw->cbuffer,
			       rtpw->buframes * fbytes,
			       (i % rtpw->buframes) * fbytes,
			       inbuf, rem * fbytes);
      spa_ringbuffer_read_update(&rtpw->ring, i + rem);
      inbuf += rem*rtpw->nchnls;
      nframes -= rem;
      wbytes += rem * fbytes;
    }
  } else { // channel-count conversion
    uint32_t frame, available;
    int32_t nchnls = nchnls_i;
    int32_t copy_nchnls = SPA_MIN(nchnls, rtpw->nchnls);

    n = spa_ringbuffer_get_read_index(&rtpw->ring, &i);
    available = n > 0 ? SPA_MIN((uint32_t) n, nframes) : 0;
    for (frame = 0; frame < available; frame++) {
      cs_float *dst = inbuf + (size_t) frame * nchnls;
      memset(dst, 0, (size_t) nchnls * sizeof(cs_float));
      spa_ringbuffer_read_data(&rtpw->ring, rtpw->cbuffer,
			       rtpw->buframes * fbytes,
			       ((i + frame) % rtpw->buframes) * fbytes,
			       dst, (uint32_t) copy_nchnls * sizeof(cs_float));
    }
    if (available > 0)
      spa_ringbuffer_read_update(&rtpw->ring, i + available);
    if (available < nframes)
      memset(inbuf + (size_t) available * nchnls, 0,
	     (size_t) (nframes - available) * nchnls * sizeof(cs_float));
    wbytes = (int32_t) (nframes * (uint32_t) nchnls * sizeof(cs_float));
  }
  return wbytes;
}

static void
parm_callback(void *p, uint32_t id, const struct spa_pod *param)
{
  RTPW  *rtpw = (RTPW *) p;
  CSOUND *csound = rtpw->csound;

  if (param == NULL || id != SPA_PARAM_Format)
    return;

  if (spa_format_parse(param,
		       &rtpw->format.media_type,
		       &rtpw->format.media_subtype) < 0)
    return;

  if (rtpw->format.media_type != SPA_MEDIA_TYPE_audio ||
      rtpw->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
    return;

  spa_format_audio_raw_parse(param, &rtpw->format.info.raw);
  rtpw->sysr = (cs_float) rtpw->format.info.raw.rate;

  if (rtpw->is_input && rtpw->sr > 0 && rtpw->sysr > 0 &&
      rtpw->sysr != rtpw->sr)
    csound->Warning(csound, Str("rtpw: mismatched input sampling rate,\n"
		    "system-sr %.1f csound-sr %.1f\n"), rtpw->sysr,
		    rtpw->sr);

  if(rtpw->nchnls != (int32_t) rtpw->format.info.raw.channels &&
     rtpw->format.info.raw.channels > 0) {
    /* csound's channel layout is fixed for the performance, so the stream
       cannot be reconfigured during performance. */
    rtpw->format_error = 1;
    if (rtpw->published)
      csound->ErrorMsg(csound, Str("rtpw: stream channel count changed from "
                                   "%d to %d\n"), rtpw->nchnls,
                       (int32_t) rtpw->format.info.raw.channels);
    pw_stream_set_error(rtpw->stream, -EINVAL,
                        "rtpw: unsupported channel count change from %d "
                        "to %d", rtpw->nchnls,
                        (int32_t) rtpw->format.info.raw.channels);
  }

  rtpw->progress_seq++;
  pw_thread_loop_signal(rtpw->loop, false);
}

static const struct pw_stream_events stream_events_in = {
  PW_VERSION_STREAM_EVENTS,
  .state_changed = rtpw_state_changed,
  .process = rtpw_in_callback,
  .param_changed = parm_callback,
};

/**
   Open pipewire for input
*/
static int32_t rtpw_open_in(CSOUND *csound, const csRtAudioParams *parm){
  void **p;
  struct pw_properties *props;
  const struct spa_pod *params[1];
  RTPW *rtpw;
  cs_float sysr, required_sr;
  int32_t valid, format_error;

  p = (void**) csound->GetRtRecordUserData(csound);
  if(*p != NULL) return 0;
  if (UNLIKELY(parm->nChannels < 1)) {
    csound->ErrorMsg(csound, Str("rtpw: invalid number of input channels\n"));
    return CSOUND_ERROR;
  }
  rtpw = (RTPW *) csound->Calloc(csound, sizeof(RTPW));
  rtpw->cbuffer = csound->Calloc(csound,sizeof(cs_float)*parm->bufSamp_HW*
				 parm->nChannels);
  rtpw->nchnls = parm->nChannels;
  rtpw->sr = parm->sampleRate;
  rtpw->sysr = -1.0f;
  rtpw->graphr = -1.0f;
  rtpw->is_input = 1;
  rtpw->state = PW_STREAM_STATE_UNCONNECTED;
  rtpw->buframes = parm->bufSamp_HW;
  rtpw->csound = csound;
  csound->DebugMsg(csound, Str("pwin - hw frames: %d sw frames: %d\n"), rtpw->buframes,
		  parm->bufSamp_SW);

  rtpw->loop = pw_thread_loop_new("csound-in", NULL);
  rtpw->cloop = pw_thread_loop_get_loop(rtpw->loop);
  pw_thread_loop_lock(rtpw->loop);

  props = pw_properties_new(PW_KEY_MEDIA_TYPE,"Audio",
			    PW_KEY_MEDIA_CATEGORY,"Record",
			    PW_KEY_MEDIA_ROLE,"Music",NULL);

  if(parm->devName != NULL)
    pw_properties_set(props, PW_KEY_TARGET_OBJECT, parm->devName);

  rtpw->stream = pw_stream_new_simple(rtpw->cloop,
				      "csound-in", props,
				      &stream_events_in,rtpw);

  rtpw->b = SPA_POD_BUILDER_INIT(rtpw->pbuffer, sizeof(rtpw->pbuffer));
  /* a non-positive rate means the stream should follow the system rate */
  params[0] = spa_format_audio_raw_build(&rtpw->b, SPA_PARAM_EnumFormat,
					 &SPA_AUDIO_INFO_RAW_INIT
					 (.format = CS_FLOAT_FORMAT,
					  .channels = parm->nChannels,
					  .rate =
					    rtpw_offered_rate(parm->sampleRate)));
  rtpw->cbflag =
    spa_system_eventfd_create(rtpw->cloop->system, SPA_FD_CLOEXEC);
  spa_ringbuffer_init(&rtpw->ring);
  pw_stream_connect(rtpw->stream,
		    PW_DIRECTION_INPUT,
		    PW_ID_ANY,
		    PW_STREAM_FLAG_AUTOCONNECT |
		    PW_STREAM_FLAG_MAP_BUFFERS |
		    PW_STREAM_FLAG_RT_PROCESS,
		    params, 1);

  pw_thread_loop_start(rtpw->loop);
  pw_thread_loop_unlock(rtpw->loop);

  /* Wait for both the negotiated format and the graph clock. */
  if (UNLIKELY(!rtpw_wait_ready(rtpw, 2000))) {
    rtpw_report_open_error(csound, rtpw, "input");
    rtpw_close_one(csound, (void **) &rtpw, 0);
    return CSOUND_ERROR;
  }
  required_sr = rtpw->sr > 0 ? (cs_float) rtpw_offered_rate(rtpw->sr)
                             : rtpw->graphr;
  /* rtpw_wait_ready() releases the loop lock, a format callback could
     still set format_error before the stream is published. publish
     under the lock so csound never receives such a stream. */
  pw_thread_loop_lock(rtpw->loop);
  format_error = rtpw->format_error;
  valid = !format_error && rtpw_validate_rates(rtpw, &sysr);
  if (valid)
    rtpw->published = 1;
  pw_thread_loop_unlock(rtpw->loop);
  if (UNLIKELY(!valid)) {
    if (format_error)
      rtpw_report_open_error(csound, rtpw, "input");
    else
      csound->ErrorMsg(csound,
		       Str("rtpw: negotiated input rate %.2f does not match "
			   "required rate %.2f\n"), rtpw->sysr, required_sr);
    rtpw_close_one(csound, (void **) &rtpw, 0);
    return CSOUND_ERROR;
  }
  csound->GetSystemSr(csound, sysr);
  *p = (void *) rtpw;
  return OK;
}

static void rtpw_close_one(CSOUND *csound, void **data, int32_t output) {
  RTPW *rtpw = (RTPW *) *data;
  if(rtpw == NULL)
    return;

  *data = NULL;
  if(output) {
    uint32_t index;
    int32_t queued, timeout;
    queued = spa_ringbuffer_get_read_index(&rtpw->ring, &index);
    if(queued < 0) queued = 0;
    if (rtpw->sr > 0)
      timeout = ((queued + rtpw->buframes) * 1000) / (int32_t) rtpw->sr + 100;
    else
      timeout = 500;
    if(timeout > 2000) timeout = 2000;
    while(timeout-- > 0 &&
          spa_ringbuffer_get_read_index(&rtpw->ring, &index) > 0)
      csound->Sleep((size_t) 1);
  }

  pw_thread_loop_lock(rtpw->loop);
  if(output) {
    struct timespec deadline;
    int32_t err;

    rtpw->drained = 0;
    if(pw_stream_flush(rtpw->stream, true) == 0 &&
       pw_thread_loop_get_time(rtpw->loop, &deadline,
                               (int64_t) 2000 * 1000000) == 0) {
      /* State and format callbacks also signal the loop, so keep waiting
         until the drain completes or the deadline expires. */
      while(!rtpw->drained && rtpw->state != PW_STREAM_STATE_ERROR) {
        err = pw_thread_loop_timed_wait_full(rtpw->loop, &deadline);
        if (err == -EINTR)
          continue;
        if (err != 0)
          break;
      }
    }
  }
  pw_stream_destroy(rtpw->stream);
  pw_thread_loop_unlock(rtpw->loop);
  pw_thread_loop_stop(rtpw->loop);
  spa_system_close(rtpw->cloop->system, rtpw->cbflag);
  pw_thread_loop_destroy(rtpw->loop);
  csound->Free(csound, rtpw->cbuffer);
  csound->Free(csound, rtpw);
}

static void rtpw_close(CSOUND *csound) {
  rtpw_close_one(csound, csound->GetRtPlayUserData(csound), 1);
  rtpw_close_one(csound, csound->GetRtRecordUserData(csound), 0);
}




// device enumeration

#define PW_MAX_DEVS 256

enum {
  PW_DEV_OUTPUT = 1,
  PW_DEV_INPUT  = 2
};

struct pw_dev {
  char name[128];
  char description[256];
  uint32_t id;
  int32_t n_channels;
};

/* A registry global only carries the properties present when the node was
   exported. PipeWire adds format properties such as audio.channels later, so
   the channel count must be read from the bound node's info props. */
struct pw_dev_node {
  struct pw_node *node;
  struct spa_hook listener;
};

/* A host obtains the device count with list == NULL, allocates that many
   entries, then calls again to fill them. The count may be obtained on one
   thread and the fill performed on another, so the pending snapshot and the
   capacity it implies live in the instance state rather than in thread-local
   storage. The fill writes at most that many entries. Concurrent count
   passes for the same direction replace the pending snapshot; callers are
   expected to complete a count/fill pair before another count for the same
   direction starts. */
struct pw_dev_snapshots {
  pthread_mutex_t mutex;      /* protects devs[] and capacity[] */
  int32_t mutex_valid;
  int32_t destroying;
  struct pw_dev *devs[2];     /* pending count pass per direction, or NULL */
  int32_t capacity[2];        /* count last reported to the host, or -1 */
};

#define RTPW_DEVICE_SNAPSHOTS "_rtpw_device_snapshots"

struct pw_dev_query {
  struct pw_thread_loop *tloop;
  struct pw_context *context;
  struct pw_core *core;
  struct pw_registry *registry;
  struct spa_hook registry_listener;
  struct spa_hook core_listener;
  CSOUND *csound;
  int32_t flags;
  int32_t display;
  int32_t no_devs;
  int32_t truncated;
  int32_t done;
  int32_t status;
  int32_t seq;
  struct pw_dev *devs;
  struct pw_dev_node nodes[PW_MAX_DEVS];
  int32_t no_nodes;
};

/* A node can be used for output if it is a sink, for input if it is a
   source. Duplex nodes provide both. */
/* Match a class or a subtype separated by '/'; reject plain prefixes such as
   Audio/SinkFilter. */
static int32_t media_class_is_or_subclass(const char *media_class,
                                          const char *base) {
  size_t n = strlen(base);
  return !strncmp(media_class, base, n) &&
         (media_class[n] == '\0' || media_class[n] == '/');
}

static int32_t device_matches(const char *media_class, int32_t flags) {
  if (media_class == NULL)
    return 0;
  if ((flags & PW_DEV_OUTPUT) &&
      (media_class_is_or_subclass(media_class, "Audio/Sink") ||
       !strcmp(media_class, "Audio/Duplex")))
    return 1;
  if ((flags & PW_DEV_INPUT) &&
      (media_class_is_or_subclass(media_class, "Audio/Source") ||
       !strcmp(media_class, "Audio/Duplex")))
    return 1;
  return 0;
}

static void node_event_info(void *data, const struct pw_node_info *info) {
  struct pw_dev *dev = (struct pw_dev *) data;
  const char *channels;
  int32_t nch;

  if (info == NULL || info->props == NULL)
    return;
  channels = spa_dict_lookup(info->props, PW_KEY_AUDIO_CHANNELS);
  if (channels == NULL)
    return;
  nch = (int32_t) strtol(channels, NULL, 10);
  if (nch > 0)
    dev->n_channels = nch;
}

static const struct pw_node_events node_events = {
  PW_VERSION_NODE_EVENTS,
  .info = node_event_info,
};

static void add_pipewire_device(struct pw_dev_query *q, uint32_t id,
                                const struct spa_dict *props) {
  struct pw_dev *dev;
  CSOUND *csound = q->csound;
  const char *name, *description, *channels;
  int32_t nch = -1;

  if (q->no_devs >= PW_MAX_DEVS) {
    q->truncated = 1;
    return;
  }
  name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
  if (name == NULL || name[0] == '\0') {
    if (name != NULL && q->display)
      csound->Warning(csound, Str("PipeWire: skipping node with an empty "
                                  "name\n"));
    return;
  }
  if (strlen(name) > sizeof(((CS_AUDIODEVICE *) 0)->device_id) - 5) {
    if (q->display)
      csound->Warning(csound, Str("PipeWire: skipping node name too long "
                                  "to use as a device ID\n"));
    return;
  }
  description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
  if (description == NULL)
    description = spa_dict_lookup(props, PW_KEY_NODE_NICK);
  /* The global property is only a fallback; node_event_info() replaces this
     with the value from the bound node once its info arrives. */
  channels = spa_dict_lookup(props, PW_KEY_AUDIO_CHANNELS);
  if (channels != NULL)
    nch = (int32_t) strtol(channels, NULL, 10);

  dev = &(q->devs[q->no_devs]);
  dev->id = id;
  dev->n_channels = nch;
  snprintf(dev->name, sizeof(dev->name), "%s", name != NULL ? name : "");
  snprintf(dev->description, sizeof(dev->description), "%s",
           description != NULL ? description : dev->name);

  /* Bind the node so its current properties, including the channel count,
     can be read even when the registry global did not carry them. */
  if (q->no_nodes < PW_MAX_DEVS) {
    struct pw_dev_node *entry = &q->nodes[q->no_nodes];
    entry->node = (struct pw_node *) pw_registry_bind(
        q->registry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0);
    if (entry->node != NULL) {
      pw_node_add_listener(entry->node, &entry->listener, &node_events, dev);
      q->no_nodes++;
    }
  }
  q->no_devs++;
}

static void registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                  const char *type, uint32_t version,
                                  const struct spa_dict *props) {
  struct pw_dev_query *q = (struct pw_dev_query *) data;
  const char *media_class;

  IGN(permissions);
  IGN(version);
  if (props == NULL || strcmp(type, PW_TYPE_INTERFACE_Node) != 0)
    return;
  media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
  if (!device_matches(media_class, q->flags))
    return;
  add_pipewire_device(q, id, props);
}

static void registry_event_global_remove(void *data, uint32_t id) {
  IGN(data);
  IGN(id);
}

static const struct pw_registry_events registry_events = {
  PW_VERSION_REGISTRY_EVENTS,
  .global = registry_event_global,
  .global_remove = registry_event_global_remove,
};

static void core_event_done(void *data, uint32_t id, int seq)
{
  struct pw_dev_query *q = (struct pw_dev_query *) data;
  if (id != PW_ID_CORE || seq != q->seq)
    return;
  q->done = 1;
  pw_thread_loop_signal(q->tloop, false);
}

static void core_event_error(void *data, uint32_t id, int seq, int res,
                             const char *message)
{
  struct pw_dev_query *q = (struct pw_dev_query *) data;
  CSOUND *csound = q->csound;
  IGN(id);
  IGN(seq);
  IGN(res);
  csound->Message(csound, Str("PipeWire error: %s\n"),
                  message != NULL ? message : "unknown");
  q->status = -1;
  q->done = 1;
  pw_thread_loop_signal(q->tloop, false);
}

static const struct pw_core_events core_events = {
  PW_VERSION_CORE_EVENTS,
  .done = core_event_done,
  .error = core_event_error,
};

/* Round-trip to the server so that all registry events have been delivered.
   The thread loop must be locked by the caller. */
static int32_t query_sync(struct pw_dev_query *q) {
  int32_t err;
  q->done = 0;
  q->status = 0;
  q->seq = pw_core_sync(q->core, PW_ID_CORE, 0);
  if (UNLIKELY(q->seq < 0)) {
    q->status = -1;
    return -1;
  }
  while (!q->done) {
    err = pw_thread_loop_timed_wait(q->tloop, 5);
    if (err != 0 && err != -EINTR) {
      q->status = -1;
      break;
    }
  }
  return q->status;
}

/* Enumerate the audio devices of the requested direction(s). If snapshot is
   non-NULL, transfer the device snapshot to the caller. Returns the number
   of devices found, or 0 on failure. */
static int32_t query_pipewire(CSOUND *csound, int32_t flags, int32_t display,
                              struct pw_dev **snapshot) {
  struct pw_dev_query q;
  int32_t i, n = 0, status;

  if (snapshot != NULL)
    *snapshot = NULL;

  memset(&q, 0, sizeof(q));
  q.csound = csound;
  q.flags = flags;
  q.display = display;

  q.devs = (struct pw_dev *) calloc(PW_MAX_DEVS, sizeof(struct pw_dev));
  if (UNLIKELY(q.devs == NULL))
    return 0;

  q.tloop = pw_thread_loop_new("csound-query", NULL);
  if (UNLIKELY(q.tloop == NULL)) {
    csound->Message(csound, Str("Failed to create PipeWire thread loop\n"));
    goto cleanup;
  }
  q.context = pw_context_new(pw_thread_loop_get_loop(q.tloop), NULL, 0);
  if (UNLIKELY(q.context == NULL)) {
    csound->Message(csound, Str("Failed to create PipeWire context\n"));
    goto cleanup;
  }
  q.core = pw_context_connect(q.context, NULL, 0);
  if (UNLIKELY(q.core == NULL)) {
    csound->Message(csound, Str("Failed to connect to PipeWire\n"));
    goto cleanup;
  }
  pw_core_add_listener(q.core, &q.core_listener, &core_events, &q);
  q.registry = pw_core_get_registry(q.core, PW_VERSION_REGISTRY, 0);
  if (UNLIKELY(q.registry == NULL)) {
    csound->Message(csound, Str("Failed to get PipeWire registry\n"));
    goto cleanup;
  }
  pw_registry_add_listener(q.registry, &q.registry_listener,
                           &registry_events, &q);

  if (display) {
    if ((flags & PW_DEV_INPUT) && !(flags & PW_DEV_OUTPUT))
      csound->Message(csound, Str("pipewire sources:\n"));
    else
      csound->Message(csound, Str("pipewire sinks:\n"));
  }

  pw_thread_loop_start(q.tloop);
  pw_thread_loop_lock(q.tloop);
  status = query_sync(&q);
  /* The registry pass binds each matching node. A second round trip delivers
     the node info events that carry audio.channels, which the registry
     global properties generally do not include. The device list is already
     complete, so failure to collect the extra information does not discard
     it. */
  if (status >= 0 && q.no_nodes > 0)
    query_sync(&q);
  pw_thread_loop_unlock(q.tloop);
  pw_thread_loop_stop(q.tloop);

  if (UNLIKELY(status < 0)) {
    if (snapshot != NULL)   /* only report on the counting pass */
      csound->Message(csound, Str("Failed to query PipeWire devices\n"));
    goto cleanup;
  }

  if (q.truncated)
    csound->Warning(csound, Str("PipeWire: only the first %d devices are "
                                "listed\n"), PW_MAX_DEVS);
  if (display) {
    for (i = 0; i < q.no_devs; i++) {
      struct pw_dev *dev = &q.devs[i];
      csound->Message(csound, Str("  device: %d\n"), i);
      csound->Message(csound, "  ID: %u\n", dev->id);
      csound->Message(csound, Str("  name: %s\n"), dev->name);
      csound->Message(csound, Str("  description: %s\n"), dev->description);
      if (dev->n_channels > 0)
        csound->Message(csound, Str("  channels: %d\n"), dev->n_channels);
      csound->Message(csound, Str(" ----\n"));
    }
    csound->Message(csound, Str("found %d devices\n"), q.no_devs);
  }

  n = q.no_devs;
  if (snapshot != NULL && n > 0) {
    *snapshot = q.devs;
    q.devs = NULL;
  }

 cleanup:
  for (i = 0; i < q.no_nodes; i++) {
    spa_hook_remove(&q.nodes[i].listener);
    pw_proxy_destroy((struct pw_proxy *) q.nodes[i].node);
  }
  if (q.registry != NULL) {
    spa_hook_remove(&q.registry_listener);
    pw_proxy_destroy((struct pw_proxy *) q.registry);
  }
  if (q.core != NULL) {
    spa_hook_remove(&q.core_listener);
    pw_core_disconnect(q.core);
  }
  if (q.context != NULL)
    pw_context_destroy(q.context);
  if (q.tloop != NULL)
    pw_thread_loop_destroy(q.tloop);
  free(q.devs);
  return n;
}

static int32_t list_outputs(CSOUND *csound) {
  return query_pipewire(csound, PW_DEV_OUTPUT, 1, NULL);
}

static void rtpw_fill_audio_device(CS_AUDIODEVICE *dest,
                                   const struct pw_dev *src,
                                   int32_t is_output) {
  memset(dest, 0, sizeof(CS_AUDIODEVICE));
  snprintf(dest->device_name, sizeof(dest->device_name), "%.*s",
           (int) sizeof(dest->device_name) - 1, src->description);
  /* The node name can be given as -odac:<name> / -iadc:<name>. */
  snprintf(dest->device_id, sizeof(dest->device_id), "%s:%.*s",
           is_output ? "dac" : "adc",
           (int) sizeof(dest->device_id) - 5, src->name);
  snprintf(dest->rt_module, sizeof(dest->rt_module), "%s", "rtpw");
  dest->max_nchnls = src->n_channels;
  dest->isOutput = is_output;
}

/* The caller must hold state->mutex. */
static void rtpw_clear_snapshot(struct pw_dev_snapshots *state, int32_t slot) {
  free(state->devs[slot]);
  state->devs[slot] = NULL;
  state->capacity[slot] = -1;
}

static int32_t rtpw_list(CSOUND *csound, CS_AUDIODEVICE *list,
			 int32_t isOutput) {
  struct pw_dev_snapshots *state =
    (struct pw_dev_snapshots *) csound->QueryGlobalVariable(
        csound, RTPW_DEVICE_SNAPSHOTS);
  struct pw_dev *snapshot;
  struct pw_dev *devs = NULL;
  int32_t i, m, n, flags = isOutput ? PW_DEV_OUTPUT : PW_DEV_INPUT;
  int32_t slot = isOutput ? 0 : 1;

  if (UNLIKELY(state == NULL || !state->mutex_valid))
    return 0;

  if (list == NULL) {
    n = query_pipewire(csound, flags, 0, &devs);
    if (n < 0)
      n = 0;
    pthread_mutex_lock(&state->mutex);
    if (UNLIKELY(state->destroying)) {
      pthread_mutex_unlock(&state->mutex);
      free(devs);
      return 0;
    }
    rtpw_clear_snapshot(state, slot);
    state->devs[slot] = devs;
    state->capacity[slot] = n;
    pthread_mutex_unlock(&state->mutex);
    return n;
  }

  /* Any thread may perform the fill. The capacity comes from the count pass
     recorded in the instance state above; without one there is no value to
     bound the writes with. */
  pthread_mutex_lock(&state->mutex);
  if (UNLIKELY(state->destroying || state->capacity[slot] < 0)) {
    pthread_mutex_unlock(&state->mutex);
    return 0;
  }
  n = state->capacity[slot];
  snapshot = state->devs[slot];
  state->devs[slot] = NULL;
  state->capacity[slot] = -1;
  pthread_mutex_unlock(&state->mutex);

  if (snapshot != NULL) {
    for (i = 0; i < n; i++)
      rtpw_fill_audio_device(&list[i], &snapshot[i], isOutput);
    free(snapshot);
  } else if (n > 0) {
    /* The array was not stored, so enumerate again but never write past the
       count the host was given. */
    m = query_pipewire(csound, flags, 0, &devs);
    if (UNLIKELY(m <= 0 || devs == NULL)) {
      free(devs);
      for (i = 0; i < n; i++)
        memset(&list[i], 0, sizeof(list[i]));
      return 0;
    }
    if (m > n)
      m = n;
    for (i = 0; i < m; i++)
      rtpw_fill_audio_device(&list[i], &devs[i], isOutput);
    free(devs);
    /* Keep the caller's remaining entries defined if the list shrank. */
    for (i = m; i < n; i++)
      memset(&list[i], 0, sizeof(list[i]));
  }
  return n;
}


 int32_t csoundModuleCreate(CSOUND *csound)
{
  IGN(csound);
  csound->DebugMsg(csound, "created pipewire module\n");
  return 0;
}

int32_t csoundModuleInit(CSOUND *csound) {
  struct pw_dev_snapshots *snapshots;
  csound->ModuleListAdd(csound, "rtpw", "audio");
  char buf[32];
  char *s = (char*) csound->QueryGlobalVariable(csound, "_RTAUDIO");
  int i = 0;
  if (s != NULL) {
    while (*s != (char) 0 && i < 8)
      buf[i++] = *(s++) | (char) 0x20;
  }
  buf[i] = (char) 0;
  if (strcmp(&(buf[0]), "rtpw") == 0 ||
      strcmp(&(buf[0]), "pw") == 0 ||
      strcmp(&(buf[0]), "pipewire") == 0){

    snapshots = (struct pw_dev_snapshots *) csound->QueryGlobalVariable(
        csound, RTPW_DEVICE_SNAPSHOTS);
    if (snapshots == NULL) {
      if (UNLIKELY(csound->CreateGlobalVariable(
                       csound, RTPW_DEVICE_SNAPSHOTS,
                       sizeof(struct pw_dev_snapshots)) != 0)) {
        csound->ErrorMsg(csound, Str("rtpw: could not create device-list state\n"));
        return CSOUND_ERROR;
      }
      snapshots = (struct pw_dev_snapshots *) csound->QueryGlobalVariable(
          csound, RTPW_DEVICE_SNAPSHOTS);
    }
    if (UNLIKELY(snapshots == NULL)) {
      csound->ErrorMsg(csound, Str("rtpw: could not access device-list state\n"));
      return CSOUND_ERROR;
    }
    if (!snapshots->mutex_valid) {
      if (UNLIKELY(pthread_mutex_init(&snapshots->mutex, NULL) != 0)) {
        csound->ErrorMsg(csound, Str("rtpw: could not create device-list mutex\n"));
        return CSOUND_ERROR;
      }
      snapshots->mutex_valid = 1;
      snapshots->devs[0] = NULL;
      snapshots->devs[1] = NULL;
      snapshots->capacity[0] = -1;
      snapshots->capacity[1] = -1;
    }
    snapshots->destroying = 0;

    csound->DebugMsg(csound, Str("rtaudio: pipewire module enabled\n"));
    csound->SetPlayopenCallback(csound, rtpw_open_out);
    csound->SetRecopenCallback(csound, rtpw_open_in);
    csound->SetRtplayCallback(csound, rtpw_play);
    csound->SetRtrecordCallback(csound, rtpw_record);
    csound->SetRtcloseCallback(csound, rtpw_close);
    csound->SetAudioDeviceListCallback(csound, rtpw_list);
    pw_init(NULL,NULL);
  }
  return CSOUND_SUCCESS;
}

int32_t csoundModuleDestroy(CSOUND *csound){
  struct pw_dev_snapshots *snapshots =
    (struct pw_dev_snapshots *) csound->QueryGlobalVariable(
        csound, RTPW_DEVICE_SNAPSHOTS);

  if (snapshots == NULL || !snapshots->mutex_valid)
    return 0;

  /* Hosts must not run list calls concurrently with instance destruction.
     Any count/fill snapshot still pending is released here. */
  pthread_mutex_lock(&snapshots->mutex);
  snapshots->destroying = 1;
  rtpw_clear_snapshot(snapshots, 0);
  rtpw_clear_snapshot(snapshots, 1);
  pthread_mutex_unlock(&snapshots->mutex);
  return 0;
}

 int32_t csoundModuleInfo(void){
  return CSOUND_MODULE_INFO;
}
