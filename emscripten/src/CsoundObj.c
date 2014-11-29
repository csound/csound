#import <csound.h>
#import <csdl.h>
#import <stdlib.h>
#import <string.h>
#import <stdio.h>

typedef struct _CsoundObj
{
	CSOUND *csound;
	double *csoundOut;
	double *csoundIn;
	uint32_t zerodBFS;
	int useAudioInput;
} CsoundObj;

CsoundObj *CsoundObj_new()
{
	CsoundObj *self = calloc(1, sizeof(CsoundObj));
	self->csound = csoundCreate(NULL);
	self->useAudioInput = 0;
	csoundSetHostImplementedAudioIO(self->csound, 1, 0);

	return self;
}

void CsoundObj_compileCSD(CsoundObj *self,
		char *filePath,
		uint32_t samplerate)
{
	char samplerateArgument[20] = {0};
	char controlrateArgument[20] = {0};
	char bufferSizeArgument[20] = {0};
    double controlRate = (double)samplerate/256.;
	sprintf((char *)&samplerateArgument, "-r %d", samplerate);
	sprintf((char *)&controlrateArgument, "-k %f", controlRate);
	sprintf((char *)&bufferSizeArgument, "-b %d", 256);

	char *argv[6] = {
		"csound",
        "-odac",
		samplerateArgument,
		controlrateArgument,
		bufferSizeArgument,
		filePath
	};
    
	int result = csoundCompile(self->csound, 6, argv);

	if (result == 0) {

		printf("success\n");
		self->csoundIn = csoundGetSpin(self->csound);
		self->csoundOut = csoundGetSpout(self->csound);
		self->zerodBFS = csoundGet0dBFS(self->csound);
	}
	else {

		printf("compilation failed\n");
	}
}

int CsoundObj_process(CsoundObj *self,
		int inNumberFrames,
		double *inputBuffer,
		double *outputBuffer)
{
	int result = csoundPerformKsmps(self->csound);

	if (result == 0) {

		int outputChannelCount = csoundGetNchnls(self->csound);
		int inputChannelCount = csoundGetNchnlsInput(self->csound);

		self->csoundOut = csoundGetSpout(self->csound);
		self->csoundIn = csoundGetSpin(self->csound);

		if (self->useAudioInput == 1) {

			memcpy(self->csoundIn, inputBuffer, sizeof(double) * inNumberFrames);
		}

		memcpy(outputBuffer, self->csoundOut, sizeof(double) * inNumberFrames * outputChannelCount);
//		printf("csoundOut =%f outputBuffer = %f\n", self->csoundOut[0], outputBuffer[0]);
	}

	return result;
}
uint32_t CsoundObj_compileOrc(CsoundObj *self, const char *string)
{
	int returnValue = csoundCompileOrc(self->csound, (char *) string);	
	csoundStart(self->csound);
	return returnValue;
}

uint32_t CsoundObj_readScore(CsoundObj *self, const char *string)
{
	return csoundReadScore(self->csound, (char *)string);
}

uint32_t CsoundObj_getKsmps(CsoundObj *self)
{
	return csoundGetKsmps(self->csound);
}

uint32_t CsoundObj_getNchnls(CsoundObj *self)
{
	return csoundGetNchnls(self->csound);
}

uint32_t CsoundObj_getNchnlsInput(CsoundObj *self)
{
	return csoundGetNchnlsInput(self->csound);
}

void CsoundObj_start(CsoundObj *self)
{
	csoundStart(self->csound);
}

void CsoundObj_stop(CsoundObj *self)
{
	csoundStop(self->csound);
}

void CsoundObj_reset(CsoundObj *self)
{
	csoundReset(self->csound);
}

void CsoundObj_setUsingAudioInput(CsoundObj *self, int value)
{
	self->useAudioInput = value;	
}

void CsoundObj_setControlChannel(CsoundObj *self,
		const char *name,
		double value)
{
	csoundSetControlChannel(self->csound, name, value);
}
