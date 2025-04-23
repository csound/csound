#include <csdl.h>
// Audio IO module example
// V Lazzarini, Nov 2024
#include <poll.h>

static int32_t open_device(CSOUND *csound, const csRtAudioParams *p){
  return OK;
}

static void audio_out(CSOUND *csound, const MYFLT *s, int32_t nbytes) {
  int32_t n, nsmps;
  nsmps = nbytes / sizeof(MYFLT);
  for(n = 0; n < nsmps; n++)
    fprintf(stdout, "%f\n", s[n]);
}

static int32_t audio_in(CSOUND *csound, MYFLT *s, int32_t nbytes) {
  struct pollfd fd = {0, POLLIN, 0};
  int32_t cnt = 0, nsmps;
  MYFLT data;
  nsmps = nbytes / sizeof(MYFLT);
  if(poll(&fd, 1, 0)) {
   while(scanf("%f", &data) > 0 && cnt < nsmps)
       s[cnt++] = data;
  }
  return cnt/sizeof(MYFLT);  
}

static void close_device(CSOUND *csound) { }
  

int32_t csoundModuleCreate(CSOUND *csound){
    csound->Message(csound, "stdio audio module\n");
    return 0;
}

int32_t csoundModuleInit(CSOUND *csound)
{
    char *module = (char*)
      (csound->QueryGlobalVariable(csound, "_RTAUDIO"));
    if (module == NULL)
      return 0;
    if (strcmp(module, "stdio") != 0)
      return 0;
    csound->Message(csound, "stdio audo module enabled\n");
    csound->SetPlayopenCallback(csound, open_device);
    csound->SetRecopenCallback(csound, open_device);
    csound->SetRtplayCallback(csound, audio_out);
    csound->SetRtrecordCallback(csound, audio_in);
    csound->SetRtcloseCallback(csound, close_device);
    return 0;
}

int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) +
            (CS_SUBVER << 8) +
            (int32_t) sizeof(MYFLT));
}
