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

namespace {
  const char* const kPlaySoundId = "playCsound";
  const char* const kStopSoundId = "pauseCsound";
  const char* const kOrchestraId = "orchestra";
  const char* const kScoreId = "score";
  const char* const kEventId = "event";
  const char* const kChannelId = "channel";
  const char* const kCopyId = "copyToLocal";
  const char* const kCopyUrlId = "copyUrlToLocal";
  const char* const kCsdId = "csd";
  static const char kMessageArgumentSeparator = ':';
  static const char kUrlArgumentSeparator = '#';

  const double kDefaultFrequency = 440.0;
  const double kPi = 3.141592653589;
  const double kTwoPi = 2.0 * kPi;
  const uint32_t kSampleFrameCount =512u;
  const uint32_t kChannels = 2u;
}  // namespace


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

  void PlayCsound();
  void PlayCsd(char *c);
  void CopyFileToLocalAsync(char *from, char *to);
  void CopyFromURLToLocalAsync(char *URL, char *name); 
  
  static void CsoundCallback(void* samples,
			     uint32_t buffer_size,
			     void* data) {
    CsoundInstance* instance = (CsoundInstance*) data;
    CSOUND *csound_ = instance->csound;
    if(csound_ != NULL) {
      int count_ = instance->count;
      int n, buffsamps = buffer_size / sizeof(short);
      short* buff = (short*) samples;
      MYFLT _0dbfs = csoundGet0dBFS(csound_);
      MYFLT *spout = csoundGetSpout(csound_); 
      int ksmps = csoundGetKsmps(csound_)*csoundGetNchnls(csound_);
         
     
      MYFLT scale = 32768./_0dbfs;
      if(spout != NULL) 
	for(n=0; n < buffsamps; n++) {
	  if(count_ == 0) {
	    int ret = csoundPerformKsmps(csound_);
	    if(ret != 0) return;
	    count_ = ksmps;
	  }
	  buff[n] = (int16_t) (scale*spout[ksmps-count_]);
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
  pthread_t fileThread;
  char *GetDestFileName(){ return dest; }
  void SetFileResult(int res){ fileResult = res; }
  char *GetSrcFileName(){ return from; }
  bool isCompiled() { return compiled; }
  void isCompiled(bool c) { compiled = c; }
  virtual bool Init(uint32_t argc, const char* argn[], const char* argv[]);
  virtual void HandleMessage(const pp::Var& var_message);
  CSOUND *GetCsound() { return csound; }
  char *GetCsd() { return csd;}
  bool StartDAC();

  explicit CsoundInstance(PP_Instance instance, 
			  PPB_GetInterface get_browser_interface)
    : pp::Instance(instance),
      csound(NULL), count(0), fileResult(0), 
      fileThread(NULL), 
      from(NULL), dest(NULL), csd(NULL), compiled(false)
        
  {
    get_browser_interface_ = get_browser_interface;
  }

  virtual ~CsoundInstance() {
    if(dest) free(dest); 
    if(from) free(from);
    if(csound){
      csoundDestroyMessageBuffer(csound);
      csoundDestroy(csound);
    }
  }

 
};

bool CsoundInstance::Init(uint32_t argc,
			  const char* argn[],
			  const char* argv[]) {

  csound = csoundCreate(NULL);
  csoundCreateMessageBuffer(csound, 0);  
  csoundSetHostImplementedAudioIO(csound,1,0);
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
        PP_AUDIOSAMPLERATE_44100, kSampleFrameCount);
    dac = pp::Audio(
		  this,
		  pp::AudioConfig(this, PP_AUDIOSAMPLERATE_44100, frames),
		  CsoundCallback,
		  this);
    dac.StartPlayback();
    return true;
}

void* compileThreadFunc(void *data) {
  CsoundInstance *p = (CsoundInstance *) data;
  CSOUND *csound = p->GetCsound();
  char *csd =  p->GetCsd();
  char *argv[] = {(char *)"csound", p->GetCsd()}; 
  MYFLT sr = 0.0;
  if(csoundCompile(csound,2,argv) == 0){
  if(p->StartDAC())
    p->isCompiled(true);
  else {
    return NULL;
  }
   free(csd);
  }
   return NULL;
}


void CsoundInstance::PlayCsd(char *c) {
  if(!compiled){
  csd = strdup(c);
  pthread_t t;
  pthread_create(&t, NULL, compileThreadFunc, this);
  } else 
  PostMessage("Csound is already started \n"
	      "Refresh page to play a different CSD\n");
}



void CsoundInstance::PlayCsound() {
  if(!compiled) {
    csoundSetOption(csound, (char *) "-odac");
    csoundSetOption(csound, (char *) "--nchnls=2");
    csoundSetOption(csound, (char *) "-r44100");
    csoundSetOption(csound, (char *) "-k689.0625");
    csoundSetOption(csound, (char *) "--0dbfs=1");
    csoundSetOption(csound, (char *) "-b1024");
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
      PlayCsd((char *)string_arg.c_str()); 
    }
  } else if (message.find(kOrchestraId) == 0) {
    size_t sep_pos = message.find_first_of(kMessageArgumentSeparator);
    if (sep_pos != std::string::npos) {      
      std::string string_arg = message.substr(sep_pos + 1);
      csoundCompileOrc(csound, (char *) string_arg.c_str()); 
    }
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
  } else {
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
  if(retval < 0){
    p->PostMessage(rem_name);
    p->PostMessage(": could not copy file\n");
  }
  free(rem_name);
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
    p->PostMessage("Complete\n");
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
