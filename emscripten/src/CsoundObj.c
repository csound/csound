/*
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 */


#include <csound.h>
#include <csdl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <emscripten.h>
#include <SDL/SDL_audio.h>

typedef struct {

  unsigned char status;
  unsigned char data1;
  unsigned char data2;
  unsigned char flag;

} MidiData;

typedef struct  {

  MidiData *midiData;
  int p, q;

} MidiCallbackData;

#define MIDI_QUEUE_SIZE 1024

enum { CS_RESET_STATUS = 0,
       CS_STARTED_STATUS };

typedef struct _CsoundObj
{
  CSOUND *csound;
  uint32_t zerodBFS;
  MidiCallbackData *midiCallbackData;
  uint32_t status;
  SDL_AudioSpec spec;
  int cnt;
} CsoundObj;


CsoundObj *CsoundObj_new()
{
  CsoundObj *self = calloc(1, sizeof(CsoundObj));
  self->csound = csoundCreate(self);
  self->midiCallbackData = calloc(1, sizeof(MidiCallbackData));
  self->midiCallbackData->midiData = calloc(MIDI_QUEUE_SIZE, sizeof(MidiData));
  self->status = CS_RESET_STATUS;
  return self;
}

void CsoundObj_destroy(CsoundObj *self)
{
  csoundDestroy(self->csound);
  free(self);
}


void CsoundObj_setOption(CsoundObj *self, const char *option)
{
  csoundSetOption(self->csound, option);
}


void CsoundObj_compileCSD(CsoundObj *self,
			  char *csd)
{
    csoundMessage(self->csound, "CsoundObj_compileCSD...\n");
    int result = 0;
    if (csd == 0) {
        csoundMessage(self->csound, "Error: Null CSD.\n");
        return;
    }
    // See if this is a filename or the text of a CSD.
    char *csd_start_tag = strstr(csd, "<CsoundSynthesizer>");
    char *csd_end_tag =  strstr(csd, "</CsoundSynthesizer>");
    if (csd_start_tag && csd_end_tag) {
        csoundMessage(self->csound, "csoundCompileCsdText...\n");
        result = csoundCompileCsdText(self->csound, csd);
        result |= csoundStart(self->csound);
        if (result != 0) {
             csoundMessage(self->csound, "Failed to compile CSD text.\n");
        } else {
            self->status = CS_STARTED_STATUS;
        }
        return;
    } else {
        csoundMessage(self->csound, "csoundCompile...\n");
        const char *argv[2] = {
            "csound",
            csd
        };
        result = csoundCompile(self->csound, 2, argv);
        if (result != 0) {
             csoundMessage(self->csound, "Failed to compile CSD file.\n");
        } else {
            self->status = CS_STARTED_STATUS;
        }
    }
}

void CsoundObj_prepareRT(CsoundObj *self) {
  csoundSetHostImplementedAudioIO(self->csound, 1, 0);
}

uint32_t CsoundObj_compileOrc(CsoundObj *self, const char *string)
{
  int returnValue = csoundCompileOrc(self->csound, (char *) string);
  if(self->status == CS_RESET_STATUS) {
   csoundStart(self->csound);
   self->status = CS_STARTED_STATUS;
  }
  return returnValue;
}


void CsoundObj_render(CsoundObj *self) 
{
  while(csoundPerformKsmps(self->csound) == 0);
  csoundCleanup(self->csound);
}

float CsoundObj_evaluateCode(CsoundObj *self, const char *codeString)
{
  return csoundEvalCode(self->csound, codeString);
}

int CsoundObj_readScore(CsoundObj *self, const char *scoreString)
{
  return csoundReadScore(self->csound, scoreString);
}

float CsoundObj_getControlChannel(CsoundObj *self, const char *channelName) {

  int *error = NULL;
  float returnValue = csoundGetControlChannel(self->csound, channelName, error);
  if (error != NULL) {

    printf("CsoundObj.getControlChannel: Error %d\n", *error);
    return 0;
  }
  else {

    return returnValue;
  }
}

float CsoundObj_getScoreTime(CsoundObj *self) {
  return csoundGetScoreTime(self->csound);
}

void CsoundObj_setTable(CsoundObj *self, int num, int index, float val) {
  csoundTableSet(self->csound, num, index, val);
}

void CsoundObj_setControlChannel(CsoundObj *self, const char *channelName, float value) {

  csoundSetControlChannel(self->csound, channelName, value);
}

void CsoundObj_setStringChannel(CsoundObj *self, const char *channelName, char *string) {

  csoundSetStringChannel(self->csound, channelName, string);
}

float *CsoundObj_getOutputBuffer(CsoundObj *self)
{
  return csoundGetSpout(self->csound);
}

float *CsoundObj_getInputBuffer(CsoundObj *self)
{
  return csoundGetSpin(self->csound);
}

int CsoundObj_getKsmps(CsoundObj *self)
{
  return csoundGetKsmps(self->csound);
}

int CsoundObj_performKsmps(CsoundObj *self)
{
  return csoundPerformKsmps(self->csound);
}

size_t CsoundObj_getZerodBFS(CsoundObj *self) 
{
  return csoundGet0dBFS(self->csound);
}

void CsoundObj_reset(CsoundObj *self)
{
  csoundCleanup(self->csound);
  csoundReset(self->csound);
  self->status = CS_RESET_STATUS;
}

int CsoundObj_getInputChannelCount(CsoundObj *self)
{
  return csoundGetNchnlsInput(self->csound);
}

