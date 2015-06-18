/*
 * Csound pnacl interactive frontend
 *
 * Copyright (C) 2013 V Lazzarini
 *
 * This file belongs to Csound.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>
#include "ppapi/cpp/audio.h"
#include "ppapi/cpp/audio_buffer.h"
#include "ppapi/cpp/media_stream_audio_track.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/url_loader.h"
#include "ppapi/cpp/url_request_info.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include <sys/mount.h>
#include <errno.h>
#include "nacl_io/nacl_io.h"
#include <stdio.h>
#include <time.h>
#include <csound.h>

typedef unsigned char Byte;

namespace {
  const char* const kPlaySoundId = "playCsound";
  const char* const kStopSoundId = "pauseCsound";
  const char* const kOrchestraId = "orchestra";
  const char* const kScoreId = "score";
  const char* const kEventId = "event";
  const char* const kChannelId = "channel";
  const char* const kSChannelId = "schannel";
  const char* const kChannelOutId = "outchannel";
  const char* const kCopyId = "copyToLocal";
  const char* const kCopyUrlId = "copyUrlToLocal";
  const char* const kGetFileId = "getFile";
  const char* const kGetTableId = "getTable";
  const char* const kSetTableId = "setTable";
  const char* const kCsdId = "csd";
  const char* const kRenderId = "render";
  const char* const kMIDIId = "midi";
  static const char kMessageArgumentSeparator = ':';
  static const char kUrlArgumentSeparator = '#';

  const double kDefaultFrequency = 440.0;
  const double kPi = 3.141592653589;
  const double kTwoPi = 2.0 * kPi;
  const uint32_t kSampleFrameCount = 128u;
  const uint32_t kChannels = 2u;
}  // namespace

/* MIDI message queue size */
#define DSIZE 4096

/* MIDI data struct */
struct MIDIdata {
  Byte status;
  Byte data1;
  Byte data2;
  Byte flag;
};

/* user data for MIDI callbacks */
struct cdata {
  MIDIdata *mdata;
  int p; int q;
};

/* csound MIDI input open callback, sets MIDI input queues etc */
static int MidiInDeviceOpen(CSOUND *csound, void **userData, const char *dev)
{
  int k;
  cdata **gdata;
  if(csoundCreateGlobalVariable(csound, "_MIDI_DATA", sizeof(cdata *)) == 0) 
    gdata = (cdata **) csoundQueryGlobalVariable(csound, "_MIDI_DATA");
  else return -1;

  MIDIdata *mdata = (MIDIdata *) malloc(DSIZE*sizeof(MIDIdata));
  cdata *refcon = (cdata *) malloc(sizeof(cdata));
  memset(mdata, 0, sizeof(MIDIdata)*DSIZE);
  refcon->mdata = mdata;
  refcon->p = 0;
  refcon->q = 0;
  *userData = (void*) refcon;
  *gdata = refcon;

  /* report success */
  return 0;
}

/* csound MIDI input close callback */
static int MidiInDeviceClose(CSOUND *csound, void *userData)
{
  cdata * data = (cdata *)userData;
  if (data != NULL) {
    free(data->mdata);
    free(data);
    csoundDestroyGlobalVariable(csound, "_MIDI_DATA");
  }
  return 0;
}

/* used to distinguish between 1 and 2-byte messages */
static  const   int     datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0 };

/* csound MIDI read callback, called every k-cycle */
static int MidiDataRead(CSOUND *csound, void *userData,
			unsigned char *mbuf, int nbytes)
{
  cdata *data = (cdata *)userData;
  MIDIdata *mdata = data->mdata;
  int *q = &data->q, st, d1, d2, n = 0;
  

  /* check if there is new data in circular queue */
  while (mdata[*q].flag) {
    st = (int) mdata[*q].status;
    d1 = (int) mdata[*q].data1;
    d2 = (int) mdata[*q].data2;
    /* csoundMessage(csound, "read MIDI data: %d %d %d \n", st,d1,d2);*/
    if (st < 0x80) goto next;
    if (st >= 0xF0 &&
	!(st == 0xF8 || st == 0xFA || st == 0xFB ||
	  st == 0xFC || st == 0xFF)) goto next;

    nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
    if (nbytes < 0) break;

    /* write to csound midi buffer */
    n += (datbyts[(st - 0x80) >> 4] + 1);
    switch (datbyts[(st - 0x80) >> 4]) {
    case 0:
      *mbuf++ = (unsigned char) st;
      break;
    case 1:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      break;
    case 2:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      *mbuf++ = (unsigned char) d2;
      break;
    }
    /* mark as read */
  next:
    mdata[*q].flag = 0;
    (*q)++;
    if (*q==DSIZE) *q = 0;

  }
  /* return the number of bytes read */
  return n;
}



