/*********************************************************/
/* Bela Csound Rendering functions                       */
/*                                                       */
/*********************************************************/

#include <Bela.h>
#include <csound/csound.hpp>

struct csData{
  Csound *csound;
  int blocksize;
  int res;
  int count;
};
  
csData* gCsData; 
  
bool setup(BelaContext *context, void *userData)
{
  Csound *csound = new Csound();
  gCsData  = new csData;	
  csound->SetHostImplementedAudioIO(1,32);
  gCsData->res = csound->Compile("simple.csd", "-iadc", "-odac",
				 "-+rtaudio=null", "--realtime"); // TODO get CSD name
  gCsData->csound = csound;
  gCsData->blocksize = csound->GetOutputBufferSize();
  gCsData->count = 0;
  
  if(gCsData->res != 0) return false;
  else return true;
}

void render(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  if(gCsData->res == 0) {
    int n,i,frame,blocksize, res;
    MYFLT scal = userData->csound->Get0dBFS();
    MYFLT* audioIn = userData->csound->GetInputBuffer();
    MYFLT* audioOut = userData->csound->GetOutputBuffer();
    int nchnls = userData->csound->GetNchnls();
    int chns = nchnls;
    frame = userData->count;
    blocksize = userData->blocksize;

    /* this is called when Csound is not running */
    if(frame < 0) {
      for(n = 0; n < context->audioFrames; n++){
	for(i = 0; i < context->audioOutChannels; i++){
	  audioWrite(context,n,i,0);
	}
      }
      return;
    }

    if(chns > context->audioOutChannels)
      chns = context->audioOutChannels;
    /* this is where Csound is called */
    for(n = 0; n < context->audioFrames; n++){
      if(frame == blocksize) {
	if((res = userData->csound->PerformBuffer()) == 0) j = 0;
	else {
	  frame = -1;
	  break;
	}
      }
      for(i = 0; i < chns; i++){
	audioIn[frame+i] = audioRead(context,n,i);
	audioWrite(context,n,i,audioOut[frame+i]/scal);
      }
      frame += nchnls;	
    }
    gCsData->res = res;
    userData->count = frame;
  }
}

void cleanup(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  delete userData->csound;
  delete userData;
}

