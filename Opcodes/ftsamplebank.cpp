/*
    This file is part of Csound.

        Copyright (C) 2014 Rory Walsh

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
 */

#ifdef HAVE_DIRENT_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "OpcodeBase.hpp"
#include <dirent.h>
#include <sys/types.h>

using namespace std;
using namespace csound;

/* this function will load all samples of supported types into function
   tables number 'index' and upwards.
   It return the number of samples loaded */
int32_t loadSamplesToTables(CSOUND *csound, int32_t index, char *directory,
                        int32_t skiptime, int32_t format, int32_t channel);

//-----------------------------------------------------------------
//      i-rate class
//-----------------------------------------------------------------
class iftsamplebank : public OpcodeBase<iftsamplebank> {
public:
  // Outputs.
  MYFLT *numberOfFiles;
  // Inputs.
  STRINGDAT *sDirectory;
  MYFLT *index;
  //    MYFLT* trigger;
  MYFLT *skiptime;
  MYFLT *format;
  MYFLT *channel;

  iftsamplebank() {
    channel = 0;
    index = 0;
    skiptime = 0;
    format = 0;
    index = 0;
    numberOfFiles = 0;
    sDirectory = NULL;
  }

  // init-pass
  int32_t init(CSOUND *csound) {

    *numberOfFiles = loadSamplesToTables(
        csound, *index, (char *)sDirectory->data, *skiptime, *format, *channel);
    return OK;
  }

  int32_t noteoff(CSOUND *) { return OK; }
};

//-----------------------------------------------------------------
//      k-rate class
//-----------------------------------------------------------------
class kftsamplebank : public OpcodeBase<kftsamplebank> {
public:
  // Outputs.
  MYFLT *numberOfFiles;
  // Inputs.
  STRINGDAT *sDirectory;
  MYFLT *index;
  MYFLT *trigger;
  MYFLT *skiptime;
  MYFLT *format;
  MYFLT *channel;
  int32_t internalCounter;
  kftsamplebank() : internalCounter(0) {
    channel = 0;
    index = 0;
    skiptime = 0;
    format = 0;
    index = 0;
    trigger = 0;
  }

  // init-pass
  int32_t init(CSOUND *csound) {
    IGN(csound);
    *numberOfFiles =
          loadSamplesToTables(csound, *index, (char *)sDirectory->data,
                              *skiptime, *format, *channel);
    *trigger = 0;
    return OK;
  }

  int32_t noteoff(CSOUND *) { return OK; }

  int32_t kontrol(CSOUND *csound) {
    // if directry changes update tables..
    if (*trigger == 1) {
      *numberOfFiles =
          loadSamplesToTables(csound, *index, (char *)sDirectory->data,
                              *skiptime, *format, *channel);
      *trigger = 0;
    }
    return OK;
  }
};

//-----------------------------------------------------------------
//      load samples into function tables
//-----------------------------------------------------------------
int32_t loadSamplesToTables(CSOUND *csound, int32_t index, char *directory,
                        int32_t skiptime, int32_t format, int32_t channel) {

  if (directory) {
    DIR *dir = opendir(directory);
    std::vector<std::string> fileNames;
    std::vector<std::string> fileExtensions;
    int32_t noOfFiles = 0;
    fileExtensions.push_back(".wav");
    fileExtensions.push_back(".aiff");
    fileExtensions.push_back(".ogg");
    fileExtensions.push_back(".flac");

    // check for valid path first
    if (dir) {
      struct dirent *ent;
      while ((ent = readdir(dir)) != NULL) {
        std::ostringstream fullFileName;
        // only use supported file types
        for (int32_t i = 0; (size_t)i < fileExtensions.size(); i++)
        {
          std::string fname = ent->d_name;
          std::string extension;
          if(fname.find_last_of(".") != std::string::npos)
            extension = fname.substr(fname.find_last_of("."));

          if(extension == fileExtensions[i])
            {
              if (strlen(directory) > 0) {
#if defined(WIN32)
              fullFileName << directory << "\\" << ent->d_name;
#else
              fullFileName << directory << "/" << ent->d_name;
#endif
            }
            else
              fullFileName << ent->d_name;

            noOfFiles++;

            fileNames.push_back(fullFileName.str());
          }
        }
      }

      // Sort names
      std::sort(fileNames.begin(), fileNames.end());

      // push statements to score, starting with table number 'index'
      for (int32_t y = 0; (size_t)y < fileNames.size(); y++) {
        std::ostringstream statement;
        statement << "f" << index + y << " 0 0 1 \"" << fileNames[y] << "\" "
                  << skiptime << " " << format << " " << channel << "\n";
        // csound->MessageS(csound, CSOUNDMSG_ORCH, statement.str().c_str());
        csound->InputMessage(csound, statement.str().c_str());
      }

      closedir(dir);
    } else {
      csound->Message(csound,
                      Str("Cannot load file. Error opening directory: %s\n"),
                      directory);
    }

    // return number of files
    return noOfFiles;
  } else
    return 0;
}

typedef struct {
  OPDS h;
  ARRAYDAT *outArr;
  STRINGDAT *directoryName;
  MYFLT *extension;
} DIR_STRUCT;