struct UrlReader {

  std::string  mem;
  char buffer[32768]; 
  int  bytes;
  std::string url;
  pp::Instance  *instance;
  pp::URLLoader url_loader;
  pp::URLRequestInfo url_request;
  pp::CompletionCallbackFactory<UrlReader> cc_factory;

  static UrlReader *Create(pp::Instance *inst, char *path);
  UrlReader(pp::Instance *inst, char *path);
  void Start();  
  void ReadBody();
  void OnRead(int32_t result);
  void OnOpenUrl(int32_t result);
  UrlReader(const UrlReader&);
  void operator=(const UrlReader&);
  
};

class CsoundInstance : public pp::Instance {
 
  pp::Audio dac; 
  CSOUND *csound;
  int count;
  PPB_GetInterface get_browser_interface_;
  int fileResult;
  char *from;
  char *dest;
  char *csd;
  bool compiled;
  bool finished;
  bool is_running;
  int in_channels;
  int in_samples;
  bool input_is_on;
  pp::CompletionCallbackFactory<CsoundInstance> audio_callback_factory;
  pp::MediaStreamAudioTrack audio_input;
  void  *circularBuffer;
  short *input_buffer;

  void PlayCsound();
  void PlayCsd(char *c, bool dac);
  void CopyFileToLocalAsync(char *from, char *to);
  void CopyFromURLToLocalAsync(char *URL, char *name);
  void GetFileFromLocalAsync(char *src); 


  void OnGetInput(int32_t result, pp::AudioBuffer buffer) {
    if (result != PP_OK){
      input_is_on = false;
      PostMessage("Csound: audio input error...\n");
      return;
    }
     
    const char* data = static_cast<const char*>(buffer.GetDataBuffer());
    in_channels = buffer.GetNumberOfChannels();
    in_samples = buffer.GetNumberOfSamples();

    // push into circular buffer
    if(is_running){
      csoundWriteCircularBuffer(csound,circularBuffer,data,in_samples);
      //PostMessage("Csound: audio input callback...\n");
      if(!input_is_on)csoundMessage(csound, "input sr: %d \n", buffer.GetSampleRate());
      input_is_on = true;
    }
    // recycle buffer and schedule next one
    audio_input.RecycleBuffer(buffer);
    audio_input.GetBuffer(audio_callback_factory.NewCallbackWithOutput(
								       &CsoundInstance::OnGetInput));

  }
  
  static void CsoundCallback(void* samples,
			     uint32_t buffer_size,
			     void* data) {
    CsoundInstance* instance = (CsoundInstance*) data;
    CSOUND *csound_ = instance->csound;
    if(csound_ != NULL && !instance->isFinished()) {
      int count_ = instance->count;
      int i,j,n, buffsamps = buffer_size / sizeof(short);
      short* buff = (short*) samples;
      MYFLT _0dbfs = csoundGet0dBFS(csound_);
      MYFLT *spout = csoundGetSpout(csound_); 
      MYFLT *spin = csoundGetSpin(csound_); 
      int ksmps = csoundGetKsmps(csound_)*csoundGetNchnls(csound_);
      short *buf = instance->input_buffer;
      int in_chans = instance->in_channels;
      while(csoundGetMessageCnt(csound_)){
	instance->PostMessage(csoundGetFirstMessage(csound_));
	csoundPopFirstMessage(csound_);
      }
      instance->is_running = true;    
      MYFLT scale = 32768./_0dbfs;
      if(spout != NULL) 
	for(n=0; n < buffsamps; n++) {
	  if(count_ == 0) {
	    // get data from circular buffer into spin
	    if(instance->input_is_on){
	      //instance->PostMessage("Csound: audio input read...\n");
	       csoundReadCircularBuffer(csound_,instance->circularBuffer,buf,
					in_chans*ksmps/csoundGetNchnls(csound_));
	      if(spin != NULL)
	      for(i=j=0; i < ksmps; i+=2, j+=in_chans){
		spin[i] = buf[j]/scale;
		if(in_chans > 1)
		 spin[i+1] = buf[j+1]/scale;
		 }
	    }
	    int ret = csoundPerformKsmps(csound_);
	    if(ret != 0) {
	      instance->isFinished(true);
	      return;
	    }
	    count_ = ksmps;
	  }
	  buff[n] = (int16_t) (scale*spout[ksmps-count_]);
	  count_--;
	}
      instance->count = count_;
    } else
      memset(samples,0,buffer_size);
  }

