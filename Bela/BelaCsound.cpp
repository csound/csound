/*******************************************************************************/
/* Bela Csound Rendering functions                                             */
/*                                                                           */
/*******************************************************************************/

#include <Bela.h>
#include <csound/csound.hpp>
#include <vector>
#include <sstream>

struct csChan {
  std::vector<MYFLT> data;
  std::stringstream name;
};

struct csData{
  Csound *csound;
  int blocksize;
  int res;
  int count;
  csChan channel[8];
};
  
csData* gCsData;

bool setup(BelaContext *context, void *userData)
{
  Csound *csound = new Csound();
  const char *csdfile = "my.csd"; /* CSD name */
  int numArgs = 6;
  char *args[] = { "csound", csdfile, "-iadc", "-odac","-+rtaudio=null",
		   "--realtime", "--daemon"};
  
  gCsData  = new csData;
  csound->SetHostImplementedAudioIO(1,32);
  gCsData->res = csound->Compile(numArgs, args);
  gCsData->csound = csound;
  gCsData->blocksize = csound->GetOutputBufferSize();
  gCsData->count = 0;
  
  /* set up the channels */
  for(int i; i < context->analogInChannels; i++) {
    gCsData.channel[i].data.resize(csound->GetKsmps());
    gCsData.channel[i].name << "analogue" << i+1;
  }
  
  if(gCsData->res != 0) return false;
  else return true;
}

void render(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  if(gCsData->res == 0) {
    int n,i,k,count, frmcount,blocksize,res;
    MYFLT scal = userData->csound->Get0dBFS();
    MYFLT* audioIn = userData->csound->GetInputBuffer();
    MYFLT* audioOut = userData->csound->GetOutputBuffer();
    int nchnls = userData->csound->GetNchnls();
    int chns = nchnls;
    int an_chns = context->analogInChannels;
    CsChan *channel = userData->channel;
    float frm = 0, incr = ((float) context->analogFrames)/context->audioFrames;
    int an_chans = context->analogInChannels;
    count = userData->count;
    blocksize = userData->blocksize;
    

    /* this is called when Csound is not running */
    if(count < 0) {
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
    for(n = 0; n < context->audioFrames; n++, frm+=incr, count+=nchnls){
      if(count == blocksize) {
	/* set the channels */
	for(i = 0; i < an_chns; i++) {
          csound->SetChannel(channel[i].name.str().c_str(),
			     &(channel[i].data[0]));
	}
	if((res = userData->csound->PerformBuffer()) == 0) count = 0;
	else {
	  count = -1;
	  break;
	}
      }
      /* read/write audio data */
      for(i = 0; i < chns; i++){
	audioIn[count+i] = audioRead(context,n,i);
	audioWrite(context,n,i,audioOut[count+i]/scal);
      }
 
      /* read analogue data 
         analogue frame pos gets incremented according to the
         ratio analogFrames/audioFrames.
      */
      frmcount = count/nchnls;
      for(i = 0; i < an_chns; i++) {
	k = (int) frm;
        channel[i].data[frmcount] = analogRead(context,k,i);
      }
	
    }
    gCsData->res = res;
    userData->count = count;
  }
}

void cleanup(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  delete userData->csound;
  delete userData;
}

