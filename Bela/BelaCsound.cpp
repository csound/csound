/*******************************************************************************/
/* Bela Csound Rendering functions                                             */
/*                                                                             */
/*******************************************************************************/

#include <Bela.h>
#include <csound/csound.hpp>
#include <vector>
#include <sstream>

static int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev);
static int CloseMidiInDevice(CSOUND *csound, void *userData);
static int ReadMidiData(CSOUND *csound, void *userData, unsigned char *mbuf,
			int nbytes);

struct csChan {
  std::vector<MYFLT> data;
  std::stringstream name;
};

struct csData{
  Csound *csound;
  int blocksize;
  int res;
  int count;
  std::vector<csChan> channel;
};

struct csMIDI {
  Midi midi;
};
  
csData* gCsData;


bool setup(BelaContext *context, void *userData)
{
  Csound *csound = new Csound();
  const char *csdfile = "my.csd"; /* CSD name */
  const char *midiDev = "-Mhw:1,0,0"; /* MIDI device */
  int numArgs = 8;
  const char *args[] = { "csound", csdfile, "-iadc", "-odac", "-+rtaudio=null",
		   "--realtime", "--daemon", midiDev};
  
  gCsData  = new csData;
  csound->SetHostImplementedAudioIO(1,0);
  csound->SetHostImplementedMIDIIO(1);
  csound->SetExternalMidiInOpenCallback(OpenMidiInDevice);
  csound->SetExternalMidiReadCallback(ReadMidiData);
  csound->SetExternalMidiInCloseCallback(CloseMidiInDevice);
  gCsData->res = csound->Compile(numArgs, args);
  gCsData->csound = csound;
  gCsData->blocksize =
    csound->GetKsmps()*userData->csound->GetNchnls();
  gCsData->count = 0;
  
  /* set up the channels */
  gCsData->channel.resize(context->analogInChannels);
  for(int i; i < context->analogInChannels; i++) {
    gCsData->channel[i].data.resize(csound->GetKsmps());
    gCsData->channel[i].name << "analogue" << i+1;
  }
  
  if(gCsData->res != 0) return false;
  else return true;
}

void render(BelaContext *context, void *Data)
{
  csData *userData = gCsData;
  if(gCsData->res == 0) {
    int n,i,k,count, frmcount,blocksize,res;
    CSOUND *csound = userData->csound;
    MYFLT scal = csound->Get0dBFS();
    MYFLT* audioIn = csound->GetSpin();
    MYFLT* audioOut = csound->GetSpout();
    int nchnls = csound->GetNchnls();
    int chns = nchnls;
    int an_chns = context->analogInChannels;
    csChan *channel = &(userData->channel[0]);
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
	/* run csound */
	if((res = csound->PerformKsmps()) == 0) count = 0;
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

/** MIDI Input functions 
 */
int OpenMidiInDevice(CSOUND *csound, void **userData, const char *dev) {
  csMIDI *midiData = new CsMIDI;
  Midi &midi = midiData->midi;
  midi.readFrom(dev);
  midi.enableParser(false);
  *userData = (void *) midiData;
}

int CloseMidiInDevice(CSOUND *csound, void *userData) {
  csMIDI *midiData = (CsMIDI *) userData;
  delete midiData;
}

int ReadMidiData(CSOUND *csound, void *userData,
		 unsigned char *mbuf, int nbytes) {
  int n = 0;
  csMIDI *midiData = (CsMIDI *) userData;
  Midi &midi = midiData->midi;
  
  while((byte = midi.getInput()) >= 0) {
    *mbuf++ = (unsigned char) byte;
    if(++n == nbytes) break;
  }
  
  return n;				   
}