  /* sets the MIDI callbacks */
  void MidiInit(CSOUND *csound) {
    csoundSetExternalMidiInOpenCallback(csound, MidiInDeviceOpen);
    csoundSetExternalMidiReadCallback(csound, MidiDataRead);
    csoundSetExternalMidiInCloseCallback(csound, MidiInDeviceClose);
    PostMessage("MIDI input initialized \n");
  }

  /* puts MIDI data in the circular queue */
void PutMidiData(CSOUND *csound, Byte *mididata){
  cdata *data = (cdata *) *((cdata **)csoundQueryGlobalVariable(csound, "_MIDI_DATA"));
  int *p = &data->p;
  MIDIdata *mdata = data->mdata;
  memcpy(&mdata[*p], mididata, 3);
  mdata[*p].flag = 1;
  (*p)++;
  if (*p == DSIZE) *p = 0;
  /* PostMessage("MIDI data in: ");
  char mess[1024];
  sprintf(mess, "%d %d %d\n",mididata[0], mididata[1], mididata[2]);
  PostMessage(mess); */
}

public:
  char *GetDestFileName(){ return dest; }
  void SetFileResult(int res){ fileResult = res; }
  char *GetSrcFileName(){ return from; }
  bool isCompiled() { return compiled; }
  void isCompiled(bool c) { compiled = c; }
  bool isFinished() { return finished; }
  void isFinished(bool f) { finished = f; }
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void HandleMessage(const pp::Var& var_message);
  CSOUND *GetCsound() { return csound; }
  char *GetCsd() { return csd;}
  bool StartDAC();

  explicit CsoundInstance(PP_Instance instance, 
			  PPB_GetInterface get_browser_interface)
    : pp::Instance(instance),
      csound(NULL), count(0), fileResult(0), 
      from(NULL), dest(NULL), csd(NULL), compiled(false), finished(false),
      is_running(false), input_is_on(false),
      audio_callback_factory(this)
        
  {
    get_browser_interface_ = get_browser_interface;
  }

  virtual ~CsoundInstance() {
    if(dest) free(dest); 
    if(from) free(from);
    if(csound){
      delete[] input_buffer;
      csoundDestroyMessageBuffer(csound);
      csoundDestroy(csound);
    }
  }

 
};

bool CsoundInstance::Init(uint32_t argc,
			  const char* argn[],
			  const char* argv[]) {

  csound = csoundCreate(NULL);
  MidiInit(csound);
  csoundCreateMessageBuffer(csound, 0);  
  nacl_io_init_ppapi(pp_instance(),get_browser_interface_);

  // this is to prevent a segfault with Csound
  umount("/");
  mount("", "/", "memfs", 0, "");

  mount("",                                      /* source */
        "/local",                                 /* target */
        "html5fs",                                /* filesystemtype */
        0,                                        /* mountflags */
        "type=TEMPORARY"); /* data */

  mount("",       /* source. Use relative URL */
        "/http",  /* target */
        "httpfs", /* filesystemtype */
        0,        /* mountflags */
        "");      /* data */
  return true;
}

bool CsoundInstance::StartDAC(){
  MYFLT sr = csoundGetSr(csound);
  PP_AudioSampleRate isr;
  if(sr == 44100.) isr = PP_AUDIOSAMPLERATE_44100;
  else if (sr == 48000.) isr = PP_AUDIOSAMPLERATE_48000;
  else {
    PostMessage("Csound error: unsupported SR \n");
    return false;
  }
  if(csoundGetNchnls(csound) != 2) {
    PostMessage("Csound error: only stereo output is supported \n");
    return false;
  }

  int frames = 
  pp::AudioConfig::RecommendSampleFrameCount(this,
  				       isr, kSampleFrameCount);
  dac = pp::Audio(
		  this,
		  pp::AudioConfig(this, isr, frames),
		  CsoundCallback,
		  this);
  int cbufsiz = frames*csoundGetNchnls(csound)*4;
  circularBuffer = csoundCreateCircularBuffer(csound, cbufsiz, sizeof(short));
  // FIXME: this assumes input will be at max 2 channels
  input_buffer = new short[csoundGetKsmps(csound)*2];
  char mess[64];
  sprintf(mess, "buffsize: %d\n",frames);
  PostMessage(mess);
  dac.StartPlayback();
  return true;
}

