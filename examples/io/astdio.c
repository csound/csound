// Audio IO module example
// V Lazzarini, Nov 2024
#include <csdl.h>
#include <poll.h>

static int32_t open_output(CSOUND *csound,
		const csRtAudioParams *p){
  // nothing to do here, so exit
  return OK;
}

static int32_t open_input(CSOUND *csound,
		const csRtAudioParams *p){
  // set stdin to receive data
  int mode = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, mode | O_NDELAY);
}

static void audio_out(CSOUND *csound, const MYFLT *s, int32_t nbytes) {
  int32_t n, nsmps;
  // number of interleaved samples in buffer
  nsmps = nbytes / sizeof(MYFLT);
  // send each sample to stdout
  for(n = 0; n < nsmps; n++)
    fprintf(stdout, "%f\n", s[n]);
}

static int32_t audio_in(CSOUND *csound, MYFLT *s, int32_t nbytes) {
  struct pollfd fd = {0, POLLIN, 0};
  int32_t cnt = 0, nsmps;
  MYFLT data;
  // buffer size in samples
  nsmps = nbytes / sizeof(MYFLT);
  // poll for input data on stdin
  if(poll(&fd, 1, 0)) {
    // read each sample from input
    while(scanf("%f", &data) > 0 && cnt < nsmps)
      s[cnt++] = data;
  }
  // return the number of samples read
  return cnt/sizeof(MYFLT);
}

static void close_device(CSOUND *csound) {
  // nothing to do
}


int32_t csoundModuleCreate(CSOUND *csound){
  // print message announcing module
  csound->Message(csound, "stdio audio module\n");
  return 0;
}

int32_t csoundModuleInit(CSOUND *csound)
{
  // get requested module name
  char *module = (char*)
    (csound->QueryGlobalVariable(csound, "_RTAUDIO"));
  if (module == NULL)
    return 0;
  // module section check
  if (strcmp(module, "stdio") != 0)
    return 0;
  // print message - module selected
  csound->Message(csound, "stdio audio module enabled\n");
  csound->SetPlayopenCallback(csound, open_output);
  csound->SetRecopenCallback(csound, open_input);
  csound->SetRtplayCallback(csound, audio_out);
  csound->SetRtrecordCallback(csound, audio_in);
  csound->SetRtcloseCallback(csound, close_device);
  return 0;
}

// versioning
int32_t csoundModuleInfo(void)
{
  return ((CS_VERSION << 16) +
          (CS_SUBVER << 8) +
          (int32_t) sizeof(MYFLT));
}
