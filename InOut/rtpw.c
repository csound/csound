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
#include <errno.h>
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
  enum pw_stream_state state;
  CSOUND *csound;
} RTPW;

/* The system rate is whatever the graph runs at, unless the stream itself
   was asked to follow the system rate, in which case the negotiated rate is
   the one Csound is actually running at. */
static cs_float rtpw_system_sr(RTPW *rtpw) {
  if (rtpw->sr > 0)
    return rtpw->graphr > 0 ? rtpw->graphr : -1.0f;
  if (rtpw->sysr > 0)
    return rtpw->sysr;
  return rtpw->sr;
}

/* Wait for format negotiation and, for fixed-rate streams, the graph clock.
   The PipeWire callbacks signal the loop when format or stream state changes. */
static int32_t rtpw_wait_ready(RTPW *rtpw, int32_t timeout_ms) {
  struct pw_time t;
  struct timespec deadline;
  int32_t need_graph = rtpw->sr > 0, ready = 0, err;

  if (pw_thread_loop_get_time(rtpw->loop, &deadline,
                              (int64_t) timeout_ms * 1000000) < 0)
    return 0;

  pw_thread_loop_lock(rtpw->loop);
  while (!ready && rtpw->state != PW_STREAM_STATE_ERROR) {
    if (rtpw->sysr > 0) {
      if (!need_graph)
        ready = 1;
      else if (rtpw->state == PW_STREAM_STATE_STREAMING &&
               pw_stream_get_time_n(rtpw->stream, &t, sizeof(t)) >= 0 &&
               t.rate.num != 0 && t.rate.denom != 0) {
        rtpw->graphr = (cs_float) t.rate.denom / (cs_float) t.rate.num;
        ready = 1;
      }
    }
    if (ready || rtpw->state == PW_STREAM_STATE_ERROR)
      break;
    err = pw_thread_loop_timed_wait_full(rtpw->loop, &deadline);
    if (err != 0)
      break;
  }
  pw_thread_loop_unlock(rtpw->loop);
  return ready;
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
  pw_thread_loop_signal(rtpw->loop, false);
}

static void rtpw_play(CSOUND *csound, const cs_float *outbuf, int32_t nbytes){
  RTPW *rtpw = (RTPW *) *csound->GetRtPlayUserData(csound);
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
  cs_float sysr;

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
					  .rate = parm->sampleRate > 0 ?
					          (uint32_t) (parm->sampleRate +
							      0.5) : 0));
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

  /* Wait for the format and, for fixed-rate streams, the graph clock. */
  if (UNLIKELY(!rtpw_wait_ready(rtpw, 2000))) {
    csound->ErrorMsg(csound, Str("rtpw: could not determine the output "
				 "stream sampling rate\n"));
    rtpw_close_one(csound, (void **) &rtpw, 1);
    return CSOUND_ERROR;
  }
  sysr = rtpw_system_sr(rtpw);
  if (sysr > 0) {
    if (rtpw->sr <= 0)      /* stream followed the system rate */
      rtpw->sr = sysr;
    csound->GetSystemSr(csound, sysr);
  } else {
    csound->Warning(csound, Str("rtpw: could not get the system "
				"sampling rate\n"));
  }
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
    // update channel count and realloc buffer
    rtpw->nchnls = rtpw->format.info.raw.channels;
    rtpw->cbuffer = csound->ReAlloc(csound,rtpw->cbuffer,sizeof(cs_float)*rtpw->buframes*
				    rtpw->nchnls);
    csound->DebugMsg(csound, Str("pw - reallocated hw buffer: %d\n"),
		     rtpw->buframes);
  }

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
  cs_float sysr;

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
					  .rate = parm->sampleRate > 0 ?
					          (uint32_t) (parm->sampleRate +
							      0.5) : 0));
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

  /* Wait for the format and, for fixed-rate streams, the graph clock. */
  if (UNLIKELY(!rtpw_wait_ready(rtpw, 2000))) {
    csound->ErrorMsg(csound, Str("rtpw: could not determine the input "
				 "stream sampling rate\n"));
    rtpw_close_one(csound, (void **) &rtpw, 0);
    return CSOUND_ERROR;
  }
  sysr = rtpw_system_sr(rtpw);
  if (sysr > 0) {
    if (rtpw->sr <= 0)      /* stream followed the system rate */
      rtpw->sr = sysr;
    csound->GetSystemSr(csound, sysr);
  } else {
    csound->Warning(csound, Str("rtpw: could not get the system "
				"sampling rate\n"));
  }
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
    rtpw->drained = 0;
    if(pw_stream_flush(rtpw->stream, true) == 0 && !rtpw->drained)
      pw_thread_loop_timed_wait(rtpw->loop, 2);
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
  int32_t n_channels;
};

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
};