void* compileThreadFunc(void *data) {
  CsoundInstance *p = (CsoundInstance *) data;
  CSOUND *csound = p->GetCsound();
  char *csd =  p->GetCsd();
  char *argv[] = {(char *)"csound", p->GetCsd(), (char *)"-+rtaudio=null"}; 
  MYFLT sr = 0.0;
  if(csoundCompile(csound,3,argv) == 0){
    p->PostMessage("Compiled");
    if(p->StartDAC())
      p->isCompiled(true);
    else {
      free(csd);
      return NULL;
    }
  }
  free(csd);
  return NULL;
}

void* compileThreadFuncNoDAC(void *data) {
  CsoundInstance *p = (CsoundInstance *) data;
  CSOUND *csound = p->GetCsound();
  char *csd =  p->GetCsd();
  char *argv[] = {(char *)"csound", p->GetCsd()}; 
  MYFLT sr = 0.0;
  int ret = csoundCompile(csound,2,argv);
  while(csoundGetMessageCnt(csound)){
    p->PostMessage(csoundGetFirstMessage(csound));
    csoundPopFirstMessage(csound);
  } 
  if(ret == 0) {
    p->isCompiled(true);
    while(csoundPerformKsmps(csound) == 0) {
      while(csoundGetMessageCnt(csound)){
	p->PostMessage(csoundGetFirstMessage(csound));
	csoundPopFirstMessage(csound);
      }  
    }
    csoundCleanup(csound); 
    while(csoundGetMessageCnt(csound)){
      p->PostMessage(csoundGetFirstMessage(csound));
      csoundPopFirstMessage(csound);
    }
    p->PostMessage("finished render"); 
  }    
  free(csd);
  return NULL;
}


void CsoundInstance::PlayCsd(char *c, bool dac) {
  if(!compiled){
    csd = strdup(c);
    pthread_t t;
    if(dac)
      pthread_create(&t, NULL, compileThreadFunc, this);
    else
      pthread_create(&t, NULL, compileThreadFuncNoDAC, this);
  } else 
    PostMessage("Csound is already started \n"
		"Refresh page to play a different CSD\n");
}



void CsoundInstance::PlayCsound() {
  if(!compiled) {
    csoundSetHostImplementedAudioIO(csound,1,0);
    csoundSetOption(csound, (char *) "-odac");
    csoundSetOption(csound, (char *) "-iadc");
    csoundSetOption(csound, (char *) "-M0");
    csoundSetOption(csound, (char *) "--nchnls=2");
    csoundSetOption(csound, (char *) "-r44100");
    csoundSetOption(csound, (char *) "-k689.0625");
    csoundSetOption(csound, (char *) "--0dbfs=1");
    //csoundSetOption(csound, (char *) "-b1024");
    csoundSetOption(csound, (char *) "--nodisplays");
    csoundSetOption(csound, (char *) "--daemon");
    csoundStart(csound);
    compiled = true;
    StartDAC();
    return;
  }
  dac.StartPlayback();
}

