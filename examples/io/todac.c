/*********************************************************/
/* todac.c                                               */
/* reads ASCII float samples from stdin and puts them    */
/* to an audio device via portaudio                      */
/* (c) V Lazzarini, 2012                                 */
/*                                                       */
/* This program is licensed under the GNU Public Licence */
/*   See License.txt for details                         */
/*********************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <portaudio.h>
#include <math.h>

void usage();
int main(int argc, char** argv){

  PaError err;
  const PaDeviceInfo *info;
  PaStreamParameters outparam;
  PaStream *handle = NULL;
  int i, chn=1,bufsize=4096,sr=44100, dev;
  float *buf, out = 0.f;

  if(argc > 1)
    sr = atoi(argv[1]);
  if(argc > 2)
    chn = atoi(argv[2]);
  if(argc > 3)
    usage();

  err = Pa_Initialize();
  if(err == paNoError){
    dev = Pa_GetDefaultOutputDevice();
    buf =  (float *) malloc(sizeof(float)*bufsize); 
    memset(buf, 0, sizeof(float)*bufsize);
    memset(&outparam, 0, sizeof(PaStreamParameters));
    outparam.device = (PaDeviceIndex) dev;
    outparam.channelCount = chn;
    outparam.sampleFormat = paFloat32;
    outparam.suggestedLatency = (PaTime)
      (bufsize/chn)/(MYDBL)sr;

    err = Pa_OpenStream(&handle,NULL,&outparam,
			sr,bufsize,paNoFlag, 
			NULL, NULL);
    
    if(err == paNoError){
      err = Pa_StartStream(handle);
      if(err == paNoError){
        long cnt, i; 
        do{
	  cnt = 0;
	  for(i = 0; i < bufsize; i++) {
	    cnt += fscanf(stdin, "%f", &buf[i]);
	  }
	  if(cnt > 0) { 
            err = (int) Pa_WriteStream(handle, buf, cnt/chn);
	    if(err != paNoError)
	      printf("write error: %s \n", Pa_GetErrorText(err));
	  }
	  else break;
	} while(cnt > 0);        
	Pa_StopStream(handle);
      } else printf("%s \n", Pa_GetErrorText(err));
      Pa_CloseStream(handle);
    } else printf("%s \n", Pa_GetErrorText(err));
    Pa_Terminate();
  }  else  printf("%s \n", Pa_GetErrorText(err));
  return 0;
}


void usage()
{
  fprintf (stderr,
	   "usage: todac [sr] [channels] < input\n"
	   );
  exit(1);
}
