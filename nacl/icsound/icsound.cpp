/*
 * Csound pnacl interactive frontend
 * based on nacl sdk audio api example
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
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var.h"
#include <sys/mount.h>
#include <errno.h>
#include "nacl_io/nacl_io.h"
#include <stdio.h>
#include <csound.h>

namespace {
const char* const kPlaySoundId = "playSound";
const char* const kStopSoundId = "stopSound";
const char* const kOrchestraId = "orchestra";
const char* const kScoreId = "score";
const char* const kEventId = "event";
const char* const kChannelId = "channel";
const char* const kCopyId = "copyToLocal";
const char* const kCopyUrlId = "copyUrlToLocal";
static const char kMessageArgumentSeparator = ':';
static const char kUrlArgumentSeparator = '#';

const double kDefaultFrequency = 440.0;
const double kPi = 3.141592653589;
const double kTwoPi = 2.0 * kPi;
const uint32_t kSampleFrameCount =512u;
const uint32_t kChannels = 2u;
}  // namespace


class AudioInstance : public pp::Instance {

 public:
  explicit AudioInstance(PP_Instance instance, 
                         PPB_GetInterface get_browser_interface)
      : pp::Instance(instance),
        csound(NULL), count(0), fileResult(0), 
        fileThread(NULL), urlThread(NULL), 
        fname(NULL), url(NULL) {
    get_browser_interface_ = get_browser_interface;
}
  virtual ~AudioInstance() {
    if(fname) free(fname); 
    if(url) free(url);
    if(csound){
      csoundDestroyMessageBuffer(csound);
      csoundDestroy(csound);
    }
  }

  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void HandleMessage(const pp::Var& var_message);
  void CopyFileToLocalAsync(char *name);
  void CopyFromURLToLocalAsync(char *URL, char *name);  

 private:  
  pp::Audio audio_;
  CSOUND *csound;
  int count;
  PPB_GetInterface get_browser_interface_;
  int fileResult;
  pthread_t fileThread;
  pthread_t urlThread;
  char *fname;
  char *url;
  
  static void CsoundCallback(void* samples,
                               uint32_t buffer_size,
                               void* data) {
    AudioInstance* instance = (AudioInstance*) data;
    CSOUND *csound_ = instance->csound;
    if(csound_ != NULL) {
     int count_ = instance->count;
     int n, buffsamps = buffer_size / sizeof(short);
     short* buff = (short*) samples;
     MYFLT _0dbfs = csoundGet0dBFS(csound_);
     MYFLT *spout = csoundGetSpout(csound_); 
     int ksmps = csoundGetKsmps(csound_)*csoundGetNchnls(csound_);
         
     if(spout != NULL) 
       for(n=0; n < buffsamps; n++) {
	 if(count_ == 0) {
         int ret = csoundPerformKsmps(csound_);
         if(ret != 0) return;
         count_ = ksmps;
	 }
         buff[n] = (int16_t) (32768*spout[ksmps-count_]);
         count_--;
       }
     instance->count = count_;

    while(csoundGetMessageCnt(csound_)){
       instance->PostMessage(csoundGetFirstMessage(csound_));
       csoundPopFirstMessage(csound_);
    }

    }
  }  
public:
  char *GetFileName(){ return fname; }
  void SetFileResult(int res){ fileResult = res; }
  char *GetUrl(){ return url; }         
};

bool AudioInstance::Init(uint32_t argc,
                         const char* argn[],
                         const char* argv[]) {

  int frames = pp::AudioConfig::RecommendSampleFrameCount(
    this, PP_AUDIOSAMPLERATE_44100, kSampleFrameCount);

  csound = csoundCreate(NULL);
  csoundCreateMessageBuffer(csound, 0);
  csoundSetHostImplementedAudioIO(csound,1,0);
  csoundSetOption(csound, (char *) "-odac");
  csoundSetOption(csound, (char *) "--nchnls=2");
  csoundSetOption(csound, (char *) "-r44100");
  csoundSetOption(csound, (char *) "-k689.0625");
  csoundSetOption(csound, (char *) "--0dbfs=1");
  csoundSetOption(csound, (char *) "-b1024");
  csoundSetOption(csound, (char *) "--daemon");
  csoundStart(csound);
 
  audio_ = pp::Audio(
      this,
      pp::AudioConfig(this, PP_AUDIOSAMPLERATE_44100, frames),
      CsoundCallback,
      this);
  frames = csoundGetOutputBufferSize(csound);

  nacl_io_init_ppapi(pp_instance(),get_browser_interface_);

  mount("",                                       /* source */
        "/local",                                 /* target */
        "html5fs",                                /* filesystemtype */
        0,                                        /* mountflags */
        "type=TEMPORARY"); /* data */

  int res = mount("",       /* source. Use relative URL */
        "/http",  /* target */
        "httpfs", /* filesystemtype */
        0,        /* mountflags */
        "");      /* data */
  

  return true;
}