/* this function will looks for files of a set type, in a particular directory
 */
std::vector<std::string> searchDir(CSOUND *csound, char *directory,
                                   char *extension);

#include "arrays.h"
#if 0
/* from Opcodes/arrays.c */
static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int32_t size) {
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==1 && p->sizes[0] < size)) {
      size_t ss;
      if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }
      ss = p->arrayMemberSize*size;
      if (p->data==NULL) {
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->allocated = ss;
      }
      else if (ss > p->allocated) {
        p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
        p->allocated = ss;
      }
      if (p->dimensions==0) {
        p->dimensions = 1;
        p->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
      }
    }
    p->sizes[0] = size;
}
#endif

static int32_t directory(CSOUND *csound, DIR_STRUCT *p) {
  int32_t inArgCount = p->INOCOUNT;
  char *extension, *file;
  std::vector<std::string> fileNames;

  if (inArgCount == 0)
    return csound->InitError(
        csound, "%s", Str("Error: you must pass a directory as a string."));

  if (inArgCount == 1) {
    fileNames = searchDir(csound, p->directoryName->data, (char *)"");
  }

  else if (inArgCount == 2) {
    CS_TYPE *argType = GetTypeForArg(p->extension);
    if (strcmp("S", argType->varTypeName) == 0) {
      extension = csound->Strdup(csound, ((STRINGDAT *)p->extension)->data);
      fileNames = searchDir(csound, p->directoryName->data, extension);
    } else
      return csound->InitError(csound,
                               "%s", Str("Error: second parameter to directory"
                                   " must be a string"));
  }

  int32_t numberOfFiles = (int32_t) fileNames.size();
  tabinit(csound, p->outArr, numberOfFiles, &(p->h));
  STRINGDAT *strings = (STRINGDAT *)p->outArr->data;

  for (int32_t i = 0; i < numberOfFiles; i++) {
    file = &fileNames[i][0u];
    strings[i].size = strlen(file) + 1;
    strings[i].data = csound->Strdup(csound, file);
  }

  fileNames.clear();

  return OK;
}

//-----------------------------------------------------------------
//      load samples into function tables
//-----------------------------------------------------------------
std::vector<std::string> searchDir(CSOUND *csound, char *directory,
                                   char *extension) {
  std::vector<std::string> fileNames;
  if (directory) {
    DIR *dir = opendir(directory);
    std::string fileExtension(extension);
    //    int32_t noOfFiles = 0;

    // check for valid path first
    if (dir) {
      struct dirent *ent;
      while ((ent = readdir(dir)) != NULL) {
        std::ostringstream fullFileName;

        std::string fname = ent->d_name;
        size_t lastPos = fname.find_last_of(".");
        if (fname.length() > 0 && (fileExtension.empty() ||
            (lastPos != std::string::npos &&
            fname.substr(lastPos) == fileExtension))) {
          if (strlen(directory) > 0) {
#if defined(WIN32)
            fullFileName << directory << "\\" << ent->d_name;
#else
            fullFileName << directory << "/" << ent->d_name;
#endif
          } else
            fullFileName << ent->d_name;

          //noOfFiles++;
          fileNames.push_back(fullFileName.str());
        }
      }

      // Sort names
      std::sort(fileNames.begin(), fileNames.end());
    } else {
      csound->Message(csound, Str("Cannot find directory. "
                                  "Error opening directory: %s\n"),
                      directory);
    }
    closedir(dir);
  }

  return fileNames;
}

extern "C" {

PUBLIC int32_t csoundModuleInit_ftsamplebank(CSOUND *csound) {

  int32_t status = csound->AppendOpcode(
      csound, (char *)"ftsamplebank.k", sizeof(kftsamplebank), 0,
      (char *)"k", (char *)"Skkkkk",
      (int32_t (*)(CSOUND *, void *))kftsamplebank::init_,
      (int32_t (*)(CSOUND *, void *))kftsamplebank::kontrol_,
      (int32_t (*)(CSOUND *, void *))0);

  status |= csound->AppendOpcode(
      csound, (char *)"ftsamplebank.i", sizeof(iftsamplebank), 0, 
      (char *)"i", (char *)"Siiii",
      (int32_t (*)(CSOUND *, void *))iftsamplebank::init_,
      (int32_t (*)(CSOUND *, void *))0, (int32_t (*)(CSOUND *, void *))0);

  /*  status |= csound->AppendOpcode(csound,
      (char*)"ftsamplebank",
      0xffff,
      0,
      0,
      0,
      0,
      0,
      0,
      0); */

  status |= csound->AppendOpcode(
      csound, (char *)"directory", sizeof(DIR_STRUCT), 0, (char *)"S[]",
      (char *)"SN", (int32_t (*)(CSOUND *, void *))directory,
      (int32_t (*)(CSOUND *, void *))0, (int32_t (*)(CSOUND *, void *))0);
  return status;
}

#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleCreate(CSOUND *csound) {
  IGN(csound);
  return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound) {
  return csoundModuleInit_ftsamplebank(csound);
}

PUBLIC int32_t csoundModuleDestroy(CSOUND *csound) {
  IGN(csound);
  return 0;
}
#endif
}
#endif
