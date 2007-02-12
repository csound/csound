/*
    ConfigFile.cpp:
    Copyright (C) 2006 Istvan Varga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "CsoundGUI.hpp"

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#  include <direct.h>
#  include <windows.h>
#endif

using namespace std;
// md5sum of "Csound5GUIConfig"

static const unsigned char hdrMagic[16] = {
    0xB4, 0x77, 0xBD, 0x68, 0xD7, 0xCD, 0x96, 0xD2,
    0xD5, 0x33, 0xA3, 0x23, 0xFA, 0x00, 0x56, 0xB7
};

static const int endianTest = 1;

// ----------------------------------------------------------------------------

static int writeInt_(FILE *f, int n)
{
    int           retval = 0;
    unsigned int  x = (unsigned int) n;

    retval |= fputc((int) ((x & 0xFF000000U) >> 24), f);
    retval |= fputc((int) ((x & 0x00FF0000U) >> 16), f);
    retval |= fputc((int) ((x & 0x0000FF00U) >> 8), f);
    retval |= fputc((int) (x & 0x000000FFU), f);

    return (retval < 0 ? -1 : 0);
}

static int writeBool(FILE *f, const bool *p)
{
    int     retval = 0;

    retval |= writeInt_(f, 1);
    retval |= fputc((*p ? 1 : 0), f);

    return (retval < 0 ? -1 : 0);
}

static int writeInt(FILE *f, const int *p)
{
    int     retval = 0;

    retval |= writeInt_(f, 4);
    retval |= writeInt_(f, *p);

    return (retval < 0 ? -1 : 0);
}

static int writeDouble(FILE *f, const double *p)
{
    int     retval = 0;
    int     i;

    retval |= writeInt_(f, (int) sizeof(double));
    if (*((char*) &endianTest) == (char) 0) {
      for (i = 0; i < (int) sizeof(double); i++)
        retval |= fputc((int) ((const unsigned char*) p)[i], f);
    }
    else {
      for (i = (int) sizeof(double) - 1; i >= 0; i--)
        retval |= fputc((int) ((const unsigned char*) p)[i], f);
    }

    return (retval < 0 ? -1 : 0);
}

static int writeString(FILE *f, string& p)
{
    int     retval = 0;
    int     len;

    len = (int) p.size() + 1;
    retval |= writeInt_(f, len);
    if ((int) fwrite((const void*) p.c_str(), 1, (size_t) len, f) != len)
      retval = -1;

    return (retval < 0 ? -1 : 0);
}

// ----------------------------------------------------------------------------

static int readInt_(FILE *f, int *n)
{
    int           c;
    unsigned int  x;

    *n = 0;
    c = fgetc(f);
    if (c < 0)
      return -1;
    x = (unsigned int) c & 0xFFU;
    c = fgetc(f);
    if (c < 0)
      return -1;
    x = (x << 8) | ((unsigned int) c & 0xFFU);
    c = fgetc(f);
    if (c < 0)
      return -1;
    x = (x << 8) | ((unsigned int) c & 0xFFU);
    c = fgetc(f);
    if (c < 0)
      return -1;
    x = (x << 8) | ((unsigned int) c & 0xFFU);
    *n = (int) (x & 0x7FFFFFFFU);
    if (x & 0x80000000U) {
      *n -= (int) 0x7FFFFFFF;
      (*n)--;
    }

    return 0;
}

static int readBool(FILE *f, bool *p)
{
    int     n;

    *p = false;
    if (readInt_(f, &n) != 0)
      return -1;
    if (n != 1)
      return -1;
    n = fgetc(f);
    if (n < 0)
      return -1;
    if (n > 0)
      *p = true;

    return 0;
}

static int readInt(FILE *f, int *p)
{
    int     n;

    *p = 0;
    if (readInt_(f, &n) != 0)
      return -1;
    if (n != 4)
      return -1;
    return readInt_(f, p);
}

static int readDouble(FILE *f, double *p)
{
    int     n;
    int     i;

    *p = 0.0;
    if (readInt_(f, &n) != 0)
      return -1;
    if (n != (int) sizeof(double))
      return -1;
    if (*((char*) &endianTest) == (char) 0) {
      for (i = 0; i < (int) sizeof(double); i++) {
        int c = fgetc(f);
        if (c < 0)
          return -1;
        ((unsigned char*) p)[i] = (unsigned char) c;
      }
    }
    else {
      for (i = (int) sizeof(double) - 1; i >= 0; i--) {
        int c = fgetc(f);
        if (c < 0)
          return -1;
        ((unsigned char*) p)[i] = (unsigned char) c;
      }
    }

    return 0;
}

static int readString(FILE *f, string& p)
{
    char    buf[128];
    int     len, cnt;

    p = "";
    if (readInt_(f, &len) != 0)
      return -1;
    if (len < 1 || len > 1000000)
      return -1;
    cnt = 0;
    for (int i = 0; i < (len - 1); i++) {
      int   c = fgetc(f);
      if (c < 0)
        return -1;
      buf[cnt++] = (char) c;
      if (cnt >= 127) {
        buf[cnt] = (char) 0;
        p += &(buf[0]);
        cnt = 0;
      }
    }
    if (cnt) {
      buf[cnt] = (char) 0;
      p += &(buf[0]);
    }
    if (fgetc(f) != 0)
      return -1;

    return 0;
}

// ----------------------------------------------------------------------------

static void getFullPathFileName(const char *fileName, string& fullName)
{
    string dirName;

    dirName = "";
#ifndef WIN32
    if (getenv("HOME") != (char*) 0)
      dirName = getenv("HOME");
    if ((int) dirName.size() == 0)
      dirName = ".";
    mkdir(dirName.c_str(), 0700);
    if (dirName[dirName.size() - 1] != '/')
      dirName += '/';
    dirName += ".csound";
    mkdir(dirName.c_str(), 0700);
#else
    CsoundGUIMain::stripString(dirName, getenv("USERPROFILE"));
    if ((int) dirName.size() != 0) {
      struct _stat tmp;
      memset(&tmp, 0, sizeof(struct _stat));
      if (dirName[dirName.size() - 1] != '\\')
        dirName += '\\';
      dirName += "Application Data";
      if (_stat(dirName.c_str(), &tmp) != 0 ||
          !(tmp.st_mode & _S_IFDIR))
        dirName = "";
    }
    if ((int) dirName.size() == 0) {
      CsoundGUIMain::stripString(dirName, getenv("HOME"));
      if ((int) dirName.size() != 0) {
        struct _stat tmp;
        memset(&tmp, 0, sizeof(struct _stat));
        if (_stat(dirName.c_str(), &tmp) != 0 ||
            !(tmp.st_mode & _S_IFDIR))
          dirName = "";
      }
    }
    if ((int) dirName.size() == 0) {
      char  buf[512];
      int   len;
      len = (int) GetModuleFileName((HMODULE) 0, &(buf[0]), (DWORD) 512);
      if (len >= 512)
        len = 0;
      while (len > 0) {
        len--;
        if (buf[len] == '\\') {
          buf[len] = (char) 0;
          break;
        }
      }
      if (len > 0)
        dirName = &(buf[0]);
      else
        dirName = ".";
    }
    if (dirName[dirName.size() - 1] != '\\')
      dirName += '\\';
    dirName += ".csound";
    _mkdir(dirName.c_str());
#endif
    fullName = dirName;
#ifndef WIN32
    fullName += '/';
#else
    fullName += '\\';
#endif
    fullName += fileName;
}

int writeCsound5GUIConfigFile(const char *fileName, CsoundUtilitySettings& cfg)
{
    string fullName;
    FILE        *f;
    int         err = 0;

    getFullPathFileName(fileName, fullName);
    if (remove(fullName.c_str()) != 0) {
      if (errno != ENOENT)
        return -1;
    }
    f = fopen(fullName.c_str(), "wb");
    if (f == NULL)
      return -1;
    if ((int) fwrite((const void*) &(hdrMagic[0]), 1, 16, f) != 16)
      err = -1;

    err |= writeInt_(f, 20001);
    err |= writeBool(f, &(cfg.listOpcodes_printDetails));

    err |= writeInt_(f, 20101);
    err |= writeString(f, cfg.cvanal_inputFile);
    err |= writeInt_(f, 20102);
    err |= writeString(f, cfg.cvanal_outputFile);
    err |= writeInt_(f, 20103);
    err |= writeInt(f, &(cfg.cvanal_channel));
    err |= writeInt_(f, 20104);
    err |= writeDouble(f, &(cfg.cvanal_beginTime));
    err |= writeInt_(f, 20105);
    err |= writeDouble(f, &(cfg.cvanal_duration));

    err |= writeInt_(f, 20201);
    err |= writeString(f, cfg.pvanal_inputFile);
    err |= writeInt_(f, 20202);
    err |= writeString(f, cfg.pvanal_outputFile);
    err |= writeInt_(f, 20203);
    err |= writeInt(f, &(cfg.pvanal_channel));
    err |= writeInt_(f, 20204);
    err |= writeDouble(f, &(cfg.pvanal_beginTime));
    err |= writeInt_(f, 20205);
    err |= writeDouble(f, &(cfg.pvanal_duration));
    err |= writeInt_(f, 20206);
    err |= writeInt(f, &(cfg.pvanal_frameSize));
    err |= writeInt_(f, 20207);
    err |= writeInt(f, &(cfg.pvanal_overlap));
    err |= writeInt_(f, 20208);
    err |= writeInt(f, &(cfg.pvanal_hopSize));
    err |= writeInt_(f, 20209);
    err |= writeInt(f, &(cfg.pvanal_windowType));

    err |= writeInt_(f, 20301);
    err |= writeString(f, cfg.hetro_inputFile);
    err |= writeInt_(f, 20302);
    err |= writeString(f, cfg.hetro_outputFile);
    err |= writeInt_(f, 20303);
    err |= writeInt(f, &(cfg.hetro_channel));
    err |= writeInt_(f, 20304);
    err |= writeDouble(f, &(cfg.hetro_beginTime));
    err |= writeInt_(f, 20305);
    err |= writeDouble(f, &(cfg.hetro_duration));
    err |= writeInt_(f, 20306);
    err |= writeDouble(f, &(cfg.hetro_startFreq));
    err |= writeInt_(f, 20307);
    err |= writeInt(f, &(cfg.hetro_partials));
    err |= writeInt_(f, 20308);
    err |= writeDouble(f, &(cfg.hetro_maxAmp));
    err |= writeInt_(f, 20309);
    err |= writeDouble(f, &(cfg.hetro_minAmp));
    err |= writeInt_(f, 20310);
    err |= writeInt(f, &(cfg.hetro_breakPoints));
    err |= writeInt_(f, 20311);
    err |= writeDouble(f, &(cfg.hetro_cutoffFreq));

    err |= writeInt_(f, 20401);
    err |= writeString(f, cfg.lpanal_inputFile);
    err |= writeInt_(f, 20402);
    err |= writeString(f, cfg.lpanal_outputFile);
    err |= writeInt_(f, 20403);
    err |= writeInt(f, &(cfg.lpanal_channel));
    err |= writeInt_(f, 20404);
    err |= writeDouble(f, &(cfg.lpanal_beginTime));
    err |= writeInt_(f, 20405);
    err |= writeDouble(f, &(cfg.lpanal_duration));
    err |= writeInt_(f, 20406);
    err |= writeBool(f, &(cfg.lpanal_altMode));
    err |= writeInt_(f, 20407);
    err |= writeInt(f, &(cfg.lpanal_poles));
    err |= writeInt_(f, 20408);
    err |= writeInt(f, &(cfg.lpanal_hopSize));
    err |= writeInt_(f, 20409);
    err |= writeString(f, cfg.lpanal_comment);
    err |= writeInt_(f, 20410);
    err |= writeDouble(f, &(cfg.lpanal_minFreq));
    err |= writeInt_(f, 20411);
    err |= writeDouble(f, &(cfg.lpanal_maxFreq));
    err |= writeInt_(f, 20412);
    err |= writeInt(f, &(cfg.lpanal_verbosity));

    err |= writeInt_(f, 20501);
    err |= writeString(f, cfg.sndinfo_inputFile);

    err |= writeInt_(f, 20601);
    err |= writeString(f, cfg.srconv_inputFile);
    err |= writeInt_(f, 20602);
    err |= writeString(f, cfg.srconv_outputFile);
    err |= writeInt_(f, 20603);
    err |= writeDouble(f, &(cfg.srconv_pitchRatio));
    err |= writeInt_(f, 20604);
    err |= writeDouble(f, &(cfg.srconv_sampleRate));
    err |= writeInt_(f, 20605);
    err |= writeInt(f, &(cfg.srconv_quality));
    err |= writeInt_(f, 20606);
    err |= writeInt(f, &(cfg.srconv_fileType));
    err |= writeInt_(f, 20607);
    err |= writeInt(f, &(cfg.srconv_sampleFormat));
    err |= writeInt_(f, 20608);
    err |= writeBool(f, &(cfg.srconv_peakChunks));
    err |= writeInt_(f, 20609);
    err |= writeBool(f, &(cfg.srconv_rewriteHeader));
    err |= writeInt_(f, 20610);
    err |= writeInt(f, &(cfg.srconv_heartBeat));

    err |= writeInt_(f, 20701);
    err |= writeString(f, cfg.dnoise_inputFile);
    err |= writeInt_(f, 20702);
    err |= writeString(f, cfg.dnoise_outputFile);
    err |= writeInt_(f, 20703);
    err |= writeString(f, cfg.dnoise_noiseFile);
    err |= writeInt_(f, 20704);
    err |= writeDouble(f, &(cfg.dnoise_beginTime));
    err |= writeInt_(f, 20705);
    err |= writeDouble(f, &(cfg.dnoise_endTime));
    err |= writeInt_(f, 20706);
    err |= writeInt(f, &(cfg.dnoise_fftSize));
    err |= writeInt_(f, 20707);
    err |= writeInt(f, &(cfg.dnoise_overlap));
    err |= writeInt_(f, 20708);
    err |= writeInt(f, &(cfg.dnoise_synLen));
    err |= writeInt_(f, 20709);
    err |= writeInt(f, &(cfg.dnoise_decFact));
    err |= writeInt_(f, 20710);
    err |= writeDouble(f, &(cfg.dnoise_threshold));
    err |= writeInt_(f, 20711);
    err |= writeInt(f, &(cfg.dnoise_sharpness));
    err |= writeInt_(f, 20712);
    err |= writeInt(f, &(cfg.dnoise_numFFT));
    err |= writeInt_(f, 20713);
    err |= writeDouble(f, &(cfg.dnoise_minGain));
    err |= writeInt_(f, 20714);
    err |= writeInt(f, &(cfg.dnoise_fileType));
    err |= writeInt_(f, 20715);
    err |= writeInt(f, &(cfg.dnoise_sampleFormat));
    err |= writeInt_(f, 20716);
    err |= writeInt(f, &(cfg.dnoise_heartBeat));
    err |= writeInt_(f, 20717);
    err |= writeBool(f, &(cfg.dnoise_rewriteHeader));
    err |= writeInt_(f, 20718);
    err |= writeBool(f, &(cfg.dnoise_verbose));

    err |= writeInt_(f, 0);
    fclose(f);

    return err;
}

int writeCsound5GUIConfigFile(const char *fileName, CsoundGlobalSettings& cfg)
{
    string fullName;
    FILE        *f;
    int         err = 0;

    getFullPathFileName(fileName, fullName);
    if (remove(fullName.c_str()) != 0) {
      if (errno != ENOENT)
        return -1;
    }
    f = fopen(fullName.c_str(), "wb");
    if (f == NULL)
      return -1;
    if ((int) fwrite((const void*) &(hdrMagic[0]), 1, 16, f) != 16)
      err = -1;
    err |= writeInt_(f, 10001);
    err |= writeString(f, cfg.textEditorProgram);
    err |= writeInt_(f, 10002);
    err |= writeString(f, cfg.soundEditorProgram);
    err |= writeInt_(f, 10003);
    err |= writeBool(f, &(cfg.forcePerformanceSettings));
    err |= writeInt_(f, 10004);
    err |= writeBool(f, &(cfg.editSoundFileAfterPerformance));
    err |= writeInt_(f, 10005);
    err |= writeString(f, cfg.helpBrowserProgram);
    err |= writeInt_(f, 10006);
    err |= writeBool(f, &(cfg.useBuiltInEditor));
    err |= writeInt_(f, 10101);
    err |= writeString(f, cfg.performanceSettings1_Name);
    err |= writeInt_(f, 10102);
    err |= writeString(f, cfg.performanceSettings2_Name);
    err |= writeInt_(f, 10103);
    err |= writeString(f, cfg.performanceSettings3_Name);
    err |= writeInt_(f, 10104);
    err |= writeString(f, cfg.performanceSettings4_Name);
    err |= writeInt_(f, 10105);
    err |= writeString(f, cfg.performanceSettings5_Name);
    err |= writeInt_(f, 10106);
    err |= writeString(f, cfg.performanceSettings6_Name);
    err |= writeInt_(f, 10107);
    err |= writeString(f, cfg.performanceSettings7_Name);
    err |= writeInt_(f, 10108);
    err |= writeString(f, cfg.performanceSettings8_Name);
    err |= writeInt_(f, 10109);
    err |= writeString(f, cfg.performanceSettings9_Name);
    err |= writeInt_(f, 10110);
    err |= writeString(f, cfg.performanceSettings10_Name);
    err |= writeInt_(f, 10200);
    err |= writeInt(f, &(cfg.guiPosX));
    err |= writeInt_(f, 10201);
    err |= writeInt(f, &(cfg.guiPosY));
    err |= writeInt_(f, 10202);
    err |= writeInt(f, &(cfg.orcEditorPosX));
    err |= writeInt_(f, 10203);
    err |= writeInt(f, &(cfg.orcEditorPosY));
    err |= writeInt_(f, 10204);
    err |= writeInt(f, &(cfg.orcEditorWidth));
    err |= writeInt_(f, 10205);
    err |= writeInt(f, &(cfg.orcEditorHeight));
    err |= writeInt_(f, 10206);
    err |= writeInt(f, &(cfg.scoEditorPosX));
    err |= writeInt_(f, 10207);
    err |= writeInt(f, &(cfg.scoEditorPosY));
    err |= writeInt_(f, 10208);
    err |= writeInt(f, &(cfg.scoEditorWidth));
    err |= writeInt_(f, 10209);
    err |= writeInt(f, &(cfg.scoEditorHeight));
    err |= writeInt_(f, 10210);
    err |= writeInt(f, &(cfg.consolePosX));
    err |= writeInt_(f, 10211);
    err |= writeInt(f, &(cfg.consolePosY));
    err |= writeInt_(f, 10212);
    err |= writeInt(f, &(cfg.consoleWidth));
    err |= writeInt_(f, 10213);
    err |= writeInt(f, &(cfg.consoleHeight));
    err |= writeInt_(f, 0);
    fclose(f);

    return err;
}

int writeCsound5GUIConfigFile(const char *fileName,
                              CsoundPerformanceSettings& cfg)
{
    string fullName;
    FILE        *f;
    int         err = 0;

    getFullPathFileName(fileName, fullName);
    if (remove(fullName.c_str()) != 0) {
      if (errno != ENOENT)
        return -1;
    }
    f = fopen(fullName.c_str(), "wb");
    if (f == NULL)
      return -1;
    if ((int) fwrite((const void*) &(hdrMagic[0]), 1, 16, f) != 16)
      err = -1;
    err |= writeInt_(f,  1); err |= writeString(f, cfg.orcName);
    err |= writeInt_(f,  2); err |= writeString(f, cfg.scoName);
    err |= writeInt_(f,  3); err |= writeString(f, cfg.soundFileType);
    err |= writeInt_(f,  4); err |= writeString(f, cfg.soundSampleFormat);
    err |= writeInt_(f,  5); err |= writeBool(f, &(cfg.enablePeakChunks));
    err |= writeInt_(f,  6); err |= writeInt(f, &(cfg.displayMode));
    err |= writeInt_(f,  7); err |= writeBool(f, &(cfg.deferGEN1));
    err |= writeInt_(f,  8); err |= writeInt(f, &(cfg.bufFrames_SW));
    err |= writeInt_(f,  9); err |= writeInt(f, &(cfg.nBuffers));
    err |= writeInt_(f, 10); err |= writeString(f, cfg.midiInFileName);
    err |= writeInt_(f, 11); err |= writeString(f, cfg.midiOutFileName);
    err |= writeInt_(f, 12); err |= writeString(f, cfg.midiInDevName);
    err |= writeInt_(f, 13); err |= writeString(f, cfg.midiOutDevName);
    err |= writeInt_(f, 14); err |= writeBool(f, &(cfg.terminateOnMidi));
    err |= writeInt_(f, 15); err |= writeInt(f, &(cfg.heartBeatMode));
    err |= writeInt_(f, 16); err |= writeBool(f, &(cfg.rewriteHeader));
    err |= writeInt_(f, 17); err |= writeString(f, cfg.inputFileName);
    err |= writeInt_(f, 18); err |= writeString(f, cfg.outputFileName);
    err |= writeInt_(f, 19); err |= writeBool(f, &(cfg.enableSoundOutput));
    err |= writeInt_(f, 20); err |= writeDouble(f, &(cfg.beatModeTempo));
    err |= writeInt_(f, 21); err |= writeBool(f, &(cfg.iTimeOnly));
    err |= writeInt_(f, 22); err |= writeDouble(f, &(cfg.sampleRateOverride));
    err |= writeInt_(f, 23); err |= writeDouble(f, &(cfg.controlRateOverride));
    err |= writeInt_(f, 24); err |= writeString(f, cfg.lineInput);
    err |= writeInt_(f, 25); err |= writeInt(f, &(cfg.messageLevel));
    err |= writeInt_(f, 26); err |= writeBool(f, &(cfg.enableExpressionOpt));
    err |= writeInt_(f, 27); err |= writeString(f, cfg.sadirPath);
    err |= writeInt_(f, 28); err |= writeString(f, cfg.ssdirPath);
    err |= writeInt_(f, 29); err |= writeString(f, cfg.sfdirPath);
    err |= writeInt_(f, 30); err |= writeString(f, cfg.incdirPath);
    err |= writeInt_(f, 52); err |= writeString(f, cfg.csdocdirPath);
    err |= writeInt_(f, 100); err |= writeString(f, cfg.strsets[0]);
    err |= writeInt_(f, 101); err |= writeString(f, cfg.strsets[1]);
    err |= writeInt_(f, 102); err |= writeString(f, cfg.strsets[2]);
    err |= writeInt_(f, 103); err |= writeString(f, cfg.strsets[3]);
    err |= writeInt_(f, 104); err |= writeString(f, cfg.strsets[4]);
    err |= writeInt_(f, 105); err |= writeString(f, cfg.strsets[5]);
    err |= writeInt_(f, 106); err |= writeString(f, cfg.strsets[6]);
    err |= writeInt_(f, 107); err |= writeString(f, cfg.strsets[7]);
    err |= writeInt_(f, 108); err |= writeString(f, cfg.strsets[8]);
    err |= writeInt_(f, 109); err |= writeString(f, cfg.strsets[9]);
    err |= writeInt_(f, 31); err |= writeBool(f, &(cfg.verbose));
    err |= writeInt_(f, 32); err |= writeBool(f, &(cfg.enableDither));
    err |= writeInt_(f, 33); err |= writeString(f, cfg.pluginLibs);
    err |= writeInt_(f, 34); err |= writeString(f, cfg.sndidArtist);
    err |= writeInt_(f, 35); err |= writeString(f, cfg.sndidComment);
    err |= writeInt_(f, 36); err |= writeString(f, cfg.sndidCopyright);
    err |= writeInt_(f, 37); err |= writeString(f, cfg.sndidDate);
    err |= writeInt_(f, 38); err |= writeString(f, cfg.sndidSoftware);
    err |= writeInt_(f, 39); err |= writeString(f, cfg.sndidTitle);
    err |= writeInt_(f, 40); err |= writeBool(f, &(cfg.ignoreCSDOptions));
    err |= writeInt_(f, 41); err |= writeString(f, cfg.jackClientName);
    err |= writeInt_(f, 42); err |= writeString(f, cfg.jackInPortName);
    err |= writeInt_(f, 43); err |= writeString(f, cfg.jackOutPortName);
    err |= writeInt_(f, 44); err |= writeInt(f, &(cfg.maxStrLen));
    err |= writeInt_(f, 45); err |= writeString(f, cfg.midiFileMuteTracks);
    err |= writeInt_(f, 150); err |= writeInt(f, &(cfg.midiKeyMidi));
    err |= writeInt_(f, 151); err |= writeInt(f, &(cfg.midiKeyCps));
    err |= writeInt_(f, 152); err |= writeInt(f, &(cfg.midiKeyOct));
    err |= writeInt_(f, 153); err |= writeInt(f, &(cfg.midiKeyPch));
    err |= writeInt_(f, 154); err |= writeInt(f, &(cfg.midiVelMidi));
    err |= writeInt_(f, 155); err |= writeInt(f, &(cfg.midiVelAmp));
    err |= writeInt_(f, 46); err |= writeBool(f, &(cfg.rawControllerMode));
    err |= writeInt_(f, 47); err |= writeString(f, cfg.rtAudioModule);
    err |= writeInt_(f, 54); err |= writeString(f, cfg.rtAudioOutputDevice);
    err |= writeInt_(f, 55); err |= writeString(f, cfg.rtAudioInputDevice);
    err |= writeInt_(f, 48); err |= writeString(f, cfg.rtMidiModule);
    err |= writeInt_(f, 49); err |= writeDouble(f, &(cfg.scoreOffsetSeconds));
    err |= writeInt_(f, 50); err |= writeBool(f, &(cfg.useThreads));
    err |= writeInt_(f, 51); err |= writeString(f, cfg.scriptFileName);
    err |= writeInt_(f, 53); err |= writeBool(f, &(cfg.runRealtime));
    err |= writeInt_(f, 56); err |= writeBool(f, &(cfg.disableDiskOutput));
    err |= writeInt_(f, 200); err |= writeString(f, cfg.additionalFlags);
    err |= writeInt_(f, 201); err |= writeBool(f, &(cfg.useAdditionalFlags));
    err |= writeInt_(f, 0);
    fclose(f);

    return err;
}

int readCsound5GUIConfigFile(const char *fileName, CsoundUtilitySettings& cfg)
{
    string fullName;
    FILE        *f;
    int         n;
    CsoundUtilitySettings   *tmp = (CsoundUtilitySettings*) 0;

    getFullPathFileName(fileName, fullName);
    f = fopen(fullName.c_str(), "rb");
    if (f == NULL)
      return -1;
    for (int i = 0; i < 16; i++) {
      int   c = fgetc(f);
      if (c < 0 || (unsigned char) c != hdrMagic[i])
        goto err_return;
    }
    tmp = new CsoundUtilitySettings();
    while (readInt_(f, &n) == 0) {
      switch (n) {
      case 0:
        if (readInt_(f, &n) == 0)
          goto err_return;
        cfg = *tmp;
        delete tmp;
        fclose(f);
        return 0;
      case 20001:
        if (readBool(f, &(tmp->listOpcodes_printDetails)) != 0)
          goto err_return;
        break;
      case 20101:
        if (readString(f, tmp->cvanal_inputFile) != 0)
          goto err_return;
        break;
      case 20102:
        if (readString(f, tmp->cvanal_outputFile) != 0)
          goto err_return;
        break;
      case 20103:
        if (readInt(f, &(tmp->cvanal_channel)) != 0)
          goto err_return;
        break;
      case 20104:
        if (readDouble(f, &(tmp->cvanal_beginTime)) != 0)
          goto err_return;
        break;
      case 20105:
        if (readDouble(f, &(tmp->cvanal_duration)) != 0)
          goto err_return;
        break;
      case 20201:
        if (readString(f, tmp->pvanal_inputFile) != 0)
          goto err_return;
        break;
      case 20202:
        if (readString(f, tmp->pvanal_outputFile) != 0)
          goto err_return;
        break;
      case 20203:
        if (readInt(f, &(tmp->pvanal_channel)) != 0)
          goto err_return;
        break;
      case 20204:
        if (readDouble(f, &(tmp->pvanal_beginTime)) != 0)
          goto err_return;
        break;
      case 20205:
        if (readDouble(f, &(tmp->pvanal_duration)) != 0)
          goto err_return;
        break;
      case 20206:
        if (readInt(f, &(tmp->pvanal_frameSize)) != 0)
          goto err_return;
        break;
      case 20207:
        if (readInt(f, &(tmp->pvanal_overlap)) != 0)
          goto err_return;
        break;
      case 20208:
        if (readInt(f, &(tmp->pvanal_hopSize)) != 0)
          goto err_return;
        break;
      case 20209:
        if (readInt(f, &(tmp->pvanal_windowType)) != 0)
          goto err_return;
        break;
      case 20301:
        if (readString(f, tmp->hetro_inputFile) != 0)
          goto err_return;
        break;
      case 20302:
        if (readString(f, tmp->hetro_outputFile) != 0)
          goto err_return;
        break;
      case 20303:
        if (readInt(f, &(tmp->hetro_channel)) != 0)
          goto err_return;
        break;
      case 20304:
        if (readDouble(f, &(tmp->hetro_beginTime)) != 0)
          goto err_return;
        break;
      case 20305:
        if (readDouble(f, &(tmp->hetro_duration)) != 0)
          goto err_return;
        break;
      case 20306:
        if (readDouble(f, &(tmp->hetro_startFreq)) != 0)
          goto err_return;
        break;
      case 20307:
        if (readInt(f, &(tmp->hetro_partials)) != 0)
          goto err_return;
        break;
      case 20308:
        if (readDouble(f, &(tmp->hetro_maxAmp)) != 0)
          goto err_return;
        break;
      case 20309:
        if (readDouble(f, &(tmp->hetro_minAmp)) != 0)
          goto err_return;
        break;
      case 20310:
        if (readInt(f, &(tmp->hetro_breakPoints)) != 0)
          goto err_return;
        break;
      case 20311:
        if (readDouble(f, &(tmp->hetro_cutoffFreq)) != 0)
          goto err_return;
        break;
      case 20401:
        if (readString(f, tmp->lpanal_inputFile) != 0)
          goto err_return;
        break;
      case 20402:
        if (readString(f, tmp->lpanal_outputFile) != 0)
          goto err_return;
        break;
      case 20403:
        if (readInt(f, &(tmp->lpanal_channel)) != 0)
          goto err_return;
        break;
      case 20404:
        if (readDouble(f, &(tmp->lpanal_beginTime)) != 0)
          goto err_return;
        break;
      case 20405:
        if (readDouble(f, &(tmp->lpanal_duration)) != 0)
          goto err_return;
        break;
      case 20406:
        if (readBool(f, &(tmp->lpanal_altMode)) != 0)
          goto err_return;
        break;
      case 20407:
        if (readInt(f, &(tmp->lpanal_poles)) != 0)
          goto err_return;
        break;
      case 20408:
        if (readInt(f, &(tmp->lpanal_hopSize)) != 0)
          goto err_return;
        break;
      case 20409:
        if (readString(f, tmp->lpanal_comment) != 0)
          goto err_return;
        break;
      case 20410:
        if (readDouble(f, &(tmp->lpanal_minFreq)) != 0)
          goto err_return;
        break;
      case 20411:
        if (readDouble(f, &(tmp->lpanal_maxFreq)) != 0)
          goto err_return;
        break;
      case 20412:
        if (readInt(f, &(tmp->lpanal_verbosity)) != 0)
          goto err_return;
        break;
      case 20501:
        if (readString(f, tmp->sndinfo_inputFile) != 0)
          goto err_return;
        break;
      case 20601:
        if (readString(f, tmp->srconv_inputFile) != 0)
          goto err_return;
        break;
      case 20602:
        if (readString(f, tmp->srconv_outputFile) != 0)
          goto err_return;
        break;
      case 20603:
        if (readDouble(f, &(tmp->srconv_pitchRatio)) != 0)
          goto err_return;
        break;
      case 20604:
        if (readDouble(f, &(tmp->srconv_sampleRate)) != 0)
          goto err_return;
        break;
      case 20605:
        if (readInt(f, &(tmp->srconv_quality)) != 0)
          goto err_return;
        break;
      case 20606:
        if (readInt(f, &(tmp->srconv_fileType)) != 0)
          goto err_return;
        break;
      case 20607:
        if (readInt(f, &(tmp->srconv_sampleFormat)) != 0)
          goto err_return;
        break;
      case 20608:
        if (readBool(f, &(tmp->srconv_peakChunks)) != 0)
          goto err_return;
        break;
      case 20609:
        if (readBool(f, &(tmp->srconv_rewriteHeader)) != 0)
          goto err_return;
        break;
      case 20610:
        if (readInt(f, &(tmp->srconv_heartBeat)) != 0)
          goto err_return;
        break;
      case 20701:
        if (readString(f, tmp->dnoise_inputFile) != 0)
          goto err_return;
        break;
      case 20702:
        if (readString(f, tmp->dnoise_outputFile) != 0)
          goto err_return;
        break;
      case 20703:
        if (readString(f, tmp->dnoise_noiseFile) != 0)
          goto err_return;
        break;
      case 20704:
        if (readDouble(f, &(tmp->dnoise_beginTime)) != 0)
          goto err_return;
        break;
      case 20705:
        if (readDouble(f, &(tmp->dnoise_endTime)) != 0)
          goto err_return;
        break;
      case 20706:
        if (readInt(f, &(tmp->dnoise_fftSize)) != 0)
          goto err_return;
        break;
      case 20707:
        if (readInt(f, &(tmp->dnoise_overlap)) != 0)
          goto err_return;
        break;
      case 20708:
        if (readInt(f, &(tmp->dnoise_synLen)) != 0)
          goto err_return;
        break;
      case 20709:
        if (readInt(f, &(tmp->dnoise_decFact)) != 0)
          goto err_return;
        break;
      case 20710:
        if (readDouble(f, &(tmp->dnoise_threshold)) != 0)
          goto err_return;
        break;
      case 20711:
        if (readInt(f, &(tmp->dnoise_sharpness)) != 0)
          goto err_return;
        break;
      case 20712:
        if (readInt(f, &(tmp->dnoise_numFFT)) != 0)
          goto err_return;
        break;
      case 20713:
        if (readDouble(f, &(tmp->dnoise_minGain)) != 0)
          goto err_return;
        break;
      case 20714:
        if (readInt(f, &(tmp->dnoise_fileType)) != 0)
          goto err_return;
        break;
      case 20715:
        if (readInt(f, &(tmp->dnoise_sampleFormat)) != 0)
          goto err_return;
        break;
      case 20716:
        if (readInt(f, &(tmp->dnoise_heartBeat)) != 0)
          goto err_return;
        break;
      case 20717:
        if (readBool(f, &(tmp->dnoise_rewriteHeader)) != 0)
          goto err_return;
        break;
      case 20718:
        if (readBool(f, &(tmp->dnoise_verbose)) != 0)
          goto err_return;
        break;
      default:
        goto err_return;
      }
    }

 err_return:
    fclose(f);
    if (tmp)
      delete tmp;
    return -1;
}

int readCsound5GUIConfigFile(const char *fileName, CsoundGlobalSettings& cfg)
{
    string fullName;
    FILE        *f;
    int         n;
    CsoundGlobalSettings    *tmp = (CsoundGlobalSettings*) 0;

    getFullPathFileName(fileName, fullName);
    f = fopen(fullName.c_str(), "rb");
    if (f == NULL)
      return -1;
    for (int i = 0; i < 16; i++) {
      int   c = fgetc(f);
      if (c < 0 || (unsigned char) c != hdrMagic[i])
        goto err_return;
    }
    tmp = new CsoundGlobalSettings();
    while (readInt_(f, &n) == 0) {
      switch (n) {
      case 0:
        if (readInt_(f, &n) == 0)
          goto err_return;
        cfg = *tmp;
        delete tmp;
        fclose(f);
        return 0;
      case 10001:
        if (readString(f, tmp->textEditorProgram) != 0)
          goto err_return;
        break;
      case 10002:
        if (readString(f, tmp->soundEditorProgram) != 0)
          goto err_return;
        break;
      case 10003:
        if (readBool(f, &(tmp->forcePerformanceSettings)) != 0)
          goto err_return;
        break;
      case 10004:
        if (readBool(f, &(tmp->editSoundFileAfterPerformance)) != 0)
          goto err_return;
        break;
      case 10005:
        if (readString(f, tmp->helpBrowserProgram) != 0)
          goto err_return;
        break;
      case 10006:
        if (readBool(f, &(tmp->useBuiltInEditor)) != 0)
          goto err_return;
        break;
      case 10101:
        if (readString(f, tmp->performanceSettings1_Name) != 0)
          goto err_return;
        break;
      case 10102:
        if (readString(f, tmp->performanceSettings2_Name) != 0)
          goto err_return;
        break;
      case 10103:
        if (readString(f, tmp->performanceSettings3_Name) != 0)
          goto err_return;
        break;
      case 10104:
        if (readString(f, tmp->performanceSettings4_Name) != 0)
          goto err_return;
        break;
      case 10105:
        if (readString(f, tmp->performanceSettings5_Name) != 0)
          goto err_return;
        break;
      case 10106:
        if (readString(f, tmp->performanceSettings6_Name) != 0)
          goto err_return;
        break;
      case 10107:
        if (readString(f, tmp->performanceSettings7_Name) != 0)
          goto err_return;
        break;
      case 10108:
        if (readString(f, tmp->performanceSettings8_Name) != 0)
          goto err_return;
        break;
      case 10109:
        if (readString(f, tmp->performanceSettings9_Name) != 0)
          goto err_return;
        break;
      case 10110:
        if (readString(f, tmp->performanceSettings10_Name) != 0)
          goto err_return;
        break;
      case 10200:
        if (readInt(f, &(tmp->guiPosX)) != 0)
          goto err_return;
        break;
      case 10201:
        if (readInt(f, &(tmp->guiPosY)) != 0)
          goto err_return;
        break;
      case 10202:
        if (readInt(f, &(tmp->orcEditorPosX)) != 0)
          goto err_return;
        break;
      case 10203:
        if (readInt(f, &(tmp->orcEditorPosX)) != 0)
          goto err_return;
        break;
      case 10204:
        if (readInt(f, &(tmp->orcEditorWidth)) != 0)
          goto err_return;
        break;
      case 10205:
        if (readInt(f, &(tmp->orcEditorHeight)) != 0)
          goto err_return;
        break;
      case 10206:
        if (readInt(f, &(tmp->scoEditorPosX)) != 0)
          goto err_return;
        break;
      case 10207:
        if (readInt(f, &(tmp->scoEditorPosX)) != 0)
          goto err_return;
        break;
      case 10208:
        if (readInt(f, &(tmp->scoEditorWidth)) != 0)
          goto err_return;
        break;
      case 10209:
        if (readInt(f, &(tmp->scoEditorHeight)) != 0)
          goto err_return;
        break;
      case 10210:
        if (readInt(f, &(tmp->consolePosX)) != 0)
          goto err_return;
        break;
      case 10211:
        if (readInt(f, &(tmp->consolePosY)) != 0)
          goto err_return;
        break;
      case 10212:
        if (readInt(f, &(tmp->consoleWidth)) != 0)
          goto err_return;
        break;
      case 10213:
        if (readInt(f, &(tmp->consoleHeight)) != 0)
          goto err_return;
        break;
      default:
        goto err_return;
      }
    }

 err_return:
    fclose(f);
    if (tmp)
      delete tmp;
    return -1;
}

int readCsound5GUIConfigFile(const char *fileName,
                             CsoundPerformanceSettings& cfg)
{
    string fullName;
    FILE        *f;
    int         n;
    CsoundPerformanceSettings   *tmp = (CsoundPerformanceSettings*) 0;

    getFullPathFileName(fileName, fullName);
    f = fopen(fullName.c_str(), "rb");
    if (f == NULL)
      return -1;
    for (int i = 0; i < 16; i++) {
      int   c = fgetc(f);
      if (c < 0 || (unsigned char) c != hdrMagic[i])
        goto err_return;
    }
    tmp = new CsoundPerformanceSettings();
    while (readInt_(f, &n) == 0) {
      switch (n) {
      case 0:
        if (readInt_(f, &n) == 0)
          goto err_return;
        cfg = *tmp;
        delete tmp;
        fclose(f);
        return 0;
      case 1:
        if (readString(f, tmp->orcName) != 0)
          goto err_return;
        break;
      case 2:
        if (readString(f, tmp->scoName) != 0)
          goto err_return;
        break;
      case 3:
        if (readString(f, tmp->soundFileType) != 0)
          goto err_return;
        break;
      case 4:
        if (readString(f, tmp->soundSampleFormat) != 0)
          goto err_return;
        break;
      case 5:
        if (readBool(f, &(tmp->enablePeakChunks)) != 0)
          goto err_return;
        break;
      case 6:
        if (readInt(f, &(tmp->displayMode)) != 0)
          goto err_return;
        break;
      case 7:
        if (readBool(f, &(tmp->deferGEN1)) != 0)
          goto err_return;
        break;
      case 8:
        if (readInt(f, &(tmp->bufFrames_SW)) != 0)
          goto err_return;
        break;
      case 9:
        if (readInt(f, &(tmp->nBuffers)) != 0)
          goto err_return;
        break;
      case 10:
        if (readString(f, tmp->midiInFileName) != 0)
          goto err_return;
        break;
      case 11:
        if (readString(f, tmp->midiOutFileName) != 0)
          goto err_return;
        break;
      case 12:
        if (readString(f, tmp->midiInDevName) != 0)
          goto err_return;
        break;
      case 13:
        if (readString(f, tmp->midiOutDevName) != 0)
          goto err_return;
        break;
      case 14:
        if (readBool(f, &(tmp->terminateOnMidi)) != 0)
          goto err_return;
        break;
      case 15:
        if (readInt(f, &(tmp->heartBeatMode)) != 0)
          goto err_return;
        break;
      case 16:
        if (readBool(f, &(tmp->rewriteHeader)) != 0)
          goto err_return;
        break;
      case 17:
        if (readString(f, tmp->inputFileName) != 0)
          goto err_return;
        break;
      case 18:
        if (readString(f, tmp->outputFileName) != 0)
          goto err_return;
        break;
      case 19:
        if (readBool(f, &(tmp->enableSoundOutput)) != 0)
          goto err_return;
        break;
      case 20:
        if (readDouble(f, &(tmp->beatModeTempo)) != 0)
          goto err_return;
        break;
      case 21:
        if (readBool(f, &(tmp->iTimeOnly)) != 0)
          goto err_return;
        break;
      case 22:
        if (readDouble(f, &(tmp->sampleRateOverride)) != 0)
          goto err_return;
        break;
      case 23:
        if (readDouble(f, &(tmp->controlRateOverride)) != 0)
          goto err_return;
        break;
      case 24:
        if (readString(f, tmp->lineInput) != 0)
          goto err_return;
        break;
      case 25:
        if (readInt(f, &(tmp->messageLevel)) != 0)
          goto err_return;
        break;
      case 26:
        if (readBool(f, &(tmp->enableExpressionOpt)) != 0)
          goto err_return;
        break;
      case 27:
        if (readString(f, tmp->sadirPath) != 0)
          goto err_return;
        break;
      case 28:
        if (readString(f, tmp->ssdirPath) != 0)
          goto err_return;
        break;
      case 29:
        if (readString(f, tmp->sfdirPath) != 0)
          goto err_return;
        break;
      case 30:
        if (readString(f, tmp->incdirPath) != 0)
          goto err_return;
        break;
      case 52:
        if (readString(f, tmp->csdocdirPath) != 0)
          goto err_return;
        break;
      case 100:
        if (readString(f, tmp->strsets[0]) != 0)
          goto err_return;
        break;
      case 101:
        if (readString(f, tmp->strsets[1]) != 0)
          goto err_return;
        break;
      case 102:
        if (readString(f, tmp->strsets[2]) != 0)
          goto err_return;
        break;
      case 103:
        if (readString(f, tmp->strsets[3]) != 0)
          goto err_return;
        break;
      case 104:
        if (readString(f, tmp->strsets[4]) != 0)
          goto err_return;
        break;
      case 105:
        if (readString(f, tmp->strsets[5]) != 0)
          goto err_return;
        break;
      case 106:
        if (readString(f, tmp->strsets[6]) != 0)
          goto err_return;
        break;
      case 107:
        if (readString(f, tmp->strsets[7]) != 0)
          goto err_return;
        break;
      case 108:
        if (readString(f, tmp->strsets[8]) != 0)
          goto err_return;
        break;
      case 109:
        if (readString(f, tmp->strsets[9]) != 0)
          goto err_return;
        break;
      case 31:
        if (readBool(f, &(tmp->verbose)) != 0)
          goto err_return;
        break;
      case 32:
        if (readBool(f, &(tmp->enableDither)) != 0)
          goto err_return;
        break;
      case 33:
        if (readString(f, tmp->pluginLibs) != 0)
          goto err_return;
        break;
      case 34:
        if (readString(f, tmp->sndidArtist) != 0)
          goto err_return;
        break;
      case 35:
        if (readString(f, tmp->sndidComment) != 0)
          goto err_return;
        break;
      case 36:
        if (readString(f, tmp->sndidCopyright) != 0)
          goto err_return;
        break;
      case 37:
        if (readString(f, tmp->sndidDate) != 0)
          goto err_return;
        break;
      case 38:
        if (readString(f, tmp->sndidSoftware) != 0)
          goto err_return;
        break;
      case 39:
        if (readString(f, tmp->sndidTitle) != 0)
          goto err_return;
        break;
      case 40:
        if (readBool(f, &(tmp->ignoreCSDOptions)) != 0)
          goto err_return;
        break;
      case 41:
        if (readString(f, tmp->jackClientName) != 0)
          goto err_return;
        break;
      case 42:
        if (readString(f, tmp->jackInPortName) != 0)
          goto err_return;
        break;
      case 43:
        if (readString(f, tmp->jackOutPortName) != 0)
          goto err_return;
        break;
      case 44:
        if (readInt(f, &(tmp->maxStrLen)) != 0)
          goto err_return;
        break;
      case 45:
        if (readString(f, tmp->midiFileMuteTracks) != 0)
          goto err_return;
        break;
      case 150:
        if (readInt(f, &(tmp->midiKeyMidi)) != 0)
          goto err_return;
        break;
      case 151:
        if (readInt(f, &(tmp->midiKeyCps)) != 0)
          goto err_return;
        break;
      case 152:
        if (readInt(f, &(tmp->midiKeyOct)) != 0)
          goto err_return;
        break;
      case 153:
        if (readInt(f, &(tmp->midiKeyPch)) != 0)
          goto err_return;
        break;
      case 154:
        if (readInt(f, &(tmp->midiVelMidi)) != 0)
          goto err_return;
        break;
      case 155:
        if (readInt(f, &(tmp->midiVelAmp)) != 0)
          goto err_return;
        break;
      case 46:
        if (readBool(f, &(tmp->rawControllerMode)) != 0)
          goto err_return;
        break;
      case 47:
        if (readString(f, tmp->rtAudioModule) != 0)
          goto err_return;
        break;
      case 54:
        if (readString(f, tmp->rtAudioOutputDevice) != 0)
          goto err_return;
        break;
      case 55:
        if (readString(f, tmp->rtAudioInputDevice) != 0)
          goto err_return;
        break;
      case 48:
        if (readString(f, tmp->rtMidiModule) != 0)
          goto err_return;
        break;
      case 49:
        if (readDouble(f, &(tmp->scoreOffsetSeconds)) != 0)
          goto err_return;
        break;
      case 50:
        if (readBool(f, &(tmp->useThreads)) != 0)
          goto err_return;
        break;
      case 51:
        if (readString(f, tmp->scriptFileName) != 0)
          goto err_return;
        break;
      case 53:
        if (readBool(f, &(tmp->runRealtime)) != 0)
          goto err_return;
        break;
      case 56:
        if (readBool(f, &(tmp->disableDiskOutput)) != 0)
          goto err_return;
        break;
      case 200:
        if (readString(f, tmp->additionalFlags) != 0)
          goto err_return;
        break;
      case 201:
        if (readBool(f, &(tmp->useAdditionalFlags)) != 0)
          goto err_return;
        break;
      default:
        goto err_return;
      }
    }

 err_return:
    fclose(f);
    if (tmp)
      delete tmp;
    return -1;
}