void AudioInstance::HandleMessage(const pp::Var& var_message) {
  if (!var_message.is_string()) {
    return;
  }
  std::string message = var_message.AsString();
  if (message == kPlaySoundId) {
    audio_.StartPlayback();
    PostMessage("Csound: running...\n");
  } else if (message == kStopSoundId) {
    audio_.StopPlayback();
    PostMessage("Csound: paused...\n");
  } else if (message.find(kOrchestraId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      csoundCompileOrc(csound, (char *) string_arg.c_str()); 
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
  } else if (message.find(kCopyId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      CopyFileToLocalAsync((char *) string_arg.c_str()); 
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
  }

}

void* fileThreadFunc(void *data){

  AudioInstance *p = (AudioInstance*) data;
  char *name = p->GetFileName();
  FILE *fp_in, *fp_out;
  int retval=0;
  char *rem_name;
  if(p->GetUrl() == NULL){
   rem_name = (char *) malloc(strlen(name)+strlen("http/"));
   sprintf(rem_name,"http/%s",name); 
  }
  else{
   rem_name = (char *) malloc(strlen(name)+strlen("url/"));
   sprintf(rem_name,"url/%s",name); 
  }
  p->PostMessage("Copying: ");
  p->PostMessage(rem_name);
  p->PostMessage("\n");
  if((fp_in = fopen(rem_name, "r"))!= NULL){
    char *local_name = (char *) malloc(strlen(name)+strlen("local/"));
    sprintf(local_name,"local/%s",name);  
    if((fp_out = fopen(local_name, "w"))!=NULL) {
      char buffer[512];
      int read;
      while((read = fread(buffer,1,512,fp_in))) 
    	fwrite(buffer,1,read,fp_out);
      fclose(fp_out);
      retval = 0;
      p->PostMessage("copied file: ");
      sprintf(rem_name,"http/%s",name); 
      p->PostMessage("\n");
    } else retval = -1;
    free(local_name);
    fclose(fp_in);
  } else retval = -1; 
  if(retval < 0){
   p->PostMessage(rem_name);
   p->PostMessage(": could not copy file\n");
  }
  free(rem_name);
  p->SetFileResult(retval); 
  return NULL;
}

void AudioInstance::CopyFileToLocalAsync(char *name){
 if(fname) free(fname); 
 fname = strdup(name);
 if(url){
   free(url);
   url = NULL;
  }
  pthread_create(&fileThread, NULL, &fileThreadFunc,(void*) this);
}

void* urlThreadFunc(void *data){
  AudioInstance *p = (AudioInstance*) data;
  mount(p->GetUrl(), /* source. Use relative URL */
        "/url",  /* target */
        "httpfs", /* filesystemtype */
        0,        /* mountflags */
        "");      /* data */
  fileThreadFunc(data);
  umount("/tmp");  
  return NULL;
}

void AudioInstance::CopyFromURLToLocalAsync(char *URL,char *name){
   fname = name;
   if(fname) free(fname); 
   fname = strdup(name);
   if(url) free(url);
   url = strdup(URL);
   pthread_create(&urlThread, NULL, &urlThreadFunc,(void*) this);
}





class AudioModule : public pp::Module {
 public:
  AudioModule() : pp::Module() {}
  ~AudioModule() {
  }

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new AudioInstance(instance, get_browser_interface());
  }
};

namespace pp {
Module* CreateModule() { return new AudioModule(); }
}  // namespace pp