/* A node can be used for output if it is a sink, for input if it is a
   source. Duplex nodes provide both. */
static int32_t device_matches(const char *media_class, int32_t flags) {
  if (media_class == NULL)
    return 0;
  if ((flags & PW_DEV_OUTPUT) &&
      (!strncmp(media_class, "Audio/Sink", 10) ||
       !strcmp(media_class, "Audio/Duplex")))
    return 1;
  if ((flags & PW_DEV_INPUT) &&
      (!strncmp(media_class, "Audio/Source", 12) ||
       !strcmp(media_class, "Audio/Duplex")))
    return 1;
  return 0;
}

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
  description = spa_dict_lookup(props, PW_KEY_NODE_DESCRIPTION);
  if (description == NULL)
    description = spa_dict_lookup(props, PW_KEY_NODE_NICK);
  channels = spa_dict_lookup(props, PW_KEY_AUDIO_CHANNELS);
  if (channels != NULL)
    nch = (int32_t) strtol(channels, NULL, 10);

  dev = &(q->devs[q->no_devs]);
  dev->n_channels = nch;
  snprintf(dev->name, sizeof(dev->name), "%s", name != NULL ? name : "");
  snprintf(dev->description, sizeof(dev->description), "%s",
           description != NULL ? description : dev->name);

  if (q->display) {
    csound->Message(csound, Str("  device: %d\n"), q->no_devs);
    csound->Message(csound, "  ID: %u\n", id);
    csound->Message(csound, Str("  name: %s\n"), dev->name);
    csound->Message(csound, Str("  description: %s\n"), dev->description);
    if (nch > 0)
      csound->Message(csound, Str("  channels: %d\n"), nch);
    csound->Message(csound, Str(" ----\n"));
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

/* Enumerate the audio devices of the requested direction(s). If list is
   non-NULL it is filled with one CS_AUDIODEVICE per device. Returns the
   number of devices found, or 0 on failure. */
static int32_t query_pipewire(CSOUND *csound, int32_t flags, int32_t display,
                              CS_AUDIODEVICE *list) {
  struct pw_dev_query q;
  int32_t i, n = 0, status, is_output = (flags & PW_DEV_OUTPUT) != 0;

  memset(&q, 0, sizeof(q));
  q.csound = csound;
  q.flags = flags;
  q.display = display;

  q.devs = (struct pw_dev *) csound->Calloc(csound,
                                   PW_MAX_DEVS * sizeof(struct pw_dev));
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
  pw_thread_loop_unlock(q.tloop);
  pw_thread_loop_stop(q.tloop);

  if (UNLIKELY(status < 0)) {
    if (list == NULL)   /* only report on the counting pass */
      csound->Message(csound, Str("Failed to query PipeWire devices\n"));
    goto cleanup;
  }

  if (q.truncated)
    csound->Warning(csound, Str("PipeWire: only the first %d devices are "
                                "listed\n"), PW_MAX_DEVS);
  if (display)
    csound->Message(csound, Str("found %d devices\n"), q.no_devs);

  n = q.no_devs;
  if (list != NULL) {
    for (i = 0; i < n; i++) {
      CS_AUDIODEVICE *dev = &(list[i]);
      memset(dev, 0, sizeof(CS_AUDIODEVICE));
      snprintf(dev->device_name, sizeof(dev->device_name), "%.*s",
               (int) sizeof(dev->device_name) - 1, q.devs[i].description);
      /* the node name can be given as -odac:<name> / -iadc:<name> */
      snprintf(dev->device_id, sizeof(dev->device_id), "%s:%.*s",
               is_output ? "dac" : "adc",
               (int) sizeof(dev->device_id) - 5, q.devs[i].name);
      snprintf(dev->rt_module, sizeof(dev->rt_module), "%s", "rtpw");
      dev->max_nchnls = q.devs[i].n_channels;
      dev->isOutput = is_output;
    }
  }

 cleanup:
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
  csound->Free(csound, q.devs);
  return n;
}

static int32_t list_outputs(CSOUND *csound) {
  return query_pipewire(csound, PW_DEV_OUTPUT, 1, NULL);
}

static int32_t rtpw_list(CSOUND *csound, CS_AUDIODEVICE *list,
			 int32_t isOutput){
  return query_pipewire(csound, isOutput ? PW_DEV_OUTPUT : PW_DEV_INPUT,
                        0, list);
}




 int32_t csoundModuleCreate(CSOUND *csound)
{
  IGN(csound);
  csound->DebugMsg(csound, "created pipewire module\n");
  return 0;
}

 int32_t csoundModuleInit(CSOUND *csound) {
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
  return 0;
}

 int32_t csoundModuleInfo(void){
  return CSOUND_MODULE_INFO;
}
