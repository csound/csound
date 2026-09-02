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
#include "csdl.h"

#ifdef USE_DOUBLE
#define MYFLT_FORMAT SPA_AUDIO_FORMAT_F64
#else
#define MYFLT_FORMAT SPA_AUDIO_FORMAT_F32
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
  MYFLT sr, sysr;
  int32_t buframes;
  int32_t cbflag;
  int32_t drained;
  CSOUND *csound;
} RTPW;


static void rtpw_out_callback(void *p) {
  RTPW *rtpw = (RTPW *) p;
  struct pw_buffer *pwbuf;
  struct spa_buffer *spabuf;
  uint8_t *bufp;
  uint32_t i, rem, sil;
  int32_t n, frames, fbytes = (int32_t) (rtpw->nchnls*sizeof(MYFLT));

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
    rtpw->csound->Warning(rtpw->csound, "%d dropped output frames", sil);
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

static void rtpw_play(CSOUND *csound, const MYFLT *outbuf, int32_t nbytes){
  RTPW *rtpw = (RTPW *) *csound->GetRtPlayUserData(csound);
  int32_t nframes = nbytes/(sizeof(MYFLT)*rtpw->nchnls);
  int32_t rem, fbytes = sizeof(MYFLT)*rtpw->nchnls, n;
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

static const struct pw_stream_events stream_events_out = {
  PW_VERSION_STREAM_EVENTS,
  .process = rtpw_out_callback,
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

  list_outputs(csound);
  p = (void**) csound->GetRtPlayUserData(csound);
  if(*p != NULL) return 0;
  rtpw = (RTPW *) csound->Calloc(csound, sizeof(RTPW));
  rtpw->cbuffer = csound->Calloc(csound,sizeof(MYFLT)*parm->bufSamp_HW*
				 parm->nChannels);
  rtpw->nchnls = parm->nChannels;
  rtpw->sr = parm->sampleRate;
  rtpw->buframes = parm->bufSamp_HW;
  csound->Message(csound, "hw frames: %d sw frames: %d\n", rtpw->buframes,
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
  params[0] = spa_format_audio_raw_build(&rtpw->b, SPA_PARAM_EnumFormat,
					 &SPA_AUDIO_INFO_RAW_INIT
					 (.format = MYFLT_FORMAT,
					  .channels = parm->nChannels,
					  .rate = parm->sampleRate));
  rtpw->sr = parm->sampleRate;
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
  rtpw->csound = csound;
  *p = (void *) rtpw;
  pw_thread_loop_unlock(rtpw->loop);
  // system sr is the same as sr for output only
  if(csound->GetSystemSr(csound, 0) == -1.)
    csound->GetSystemSr(csound, rtpw->sr);
  return CSOUND_SUCCESS;
}

static void rtpw_in_callback(void *p) {
  RTPW *rtpw = (RTPW *) p;
  struct pw_buffer *pwbuf;
  struct spa_buffer *spabuf;
  uint8_t *bufp;
  uint32_t i;
  int32_t rem, n, frames, fbytes = (int32_t) (rtpw->nchnls*sizeof(MYFLT));

  if ((pwbuf = pw_stream_dequeue_buffer(rtpw->stream)) == NULL) {
    pw_log_warn("out of buffers: %m");
    return;
  }

  spabuf = pwbuf->buffer;
  if ((bufp = spabuf->datas[0].data) == NULL) return;
  frames = spabuf->datas[0].chunk->size / sizeof(MYFLT);

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


static int32_t rtpw_record(CSOUND *csound, MYFLT *inbuf, int32_t nbytes) {
  RTPW *rtpw = (RTPW *) *csound->GetRtRecordUserData(csound);
  uint32_t i, rem, nframes;
  int32_t n, fbytes = rtpw->nchnls*sizeof(MYFLT);
  int32_t wbytes = 0;
  nframes = (uint32_t) (nbytes/(csound->GetNchnls_i(csound)*sizeof(MYFLT)));


  if(csound->GetNchnls_i(csound) == (uint32_t) rtpw->nchnls) {
    while(nframes > 0) {
      n = spa_ringbuffer_get_read_index(&rtpw->ring, &i);
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
  } else { // sample by sample
    MYFLT samp;
    int32_t j, k, nchnls = csound->GetNchnls_i(csound);
    int32_t samps = (int32_t) nframes*nchnls;
    for(j = 0; j < samps; j+=nchnls){
      for(k = 0; k < nchnls; k++) {
	if(k < rtpw->nchnls) {
	  n = spa_ringbuffer_get_read_index(&rtpw->ring, &i);
	  if(n > 0) {
	    spa_ringbuffer_read_data(&rtpw->ring, rtpw->cbuffer,
				     rtpw->buframes * fbytes,
				     (i % rtpw->buframes) * sizeof(MYFLT),
				     &samp, sizeof(MYFLT));
	    spa_ringbuffer_read_update(&rtpw->ring, i + 1);
	  } else samp = FL(0.0);
	} else samp = FL(0.0);
	inbuf[j + k] = samp;
	wbytes += sizeof(MYFLT);
      }
    }
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
  rtpw->sysr = (MYFLT) rtpw->format.info.raw.rate;

  if(rtpw->sysr != rtpw->sr && rtpw->sr != -1.0)
    csound->Warning(csound, "rtpw: mismatched input sampling rate,\n"
		    "system-sr %.1f csound-sr %.1f", rtpw->sysr,
		    rtpw->sr);

  if(rtpw->nchnls != (int32_t) rtpw->format.info.raw.channels) {
    // update channel count and realloc buffer
    rtpw->nchnls = rtpw->format.info.raw.channels;
    rtpw->cbuffer = csound->ReAlloc(csound,rtpw->cbuffer,sizeof(MYFLT)*rtpw->buframes*
				    rtpw->nchnls);
    csound->Message(csound, "pw - reallocated hw buffer: %d\n", rtpw->buframes);
  }

}

static const struct pw_stream_events stream_events_in = {
  PW_VERSION_STREAM_EVENTS,
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

  p = (void**) csound->GetRtRecordUserData(csound);
  if(*p != NULL) return 0;
  rtpw = (RTPW *) csound->Calloc(csound, sizeof(RTPW));
  rtpw->cbuffer = csound->Calloc(csound,sizeof(MYFLT)*parm->bufSamp_HW*
				 parm->nChannels);
  rtpw->nchnls = parm->nChannels;
  rtpw->sr = parm->sampleRate;
  rtpw->sysr = -1.0;
  rtpw->buframes = parm->bufSamp_HW;
  csound->Message(csound, "pwin - hw frames: %d sw frames: %d\n", rtpw->buframes,
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
  params[0] = spa_format_audio_raw_build(&rtpw->b, SPA_PARAM_EnumFormat,
					 &SPA_AUDIO_INFO_RAW_INIT
					 (.format = MYFLT_FORMAT));
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
  spa_format_audio_raw_parse(params[0], &rtpw->format.info.raw);

  rtpw->csound = csound;
  *p = (void *) rtpw;
  pw_thread_loop_unlock(rtpw->loop);
  // wait for params callback to set sr
  while(rtpw->sysr < 0)
    ;
  // set the system sr
  csound->GetSystemSr(csound, rtpw->sysr);
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
    timeout = ((queued + rtpw->buframes) * 1000) / (int32_t) rtpw->sr + 100;
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




// list outputs
struct sink_info {
  uint32_t id;
  char name[256];
  char description[512];
  uint32_t n_channels;
  bool is_default;
};


struct sink_data {
  struct pw_loop *loop;
  struct pw_thread_loop *tloop;
  struct pw_context *context;
  struct pw_core *core;
  struct pw_registry *registry;
  struct spa_hook registry_listener;
  CSOUND *csound;
  int no_devs;
  int display;
  struct sink_info sinks[256];
  int done;
};

static void registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                  const char *type, uint32_t version,
                                  const struct spa_dict *props) {
  struct sink_data *sink_data = data;
  const char *media_class;
  const char *node_name;
  const char *node_description;
  CSOUND *csound = sink_data->csound;
  struct sink_info *sinks = sink_data->sinks;
  int32_t display = sink_data->display;

  // Check if this is an audio sink node
  if (strcmp(type, PW_TYPE_INTERFACE_Node) == 0) {
    if (props) {
      media_class = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
      if (media_class && strcmp(media_class, "Audio/Sink") == 0) {
	if(sink_data->no_devs < 256) {
	  int32_t n = sink_data->no_devs;
	  node_name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
	  node_description = spa_dict_lookup(props,
					     PW_KEY_NODE_DESCRIPTION);
	  if(display){
	    csound->Message(csound, "  sink node: %d\n", n);
	    csound->Message(csound, "  ID: %u\n", id);
	    csound->Message(csound,"  name: %s\n",
			    node_name ? node_name : "Unknown");
	    csound->Message(csound,"  description: %s\n",
			    node_description ? node_description :
			    "No description");
	    csound->Message(csound,"  media class: %s\n ----\n",
			    media_class);
	  }
	  sinks[n].n_channels = 2;
	  sinks[n].id = id;
	  strncpy(sinks[n].name, node_name, 256);
	  strncpy(sinks[n].description, node_description, 512);
	  sink_data->no_devs++;
	}
      }
    }
  }
}

static void registry_event_global_remove(void *data, uint32_t id) {
}

static const struct pw_registry_events registry_events = {
  PW_VERSION_REGISTRY_EVENTS,
  .global = registry_event_global,
  .global_remove = registry_event_global_remove,
};

static void core_event_done(void *data, uint32_t id, int seq)
{
  struct sink_data *sink_data = data;
  sink_data->done = 1;
  pw_thread_loop_stop(sink_data->tloop);
}

static void core_event_error(void *data, uint32_t id, int seq, int res, const char *message)
{
  struct sink_data *sink_data = data;
  sink_data->csound->Message(sink_data->csound, "Core error: %s\n", message);
  sink_data->done = 1;
  pw_thread_loop_stop(sink_data->tloop);
}

static const struct pw_core_events core_events = {
  PW_VERSION_CORE_EVENTS,
  .done = core_event_done,
  .error = core_event_error,
};

int32_t query_pipewire_sinks(CSOUND *csound, CS_AUDIODEVICE *list, int32_t display)
{
  struct sink_data data = {0};
  struct spa_hook core_listener;
  data.csound = csound;
  data.display = display;
  data.no_devs = 0;

  data.tloop = pw_thread_loop_new("sink-query", NULL);
  data.loop = pw_thread_loop_get_loop(data.tloop);

  // Create main loop
  if (!data.loop) {
    csound->Message(csound, "Failed to create main loop\n");
    goto cleanup;
  }

  // Create context
  data.context = pw_context_new(pw_thread_loop_get_loop(data.tloop), NULL, 0);
  if (!data.context) {
    csound->Message(csound, "Failed to create context\n");
    goto cleanup;
  }

  // Connect to PipeWire daemon
  data.core = pw_context_connect(data.context, NULL, 0);
  if (!data.core) {
    csound->Message(csound, "Failed to connect to PipeWire\n");
    goto cleanup;
  }

  // Add core event listener
  pw_core_add_listener(data.core, &core_listener, &core_events, &data);

  // Get registry
  data.registry = pw_core_get_registry(data.core, PW_VERSION_REGISTRY, 0);
  if (!data.registry) {
    csound->Message(csound, "Failed to get registry\n");
    goto cleanup;
  }

  // Add registry event listener
  pw_registry_add_listener(data.registry, &data.registry_listener, &registry_events, &data);

  if(display)
   csound->Message(csound, "pipewire sinks:\n");
  pw_thread_loop_start(data.tloop);
  usleep(100000);
  if(display)
    csound->Message(csound, "found %d sinks\n", data.no_devs);

 cleanup:
  pw_thread_loop_stop(data.tloop);
  // Clean up resources
  if (data.registry) {
    spa_hook_remove(&data.registry_listener);
    pw_proxy_destroy((struct pw_proxy*)data.registry);
  }
  if (data.core) {
    spa_hook_remove(&core_listener);
    pw_core_disconnect(data.core);
  }
  if (data.context)
    pw_context_destroy(data.context);
  pw_thread_loop_destroy(data.tloop);

  if(list) {
    for(int i = 0; i < data.no_devs; i++) {
      strncpy(list[i].device_name, data.sinks[i].name, 63);
      snprintf(list[i].device_id, 63, "%s%d",
	       "dac" , data.sinks[i].id);
      list[i].max_nchnls = data.sinks[i].n_channels;
      list[i].isOutput = 1;
    }
  }
  return data.no_devs;
}

static int32_t list_outputs(CSOUND *csound) {
  return query_pipewire_sinks(csound, NULL, 1);
}

static int32_t rtpw_list(CSOUND *csound, CS_AUDIODEVICE *list,
			 int32_t isOutput){
  if(isOutput)
    return query_pipewire_sinks(csound, list, 0);
  else return 0;
}




 int32_t csoundModuleCreate(CSOUND *csound)
{
  IGN(csound);
  csound->DebugMsg(csound, "created pipewire module\n");
  return 0;
}

 int32_t csoundModuleInit(CSOUND *csound) {
  const OPARMS *O = csound->GetOParms(csound);
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
  return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t) sizeof(MYFLT));
}
