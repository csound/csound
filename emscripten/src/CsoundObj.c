#import <csound.h>
#import <csdl.h>
#import <stdlib.h>
#import <string.h>

typedef struct _CsoundObj
{
	CSOUND *csound;
	double *csoundOut;
	double *csoundIn;
	uint32_t zerodBFS;
} CsoundObj;

CsoundObj *CsoundObj_new()
{
	CsoundObj *self = calloc(1, sizeof(CsoundObj));
	self->csound = csoundCreate(NULL);
	csoundSetHostImplementedAudioIO(self->csound, 1, 0);

	return self;
}

void CsoundObj_compileCSD(CsoundObj *self,
			  char *filePath,
			  uint32_t samplerate,
			  double controlrate,
			  uint32_t bufferSize)
{ 
	char samplerateArgument[10] = {0};
	char controlrateArgument[10] = {0};
	char bufferSizeArgument[10] = {0};
	sprintf((char *)&samplerateArgument, "-r %d", samplerate);
	sprintf((char *)&controlrateArgument, "-k %f", controlrate);
	sprintf((char *)&bufferSizeArgument, "-b %d", bufferSize);

	char *argv[5] = {
		"csound",
		samplerateArgument,
		controlrateArgument,
		bufferSizeArgument,
		filePath
	};

	printf("File name is %s\n", filePath);

	int result = csoundCompile(self->csound, 5, argv);

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
	int outputChannelCount = csoundGetNchnls(self->csound);
	int inputChannelCount = csoundGetNchnlsInput(self->csound);

	if (result == 0) {

		memcpy(self->csoundIn, inputBuffer, sizeof(double) * inNumberFrames * inputChannelCount);

		for (int i = 0; i < inNumberFrames * inputChannelCount; ++i) {

			self->csoundIn[i] *= (double)self->zerodBFS;
		}

		memcpy(outputBuffer, self->csoundOut, sizeof(double) * inNumberFrames * outputChannelCount);

		for (int i = 0; i < inNumberFrames * outputChannelCount; ++i) {

			outputBuffer[i] /= (double)self->zerodBFS;
		}
	}
	else {
		memset(outputBuffer, 0, sizeof(double) * inNumberFrames * outputChannelCount);
	}

	return result;
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

void CsoundObj_stop(CsoundObj *self)
{
	csoundStop(self->csound);
}

void CsoundObj_reset(CsoundObj *self)
{
	csoundReset(self->csound);
}

void CsoundObj_setControlChannel(CsoundObj *self,
				 const char *name,
				 double value)
{
	csoundSetControlChannel(self->csound, name, value);
}
