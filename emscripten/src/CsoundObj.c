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


#import <csound.h>
#import <csdl.h>
#import <stdlib.h>
#import <string.h>
#import <stdio.h>
#import <stdbool.h>
#import <emscripten.h>

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

typedef struct _CsoundObj
{
	CSOUND *csound;
	uint32_t frameCount;
	uint32_t zerodBFS;
	MidiCallbackData *midiCallbackData;
} CsoundObj;

CsoundObj *CsoundObj_new()
{
	CsoundObj *self = calloc(1, sizeof(CsoundObj));
	self->frameCount = 256;
	self->csound = csoundCreate(self);
	csoundSetHostImplementedAudioIO(self->csound, 1, 0);

	self->midiCallbackData = calloc(1, sizeof(MidiCallbackData));
	self->midiCallbackData->midiData = calloc(MIDI_QUEUE_SIZE, sizeof(MidiData));
	return self;
}

void CsoundObj_compileCSD(CsoundObj *self,
		char *filePath,
		uint32_t samplerate)
{
	char samplerateArgument[20] = {0};
	char controlrateArgument[20] = {0};
	char bufferSizeArgument[20] = {0};
	double controlRate = (double)samplerate/(double)self->frameCount;
	sprintf((char *)&samplerateArgument, "-r %d", samplerate);
	sprintf((char *)&controlrateArgument, "-k %f", controlRate);
	sprintf((char *)&bufferSizeArgument, "-b %d", 256);

	char *argv[5] = {
		"csound",
		samplerateArgument,
		controlrateArgument,
		bufferSizeArgument,
		filePath
	};

	int result = csoundCompile(self->csound, 5, argv);

	if (result != 0) {

		printf("compilation failed\n");
	}
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

void CsoundObj_setControlChannel(CsoundObj *self, const char *channelName, float value) {

	csoundSetControlChannel(self->csound, channelName, value);
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


void CsoundObj_setOutputValueCallback(CsoundObj *self, void (*outputCallback)(CSOUND *csound, const char *channelName, float value))
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
