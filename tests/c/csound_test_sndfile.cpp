/*
 * File:   csound_test_sndfile.cpp
 * Author: vlazzarini
 *
 * Created on Nov 17, 2024
 */

#include "gtest/gtest.h"
#include <stdio.h>
#include <stdlib.h>
#include "csound.h"
#include "csound_files.h"
#include "csdl.h"


class SndfileTests : public ::testing::Test {
public:
    SndfileTests ()
    {
    }

    virtual ~SndfileTests ()
    {
    }

    virtual void SetUp ()
    {
        csound = csoundCreate (NULL,NULL);
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
    }

    CSOUND* csound {nullptr};
};


 struct sfile {
   FILE *fp;
   SFLIB_INFO *sfinfo;
 };

void *sfile_open(CSOUND *csound, const char *path, int32_t mode,
                 SFLIB_INFO *sfinfo) {
  FILE *fp = fopen(path, mode == SFM_READ ? "rb" : "wb");
  if (fp != NULL) {
      sfile *file = (sfile *) csound->Calloc(csound, sizeof(sfile));
      file->sfinfo = (SFLIB_INFO *) csound->Calloc(csound, sizeof(SFLIB_INFO));
      sfinfo->samplerate =
        file->sfinfo->samplerate = (int32_t) csoundGetSr(csound);
      sfinfo->channels =
      file->sfinfo->channels = (int32_t) csoundGetChannels(csound, 0);
      file->fp = fp;
      return file;
   }
  else return NULL;
}

void *sfile_open_fd(CSOUND *csound, int32_t fd, int32_t mode, SFLIB_INFO *sfinfo,
                    int32_t close_desc) {
  FILE *fp = fdopen(fd, mode == SFM_READ ? "rb" : "wb");
  if (fp != NULL) {
      sfile *file = (sfile *) csound->Calloc(csound, sizeof(sfile));
      file->fp = fp;
      file->sfinfo = (SFLIB_INFO *) csound->Calloc(csound, sizeof(SFLIB_INFO));
      sfinfo->samplerate =
        file->sfinfo->samplerate = (int32_t) csoundGetSr(csound);
      sfinfo->channels =
      file->sfinfo->channels = (int32_t) csoundGetChannels(csound, 0); 
      return file;
   }
  else return NULL;

}

int32_t sfile_close(CSOUND *csound, void *p) {
    sfile *file = (sfile *) p;
    fclose(file->fp);
    csound->Free(csound, file->sfinfo);
    csound->Free(csound, file);
    return CSOUND_SUCCESS;
}

int64_t sfile_write(CSOUND *csound, void *p, MYFLT *data, int64_t frames) {
  sfile *file = (sfile *) p;
  return fwrite(data, sizeof(MYFLT)*file->sfinfo->channels, frames, file->fp);
}

int64_t sfile_read(CSOUND *csound, void *p, MYFLT *data, int64_t frames) {
  sfile *file = (sfile *) p;
  return fread(data, sizeof(MYFLT)*file->sfinfo->channels, frames, file->fp);
}

int64_t sfile_write_samples(CSOUND *csound, void *p, MYFLT *data, int64_t samples) {
  sfile *file = (sfile *) p;
  return fwrite(data, sizeof(MYFLT), samples, file->fp);
}

int64_t sfile_read_samples(CSOUND *csound, void *p, MYFLT *data, int64_t samples) {
  sfile *file = (sfile *) p;
  return fread(data, sizeof(MYFLT), samples, file->fp);
}

int64_t sfile_seek(CSOUND *csound, void *p, int64_t offs, int32_t whence) {
   sfile *file = (sfile *) p;
   return fseek(file->fp, offs, whence); 
}

int32_t sfile_set_string(CSOUND *csound, void *sndfile, int32_t str_type, const char* str){
  return CSOUND_SUCCESS;
}

const char *sfile_str_error(CSOUND *csound, void *){
  return "error msg: not implemented";
}

int32_t sfile_command(CSOUND *csound, void *p, int32_t flag, void *sf, int32_t n) {
  return CSOUND_SUCCESS;
}

SNDFILE_CALLBACKS *sfile_setup(CSOUND *csound) {
      SNDFILE_CALLBACKS *sfcbs = (SNDFILE_CALLBACKS *)
        csound->Calloc(csound, sizeof(SNDFILE_CALLBACKS));
      sfcbs->sndfileOpen = sfile_open;
      sfcbs->sndfileOpenFd = sfile_open_fd; 
      sfcbs->sndfileClose = sfile_close;
      sfcbs->sndfileWrite = sfile_write;
      sfcbs->sndfileRead = sfile_read;
      sfcbs->sndfileWriteSamples = sfile_write_samples;
      sfcbs->sndfileReadSamples = sfile_read_samples;
      sfcbs->sndfileSeek = sfile_seek;
      sfcbs->sndfileSetString = sfile_set_string;
      sfcbs->sndfileStrError = sfile_str_error;
      sfcbs->sndfileCommand = sfile_command;
      return sfcbs;
}

TEST_F (SndfileTests, testWriteSndfile)
{
  int32_t result;
  const char* instrument =
        "instr 1 \n"
        "a1 oscili p4, p5 \n"
        "out linen(a1,0.1,p3,0.1)   \n"
        "endin \n";
  
  SNDFILE_CALLBACKS *sfcbs = sfile_setup(csound);   
  csoundSetSndfileCallbacks(csound, sfcbs);
  csoundSetOption (csound, "--format=raw --format=double -o test.raw");
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1 0.5 440 \n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);

}

TEST_F (SndfileTests, testReadSndfile)
{
  int32_t result;
  const char* instrument =
        "0dbfs = 1\n"
        "instr 1 \n"
        "a1 inch 1 \n"
        "out a1  \n"
        "endin \n";
  
  SNDFILE_CALLBACKS *sfcbs = sfile_setup(csound); 
  csoundSetSndfileCallbacks(csound, sfcbs);
  csoundSetOption (csound, "--format=raw --format=double -itest.raw -odac");
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1\n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  while(!result)
    result = csoundPerformKsmps(csound);

}
