#include <Bela.h>
#include <csound/csound.hpp>
#include <csound/csPerfThread.hpp>

#define BLOCKS 8

struct csData{
  Csound *csound;
  CsoundPerformanceThread *thread;
  void *cbuff;
  int blocksize;
  int count;
  int read;
  MYFLT *buffer;
  MYFLT scal;
};

csData* gCsData;

void cback(void *p){
  csData* pp = (csData *) p;
  if(pp->thread->GetStatus() == 0)
     csoundWriteCircularBuffer(csound, pp->cbuff,
			    (void *) csoundGetSpout(csound),
			    pp->blocksize);
}

bool setup(BelaContext *context, void *userData)
{

  gCsData  = new csData;
  Csound *csound = new Csound();
  csound->Compile("trapped.csd","-+rtaudio=null", "--realtime");
  gCsData->blocksize = csound->GetNchnls()*csound->GetKsmps();
  gCsData->cbuff = csoundCreateCircularBuffer(csound->GetCsound(),
					      gCsData->blocksize*BLOCKS,
					      sizeof(MYFLT));

  gCsData->thread = new CsoundPerformanceThread(csound->GetCsound());
  gCsData->thread->SetProcessCallback(cback,gCsData);
  gCsData->csound = csound;
  gCsData->count = 0;
  gCsData->read = 0;
  gCsData->scal = userData->csound->Get0dBFS();
  gCsData->buffer = new MYFLT[gCsData->blocksize];
  gCsData->thread->Play();
  //userData = data;
    
  return true;
}

void render(BelaContext *context, void *Data)
{
  int n,i,j,blocksize,read;
  csData *userData = gCsData;
  MYFLT *buffer = userData->buffer;
  MYFLT scal = userData->scal;
  j = userData->count;
  read = userData->read;
  blocksize = userData->blocksize;
    
  for(n = 0; n < context->audioFrames; n++){
    for(i = 0; i < context->audioOutChannels; i++){
      if(j >= read) {
	read = csoundReadCircularBuffer(NULL,
				     userData->cbuff,
				     buffer,
				     blocksize);
	j = 0;
      }
      if(read > 0)
        audioWrite(context,n,i,(float)buffer[j++]/scal);
    }
  }
  userData->count = j;
  userData->read = read;
}

void cleanup(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  gCsData->thread->Stop();
  gCsData->thread->Join();
  csoundDestroyCircularBuffer(gCsData->csound->GetCsound(),
			      gCsData->cbuff);
  delete userData->csound;
  delete userData->thread;
  delete[] userData->buffer;
  delete userData;
}
