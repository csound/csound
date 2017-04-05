/*
 * live coding demo for Csound 6
 * Author: Rory Walsh 2013
 * 
 * 
 */ 

#include "liveCoder.hpp"
uintptr_t csThread(void *clientData);
int main(int argc, char *argv[])
{
  liveCsound* csound;
  void *iD;

  csound = new liveCsound();	
  csound->SetOption((char *)"-odac");
  csound->SetOption((char *)"-dm0");
  csound->InputMessage("f2 0 16 2 .00 .02 .04 .05 .07 .08 .09 .11");
  csound->Start();

  if(!csound->result){
    csound->perf_status=1;
    iD = csoundCreateThread(csThread, (void*)csound);
  }
  else
    return 0;

  string usage = 
    "\n\n============================\n\
CSOUND LIVE CODE DEMO\n\
============================\n\
Usage: \n\
After starting the program add an instrument definition such as:\n\
\ninstr 1\n\
a1 oscil 10000, 400, 1\n\
endin\n\n\
To update the orchestra type uorc and hit enter.\n\n\
The following commands are also available\n\
usco - updates the score, follows a score line\n\
orchist - displays the entire orch history\n\
scohist - displays the entire score histroy\n\
ends - ends session\n\n\
chnset can also be used to update any channels by simply\n\
writing it as you would in a normal instrument definition, e.g\n\n\
chnset 5, \"tempo\"\n\n\
============================\n\n\n";
  cout << usage;
  while(csound->runLiveCodeSession());

  csound->perf_status=0;
  csoundJoinThread(iD);
  delete csound;
  return 1;

  return 0;
}	


//Csound performance thread
uintptr_t csThread(void *data)
{
  liveCsound* csound = (liveCsound*)data;
  if(!csound->result)
    {
      while((csoundPerformKsmps(csound->GetCsound()) == 0)
	    &&(csound->perf_status==1)){
				
      }
      csound->perf_status = 0; 
		
    }       
  return 1;
}

