/*
 * File:   csound_test_sndfile.cpp
 * Author: vlazzarini
 *
 * Created on Nov 17, 2024
 */

#include "gtest/gtest.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
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
#ifdef _WIN32
        const auto pid = _getpid();
#else
        const auto pid = getpid();
#endif
        path = std::filesystem::temp_directory_path() /
          ("csound-sndfile-" + std::to_string(pid) + "-" +
           std::to_string(reinterpret_cast<uintptr_t>(csound)) + ".raw");
    }

    virtual void TearDown ()
    {
        csoundDestroy (csound);
        csound = nullptr;
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    CSOUND* csound {nullptr};
    std::filesystem::path path;
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
      sfcbs->SndfileOpen = sfile_open;
      sfcbs->SndfileOpenFd = sfile_open_fd; 
      sfcbs->SndfileClose = sfile_close;
      sfcbs->SndfileWrite = sfile_write;
      sfcbs->SndfileRead = sfile_read;
      sfcbs->SndfileWriteSamples = sfile_write_samples;
      sfcbs->SndfileReadSamples = sfile_read_samples;
      sfcbs->SndfileSeek = sfile_seek;
      sfcbs->SndfileSetString = sfile_set_string;
      sfcbs->SndfileStrError = sfile_str_error;
      sfcbs->SndfileCommand = sfile_command;
      return sfcbs;
}

TEST_F (SndfileTests, testFileErrorHandle)
{
  int token;
  void *soundfile = &token;
  auto savedError = csound->SndfileStrError;
  auto savedClose = csound->SndfileClose;
  csoundSetHostData(csound, soundfile);
  csound->SndfileStrError = [](CSOUND *cs, void *handle) -> const char * {
    EXPECT_EQ(handle, csoundGetHostData(cs));
    return "soundfile error";
  };
  csound->SndfileClose = [](CSOUND *cs, void *handle) -> int32_t {
    EXPECT_EQ(handle, csoundGetHostData(cs));
    return 0;
  };

  for (int type : {CSFILE_SND_R, CSFILE_SND_W}) {
    void *file = csound->CreateFileHandle(csound, &soundfile, type, "callback-test");
    ASSERT_NE(file, nullptr);
    EXPECT_STREQ(csound->FileError(csound, file), "soundfile error");
    EXPECT_EQ(csound->FileClose(csound, file, CSFILE_CLOSE_SYNC), 0);
  }

  csound->SndfileStrError = savedError;
  csound->SndfileClose = savedClose;
  csoundSetHostData(csound, nullptr);
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
  const std::string options = "--format=raw --format=double -o \"" +
                              path.generic_string() + "\"";
  ASSERT_EQ(csoundSetOption(csound, options.c_str()), 0);
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
  const std::vector<MYFLT> samples(44100, MYFLT(0.25));
  std::ofstream input(path, std::ios::binary);
  ASSERT_TRUE(input.is_open());
  input.write(reinterpret_cast<const char *>(samples.data()),
              samples.size() * sizeof(MYFLT));
  input.close();
  ASSERT_TRUE(input.good());

  int32_t result;
  const char* instrument =
        "0dbfs = 1\n"
        "instr 1 \n"
        "a1 inch 1 \n"
        "out a1  \n"
        "endin \n";
  
  SNDFILE_CALLBACKS *sfcbs = sfile_setup(csound); 
  csoundSetSndfileCallbacks(csound, sfcbs);
  const std::string options = "--format=raw --format=double -n -i \"" +
                              path.generic_string() + "\"";
  ASSERT_EQ(csoundSetOption(csound, options.c_str()), 0);
  result = csoundCompileOrc(csound, instrument, 0);
  ASSERT_TRUE (result == 0);
  csoundEventString(csound,  "i 1 0 1\n", 0);
  result = csoundStart(csound);
  ASSERT_TRUE (result == 0);
  bool heardInput = false;
  while(!result) {
    result = csoundPerformKsmps(csound);
    const MYFLT *output = csoundGetSpout(csound);
    for (uint32_t i = 0; i < csoundGetKsmps(csound); ++i)
      heardInput |= output[i] == MYFLT(0.25);
  }
  EXPECT_TRUE(heardInput);

}