void CsoundInstance::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_string()) {
    if(!var_message.is_dictionary())
      return;
    if(!input_is_on) {
      pp::VarDictionary var_dictionary_message(var_message);
      pp::Var var_input = var_dictionary_message.Get("input");
      if (!var_input.is_resource())
	return;  
      pp::Resource resource_input = var_input.AsResource();
      audio_input  = pp::MediaStreamAudioTrack(resource_input);
      audio_input.GetBuffer(audio_callback_factory.NewCallbackWithOutput(
									 &CsoundInstance::OnGetInput));
      PostMessage("Csound: started audio input...\n");
    } else {
      PostMessage("Csound: audio input has started already...\n");
    }
    return;
  }
  std::string message = var_message.AsString();
  if (message == kPlaySoundId) {
    if(!compiled) {
      PlayCsound();
      PostMessage("Csound: running...\n");
    } else {
      dac.StartPlayback();
      PostMessage("Csound: running...\n");
    }
  } else if (message == kStopSoundId) {
    dac.StopPlayback();
    PostMessage("Csound: paused...\n");
  } else if (message.find(kCsdId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      PlayCsd((char *)string_arg.c_str(), true); 
    }
  }  else if (message.find(kRenderId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      PlayCsd((char *)string_arg.c_str(), false); 
    }
  } else if (message.find(kOrchestraId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      if(compiled)
      csoundCompileOrc(csound, (char *) string_arg.c_str()); 
      else PostMessage("engine has not started yet\n");
    }
  } else if (message.find(kScoreId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      csoundReadScore(csound, (char *) string_arg.c_str()); 
    }
  } else if (message.find(kEventId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      csoundInputMessage(csound, (char *) string_arg.c_str()); 
    }
  } else if(message.find(kChannelId) == 0){
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {
      std::string string_arg = message.substr(sep_pos + 1);
      sep_pos = string_arg.find_first_of(kMessageArgumentSeparator);
      std::string channel = string_arg.substr(0, sep_pos);
      std::string svalue = string_arg.substr(sep_pos + 1);
      std::istringstream stream(svalue);
      MYFLT val;
      if (stream >> val) {
        csoundSetControlChannel(csound,(char *)channel.c_str(), val);
        return;
      }
    }
  } else if(message.find(kSChannelId) == 0){
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {
      std::string string_arg = message.substr(sep_pos + 1);
      sep_pos = string_arg.find_first_of(kMessageArgumentSeparator);
      std::string channel = string_arg.substr(0, sep_pos);
      std::string svalue = string_arg.substr(sep_pos + 1);
      csoundSetStringChannel(csound,(char *)channel.c_str(),(char *)svalue.c_str());
      return;
    }
  }
  else if(message.find(kChannelOutId) == 0){
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {
      std::string string_arg = message.substr(sep_pos + 1);
      sep_pos = string_arg.find_first_of(kMessageArgumentSeparator);
      std::string channel = string_arg.substr(0);
      char mess[64]; 
      int err;
      MYFLT val = csoundGetControlChannel(csound,(char *)channel.c_str(), &err);
      sprintf(mess, "::control::%s:%f",(char *)channel.c_str(),val);
      PostMessage(mess);
      return;  
    }
  } else if(message.find(kSetTableId) == 0){
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {
      std::string string_arg = message.substr(sep_pos + 1);
      sep_pos = string_arg.find_first_of(kMessageArgumentSeparator);
      if (sep_pos != std::string::npos){
	std::string table = string_arg.substr(0, sep_pos);
	std::string string_arg2 = string_arg.substr(sep_pos + 1);
	sep_pos = string_arg2.find_first_of(kMessageArgumentSeparator);
	if (sep_pos != std::string::npos) {
	  std::string index = string_arg2.substr(0, sep_pos);
	  std::string svalue = string_arg2.substr(sep_pos + 1);
	  std::istringstream tstream(table);
	  std::istringstream istream(index);
	  std::istringstream stream(svalue);
	  int tab, ndx;
	  MYFLT val;
	  if (stream >> val && tstream >> tab && istream >> ndx) {
	    csoundTableSet(csound, tab, ndx, val);
	    return;
	  }
	}
      }
    }
  } else if (message.find(kCopyId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      if (sep_pos != std::string::npos) {
        std::string string_arg = message.substr(sep_pos + 1);
        sep_pos = string_arg.find_first_of(kUrlArgumentSeparator);
	std::string src = string_arg.substr(0, sep_pos);
        std::string sname = string_arg.substr(sep_pos + 1);
        CopyFileToLocalAsync((char *) src.c_str(), (char *) sname.c_str()); 
      }
    }
  } else if (message.find(kCopyUrlId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      if (sep_pos != std::string::npos) {
        std::string string_arg = message.substr(sep_pos + 1);
        sep_pos = string_arg.find_first_of(kUrlArgumentSeparator);
	std::string surl = string_arg.substr(0, sep_pos);
        std::string sname = string_arg.substr(sep_pos + 1);
        CopyFromURLToLocalAsync((char *) surl.c_str(), (char *) sname.c_str()); 
      }
    }
  } else if (message.find(kGetFileId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      GetFileFromLocalAsync((char *)string_arg.c_str()); 
    }
  }
  else if (message.find(kGetTableId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {           
      std::string svalue = message.substr(sep_pos + 1);
      std::istringstream stream(svalue);
      MYFLT tab;
      if(stream >> tab){
	int len = csoundTableLength(csound, tab); 
        if(len > 0){
          PostMessage("ReadingTable:");
          pp::VarArrayBuffer v2 = pp::VarArrayBuffer(len*sizeof(MYFLT));
          void *dest = v2.Map();
          csoundTableCopyOut(csound, tab, (MYFLT*) dest);
          v2.Unmap();    
          PostMessage(v2);
          PostMessage("Table::Complete");
	}
      } 
    }
  } else if (message.find(kMIDIId) == 0){     
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) { 
      std::string string_arg1 = message.substr(sep_pos + 1);
      sep_pos = string_arg1.find_first_of(kMessageArgumentSeparator);
      if (sep_pos != std::string::npos) { 
	std::string byte1 = string_arg1.substr(0, sep_pos);
	std::string string_arg2 = string_arg1.substr(sep_pos + 1);
	sep_pos = string_arg2.find_first_of(kMessageArgumentSeparator);
	if (sep_pos != std::string::npos) { 
	  std::string byte2 = string_arg2.substr(0, sep_pos);
	  std::string byte3 = string_arg2.substr(sep_pos + 1);
	  std::istringstream stream1(byte1);
	  std::istringstream stream2(byte2);
	  std::istringstream stream3(byte3);  
	  MYFLT val;
	  Byte midi[3];
	  if (stream1 >> val) {
	    midi[0] = (unsigned char) val;
	  }
	  if (stream2 >> val) {
	    midi[1] = (unsigned char) val;
	  }
	  if (stream3 >> val) {
	    midi[2] = (unsigned char) val;
	  }
	  PutMidiData(csound, midi);
	  return;
	} else return;
      } else return;
    } else return;
}
 else {
   PostMessage("message not handled: ");
   PostMessage(message.c_str());
   PostMessage("\n");
 }
}

