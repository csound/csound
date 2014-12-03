#import <csound.h>
#import <csdl.h>
#import <stdlib.h>
#import <string.h>
#import <stdio.h>
#import <stdbool.h>
#import <emscripten.h>

typedef struct _CsoundObj
{
	CSOUND *csound;
	uint32_t frameCount;
	uint32_t zerodBFS;
	bool useAudioInput;
	bool printLog;
} CsoundObj;

static void CsoundObj_messageCallback(CSOUND *cs, int attr, const char *format, va_list valist);


CsoundObj *CsoundObj_new(int bufferSize, bool printLog)
{
	CsoundObj *self = calloc(1, sizeof(CsoundObj));
	self->frameCount = 256;
	self->csound = csoundCreate(NULL);
	self->useAudioInput = 0;
	self->printLog = printLog;
	csoundSetHostImplementedAudioIO(self->csound, 1, bufferSize);
	csoundSetMessageCallback(self->csound, CsoundObj_messageCallback);
	csoundSetHostData(self->csound, self);
	return self;
}

static void CsoundObj_messageCallback(CSOUND *csound, int attr, const char *format, va_list valist)
{
	CsoundObj *self = csoundGetHostData(csound); 

	if (self->printLog == false) {

		return;
	}

	char buffer[4096];
	static int newLine = 1;
	int i;
	buffer[4095] = 0;
	i = vsnprintf(buffer, 4095, format, valist);
	printf(newLine ? "Csound: %s" : "%s", buffer);
	newLine=(i > 0 && i < 4095 && buffer[i - 1] == '\n') ? 1 : 0;
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

	char *argv[6] = {
		"csound",
		"-odac",
		samplerateArgument,
		controlrateArgument,
		bufferSizeArgument,
		filePath
	};

	int result = csoundCompile(self->csound, 6, argv);

	if (result != 0) {

		printf("compilation failed\n");
	}
}

int CsoundObj_process(CsoundObj *self, MYFLT *input, MYFLT *output)
{
	int result = csoundPerformKsmps(self->csound);

	if (result == 0) {

		int outputChannelCount = csoundGetNchnls(self->csound);
		int inputChannelCount = csoundGetNchnlsInput(self->csound);

		MYFLT *csoundOut = csoundGetSpout(self->csound);
		MYFLT *csoundIn = csoundGetSpin(self->csound);

		memcpy(output, csoundOut, sizeof(MYFLT) * self->frameCount * outputChannelCount);
	}

	return result;	
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
