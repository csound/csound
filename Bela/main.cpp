/*********************************************************/
/* Bela Csound Main Program                              */
/* takes the option --csd=name                           */
/*                                                       */
/*********************************************************/
#include <Bela.h>
#include <csound/csound.hpp>
#include <unistd.h>
#include <cstring>

struct csData{
  Csound *csound;
  int blocksize;
  int res;
  int count;
  const char* csd;
};

int main(int argc, const char *argv[]) {
  csData CsData;
  const option opt[] = {{"csd", required_argument, NULL, 0},
			{NULL, 0, NULL, 0}};
  BelaInitSettings settings;
  Bela_defaultSettings(&settings);
  if(Bela_getopt_long(argc, argv, "", opt, &settings) != -1) {
    csData.csd = new char[strlen(optarg)+1];
    std::strcpy(csData.csd,optarg);
    bool res = Bela_initAudio(&settings, &CsData);
    if(res){
       Bela_startAudio();
       while(CsData.res == 0) sleep(1);
       Bela_stopAudio();
    }
    Bela_cleanupAudio();
    return 0;
  }
  fprintf(stderr, "no csd provided \n");
  return -1;
}


bool setup(BelaContext *context, void *userData)
{
  Csound *csound = new Csound();
  csData *CsData = (csData *) userData;
  csound->SetHostImplementedAudioIO(1,32);
  CsData->res = csound->Compile(CsData.csd, "-iadc", "-odac",
				"-+rtaudio=null", "--realtime"); // TODO get CSD name
  CsData->csound = csound;
  CsData->blocksize = csound->GetOutputBufferSize();
  CsData->count = 0;
  if(CsData->res != 0) return false;
  else return true;
}

void render(BelaContext *context, void *userData)
{
  csData *userData = (csData *) userData;
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
	if((res = userData->csound->PerformBuffer()) == 0) frame = 0;
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

void cleanup(BelaContext *context, void *userData)
{
  csData *CsData = (csData *) userData;
  delete CsData->csound;
}