void* fileThreadFunc(void *data){

  CsoundInstance *p = (CsoundInstance*) data;
  FILE *fp_in, *fp_out;
  int retval=0;
  char *rem_name;
  rem_name = (char *) 
    malloc(strlen(p->GetSrcFileName())+strlen("http/"))+1;
  sprintf(rem_name,"http/%s",p->GetSrcFileName()); 
  p->PostMessage("Copying: ");
  p->PostMessage(rem_name);
  p->PostMessage("\n");
  if((fp_in = fopen(rem_name, "r"))!= NULL){
    char *local_name = (char *) 
      malloc(strlen(p->GetDestFileName())+strlen("local/"))+1;   
    sprintf(local_name,"local/%s",p->GetDestFileName());
    if((fp_out = fopen(local_name, "w"))!=NULL) {
      char buffer[512];
      int read;
      while((read = fread(buffer,1,512,fp_in))) 
    	fwrite(buffer,1,read,fp_out);
      fclose(fp_out);
      retval = 0;
      p->PostMessage("copied file: ");
      p->PostMessage(local_name);
      p->PostMessage("\n");
    } else retval = -1;
    free(local_name);
    fclose(fp_in);
  } else retval = -1; 
  free(rem_name);
  if(retval < 0){
    p->PostMessage(rem_name);
    p->PostMessage(": could not copy file\n");    
  } else p->PostMessage("Complete"); 
  p->SetFileResult(retval);
  return NULL;
}

void CsoundInstance::CopyFileToLocalAsync(char *src , char *name){
  pthread_t id;
  if(dest) free(name); 
  dest = strdup(name);
  if(from) free(from);
  from = strdup(src);
  pthread_create(&id, NULL, &fileThreadFunc,(void*) this);
}