int CsoundObj_getOutputChannelCount(CsoundObj *self)
{
  return csoundGetNchnls(self->csound);
}

int CsoundObj_getTableLength(CsoundObj *self, int tableNumber)
{
  return csoundGetTable(self->csound, NULL, tableNumber);
}

float *CsoundObj_getTable(CsoundObj *self, int tableNumber)
{
  float *tablePointer;
  csoundGetTable(self->csound, &tablePointer, tableNumber);
  return tablePointer;
}


void CsoundObj_setOutputChannelCallback(CsoundObj *self, void (*outputCallback)(CSOUND *csound, const char *channelName, void *valuePointer, const void *channelType))
{
  csoundSetOutputChannelCallback(self->csound, outputCallback);
}

void CsoundObj_pushMidiMessage(CsoundObj *self, unsigned char status, unsigned char data1, unsigned char data2)
{
  self->midiCallbackData->midiData[self->midiCallbackData->p].status = status;
  self->midiCallbackData->midiData[self->midiCallbackData->p].data1 = data1;
  self->midiCallbackData->midiData[self->midiCallbackData->p].data2= data2;
  self->midiCallbackData->midiData[self->midiCallbackData->p].flag = 1;

  self->midiCallbackData->p++;
  if (self->midiCallbackData->p == MIDI_QUEUE_SIZE) {

    self->midiCallbackData->p = 0;
  }
}

/* used to distinguish between 1 and 2-byte messages */
static const int datbyts[8] = { 2, 2, 2, 2, 1, 1, 2, 0  };

/* csound MIDI read callback, called every k-cycle */
static int CsoundObj_midiDataRead(CSOUND *csound, void *userData, unsigned char *mbuf, int nbytes)
{

  CsoundObj *self = csoundGetHostData(csound);
  MidiCallbackData *data = self->midiCallbackData;

  if(data == NULL) return 0;

  MidiData *mdata = data->midiData;

  int *q = &data->q, st, d1, d2, n = 0;

  /* check if there is new data on circular queue */
  while (mdata[*q].flag) {
    st = (int) mdata[*q].status;
    d1 = (int) mdata[*q].data1;
    d2 = (int) mdata[*q].data2;

    if (st < 0x80)
      goto next;

    if (st >= 0xF0 &&
	!(st == 0xF8 || st == 0xFA || st == 0xFB ||
	  st == 0xFC || st == 0xFF))
      goto next;
    nbytes -= (datbyts[(st - 0x80) >> 4] + 1);
    if (nbytes < 0) break;

    /* write to csound midi buffer */
    n += (datbyts[(st - 0x80) >> 4] + 1);
    switch (datbyts[(st - 0x80) >> 4]) {
    case 0:
      *mbuf++ = (unsigned char) st;
      break;
    case 1:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      break;
    case 2:
      *mbuf++ = (unsigned char) st;
      *mbuf++ = (unsigned char) d1;
      *mbuf++ = (unsigned char) d2;
      break;

    } 
  next:
    mdata[*q].flag = 0;
    (*q)++;
    if(*q== MIDI_QUEUE_SIZE) *q = 0;

  }

  /* return the number of bytes read */
  return n;

}

static int CsoundObj_midiInOpen(CSOUND *csound, void **userData, const char *dev)
{
  return OK;
}

static int CsoundObj_midiInClose(CSOUND *csound, void *userData)
{
  return OK;
}

void CsoundObj_setMidiCallbacks(CsoundObj *self)
{
  csoundSetHostImplementedMIDIIO(self->csound, 1);
  csoundSetExternalMidiInOpenCallback(self->csound, CsoundObj_midiInOpen);
  csoundSetExternalMidiReadCallback(self->csound, CsoundObj_midiDataRead);
  csoundSetExternalMidiInCloseCallback(self->csound, CsoundObj_midiInClose);
}

/* SDL Audio implementation 
   VL, June 2017
 */
void playback(void*  userdata,
              Uint8* stream,
	      int    len){

  CsoundObj *self = ((CsoundObj *) userdata);
  CSOUND *csound = self->csound;
  short *samples = (short *)stream;
  MYFLT *data = csoundGetSpout(csound);
  int i, n = len/sizeof(short);
  MYFLT scal = csoundGet0dBFS(csound);
  int ksmps = csoundGetKsmps(csound)*csoundGetNchnls(csound);

  for(i = 0; i < n; i++) {
    if(self->cnt == ksmps) {
      csoundPerformKsmps(csound);
      self->cnt = 0;
    }
    samples[i] = data[self->cnt++]*32768/scal;
  }
}
  
int CsoundObj_openAudioOut(CsoundObj *self) {
 SDL_AudioSpec inSpec;
 inSpec.freq = csoundGetSr(self->csound);
 inSpec.format = AUDIO_S16;
 inSpec.channels = csoundGetNchnls(self->csound);
 inSpec.samples =  csoundGetOutputBufferSize(self->csound);
 inSpec.callback = playback;
 inSpec.userdata = (void *) self;
 int res = SDL_OpenAudio(&inSpec, &(self->spec));
 if (res == 0) {
    printf("Failed to open audio: %s", SDL_GetError());
    return CSOUND_ERROR;
 }
 self->cnt = csoundGetKsmps(self->csound)*csoundGetNchnls(self->csound);
 return 0;
}

void CsoundObj_play(CsoundObj *self) {
   SDL_PauseAudio(0);
}

void CsoundObj_pause(CsoundObj *self) {
   SDL_PauseAudio(1);
}

void CsoundObj_closeAudioOut(CsoundObj *self) {
  SDL_CloseAudio();
}