void* fileReadThreadFunc(void *data){

  CsoundInstance *p = (CsoundInstance*) data;
  FILE *fp_in;
  p->PostMessage("Reading:");
  if((fp_in = fopen(p->GetSrcFileName(), "r"))!= NULL){
    fseek(fp_in, 0, SEEK_END);      
    long size = ftell(fp_in);
    fseek(fp_in, 0 ,SEEK_SET); 
    char *buffer = (char *) malloc(size);
    long pos = 0, bytes =0;
    while((bytes = 
	   fread(&buffer[pos],1,16384,fp_in)) 
	  != 0) pos += bytes;
    fclose(fp_in);
    pp::VarArrayBuffer v2 = pp::VarArrayBuffer(size);
    void* pDst = v2.Map();
    memcpy(pDst,buffer,size);
    v2.Unmap();    
    p->PostMessage(v2);
  }
  p->PostMessage("Complete");
  return NULL;
}

void CsoundInstance::GetFileFromLocalAsync(char *src){
  pthread_t id;
  if(from) free(from);
  from = strdup(src);
  pthread_create(&id, NULL, &fileReadThreadFunc,(void*) this);
}



void* urlThreadFunc(void *data) {
  UrlReader *d = ((UrlReader*)data);
  CsoundInstance *p = (CsoundInstance *) d->instance;
  
  char *local_name = (char *) 
    malloc(strlen(p->GetDestFileName())+strlen("local/")+1);
  sprintf(local_name,"local/%s",p->GetDestFileName()); 
  p->PostMessage("Copying memory into: ");
  p->PostMessage(local_name);
  p->PostMessage("\n");
  FILE *fp_out;
  const char *buffer = d->mem.data();
  if((fp_out = fopen(local_name, "w"))!= NULL){
    int cnt = 0;
    while(cnt < d->bytes) {        
      cnt += fwrite(&buffer[cnt],1,512,fp_out);
    }
    fclose(fp_out);
    p->PostMessage(local_name);
    p->PostMessage(": copied ");
    p->PostMessage(pp::Var(cnt));
    p->PostMessage(" bytes\n");	
  }
  free(local_name);
  delete d;
  p->PostMessage("Complete");
  return NULL;
}

UrlReader *UrlReader::Create(pp::Instance *inst, char *path){
  return new UrlReader(inst, path);
}

UrlReader::UrlReader(pp::Instance *inst, char *path) :
  url(path), instance(inst),
  url_request(inst), url_loader(inst), cc_factory(this), bytes(0) 
{
  url_request.SetURL(url);
  url_request.SetMethod("GET");
  url_request.SetProperty(PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS, pp::Var(true));
  instance->PostMessage("UrlReader\n");
}
  
void UrlReader::Start() {
  pp::CompletionCallback cc =
    cc_factory.NewCallback(&UrlReader::OnOpenUrl);
  url_loader.Open(url_request,cc);
}

void UrlReader::ReadBody(){
  pp::CompletionCallback cc =
    cc_factory.NewOptionalCallback(&UrlReader::OnRead);
  int result;
  do {
    result = url_loader.ReadResponseBody(buffer,512,cc);
    if(result > 0){
      bytes += result;
      int end = std::min(32768,result);
      mem.insert(mem.end(), buffer, buffer+end);
    }
  }
  while(result > 0);
  if (result != PP_OK_COMPLETIONPENDING) {
    cc.Run(result);
  } 
}

void UrlReader::OnRead(int32_t result){
  if (result == PP_OK){
    pthread_t id;
    pthread_create(&id, NULL, 
		   &urlThreadFunc,(void*)this);
    instance->PostMessage("...done\n");
  }
  else if (result > 0) {
    bytes += result;
    int end = std::min(32768,result);
    mem.insert(mem.end(), buffer, buffer+end);
    ReadBody();
  }
}

void UrlReader::OnOpenUrl(int32_t result){    
  if(result != PP_OK){
    instance->PostMessage("URL open failed.\n");
    return;
  } 
  instance->PostMessage("loading URL data into memory...\n");
  ReadBody();
}

void CsoundInstance::CopyFromURLToLocalAsync(char *URL,char *name){
  if(dest) free(dest); 
  dest = strdup(name);
  UrlReader *r = UrlReader::Create(this, URL); 
  PostMessage("created URL urlReader \n");
  if(r != NULL) r->Start();
}

class CsoundModule : public pp::Module {
public:
  CsoundModule() : pp::Module() {}
  ~CsoundModule() {
  }

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new CsoundInstance(instance, get_browser_interface());
  }
};

namespace pp {
  Module* CreateModule() { return new CsoundModule(); }
}  
