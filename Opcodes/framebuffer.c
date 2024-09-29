/*

 Framebuffer.c
 Framebuffer

 Created by Edward Costello on 10/06/2015.
 Copyright (c) 2015 Edward Costello.

 This file is part of Csound.

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "interlocks.h"

    typedef struct OLABuffer {

        OPDS h;
        MYFLT *outputArgument;
        MYFLT *inputArgument;
        MYFLT *overlapArgument;
        ARRAYDAT *inputArray;
        AUXCH frameSamplesMemory;
        AUXCH framePointerMemory;
        int32_t frameIndex;
        int32_t overlapSampleIndex;
        int32_t readSampleIndex;
        int32_t framesCount;
        int32_t frameSamplesCount;
        int32_t overlapSamplesCount;
        int32_t ksmps;
        MYFLT **frames;
    } OLABuffer;

    int32_t OLABuffer_initialise(CSOUND *csound, OLABuffer *self);
    int32_t OLABuffer_process(CSOUND *csound, OLABuffer *self);




    typedef enum ArgumentType
    {
        STRING_VAR,
        ARATE_VAR,
        KRATE_VAR,
        IRATE_VAR,
        ARATE_ARRAY,
        KRATE_ARRAY,
        IRATE_ARRAY,
        UNKNOWN
    } ArgumentType;

    typedef struct Framebuffer {

        OPDS h;
        MYFLT *outputArgument;
        MYFLT *inputArgument;
        MYFLT *sizeArgument;
        ArgumentType inputType;
        ArgumentType outputType;
        MYFLT *buffer;
        AUXCH bufferMemory;
        int32_t elementCount;
        int32_t writeIndex;
        int32_t ksmps;
    } Framebuffer;

    int32_t Framebuffer_initialise(CSOUND *csound, Framebuffer *self);
    int32_t Framebuffer_process(CSOUND *csound, Framebuffer *self);

ArgumentType Framebuffer_getArgumentType(CSOUND *csound, MYFLT *argument);
void Framebuffer_checkArgumentSanity(CSOUND *csound, Framebuffer *self);

void OLABuffer_checkArgumentSanity(CSOUND *csound, OLABuffer *self);

int32_t OLABuffer_initialise(CSOUND *csound, OLABuffer *self)
{
    OLABuffer_checkArgumentSanity(csound, self);
    self->inputArray = (ARRAYDAT *)self->inputArgument;
    self->frameSamplesCount = self->inputArray->sizes[0];
    self->framesCount = *self->overlapArgument;
    self->overlapSamplesCount = self->frameSamplesCount / self->framesCount;
    csound->AuxAlloc(csound,
                     self->frameSamplesCount * self->framesCount * sizeof(MYFLT),
                     &self->frameSamplesMemory);
    csound->AuxAlloc(csound, self->framesCount * sizeof(MYFLT *),
                     &self->framePointerMemory);
    self->frames = self->framePointerMemory.auxp;
    self->ksmps = self->h.insdshead->ksmps;

    int32_t i;
    for (i = 0; i < self->framesCount; ++i) {

      self->frames[i] =
        &((MYFLT *)self->frameSamplesMemory.auxp)[i * self->frameSamplesCount];
    }

    self->overlapSampleIndex = self->overlapSamplesCount;

    return OK;
}

void OLABuffer_writeFrame(OLABuffer *self, MYFLT *inputFrame, int32_t frameIndex)
{
    int32_t firstHalfOffset = self->overlapSamplesCount * frameIndex;
    int32_t firstHalfCount = self->frameSamplesCount - firstHalfOffset;
    int32_t secondHalfCount = self->frameSamplesCount - firstHalfCount;
    memcpy(&self->frames[frameIndex][firstHalfOffset], inputFrame,
           firstHalfCount * sizeof(MYFLT));
    memcpy(self->frames[frameIndex], &inputFrame[firstHalfCount],
           secondHalfCount * sizeof(MYFLT));
}

void OLABuffer_readFrame(OLABuffer *self, MYFLT *outputFrame,
                         int32_t outputFrameOffset,
                         int32_t olaBufferOffset, int32_t samplesCount)
{
    memcpy(&outputFrame[outputFrameOffset],
           &self->frames[0][olaBufferOffset], samplesCount * sizeof(MYFLT));

    int32_t i, j;
    for (i = 1; i < self->framesCount; ++i) {

      for (j = 0; j < samplesCount; ++j) {

        outputFrame[j + outputFrameOffset] += self->frames[i][j + olaBufferOffset];
      }
    }
}

int32_t OLABuffer_process(CSOUND *csound, OLABuffer *self)
{
     IGN(csound);
    int32_t nextKPassSampleIndex =
      (self->readSampleIndex + self->ksmps) % self->overlapSamplesCount;

    if (nextKPassSampleIndex == 0) {

      OLABuffer_writeFrame(self, self->inputArray->data, self->frameIndex);
      OLABuffer_readFrame(self, self->outputArgument, 0,
                          self->readSampleIndex, self->ksmps);
      self->frameIndex++;
      self->frameIndex %= self->framesCount;
    }
    else if (nextKPassSampleIndex < self->overlapSampleIndex) {

      int32_t firstHalfCount = self->overlapSamplesCount - self->overlapSampleIndex;

      if (firstHalfCount != 0) {

        OLABuffer_readFrame(self, self->outputArgument, 0,
                            self->readSampleIndex, firstHalfCount);
      }

      OLABuffer_writeFrame(self, self->inputArray->data, self->frameIndex);

      int32_t secondHalfCount = self->ksmps - firstHalfCount;

      if (secondHalfCount != 0) {

        OLABuffer_readFrame(self, self->outputArgument, firstHalfCount,
                            self->readSampleIndex, secondHalfCount);
      }

      self->frameIndex++;
      self->frameIndex %= self->framesCount;
    }
    else {

      OLABuffer_readFrame(self, self->outputArgument, 0,
                          self->readSampleIndex, self->ksmps);
    }

    self->overlapSampleIndex += self->ksmps;
    self->overlapSampleIndex %= self->overlapSamplesCount;
    self->readSampleIndex += self->ksmps;
    self->readSampleIndex %= self->frameSamplesCount;

    return OK;
}

void OLABuffer_checkArgumentSanity(CSOUND *csound, OLABuffer *self)
{
    MYFLT overlapCount = *self->overlapArgument;

    if (UNLIKELY(floor(overlapCount) != overlapCount)) {

      csound->Die(csound,
                  "%s", Str("olabuffer: Error, overlap factor must be an integer"));
    }

    ARRAYDAT *array = (ARRAYDAT *) self->inputArgument;

    if (UNLIKELY(array->dimensions != 1)) {

      csound->Die(csound, "%s",
                  Str("olabuffer: Error, k-rate array must be one dimensional"));
    }

    int32_t frameSampleCount = array->sizes[0];

    if (UNLIKELY(frameSampleCount <= (int32_t)overlapCount)) {

      csound->Die(csound,
                  "%s", Str("olabuffer: Error, k-rate array size must be "
                      "larger than ovelap factor"));
    }

    if (UNLIKELY(frameSampleCount % (int32_t)overlapCount != 0)) {

      csound->Die(csound, "%s", Str("olabuffer: Error, overlap factor must be "
                              "an integer multiple of k-rate array size"));
    }

    if (UNLIKELY(frameSampleCount / (int32_t)overlapCount <
                 (int32_t) self->h.insdshead->ksmps)) {

      csound->Die(csound, "%s", Str("olabuffer: Error, k-rate array size divided "
                              "by overlap factor must be larger than or equal "
                              "to ksmps"));
    }
}

int32_t Framebuffer_initialise(CSOUND *csound, Framebuffer *self)
{
    self->inputType = Framebuffer_getArgumentType(csound, self->inputArgument);
    self->outputType = Framebuffer_getArgumentType(csound, self->outputArgument);
    self->elementCount = *self->sizeArgument;
    self->ksmps = self->h.insdshead->ksmps;

    Framebuffer_checkArgumentSanity(csound, self);

    csound->AuxAlloc(csound, self->elementCount * sizeof(MYFLT),
                     &self->bufferMemory);
    self->buffer = self->bufferMemory.auxp;

    if (self->outputType == KRATE_ARRAY) {

        ARRAYDAT *array = (ARRAYDAT *) self->outputArgument;
        array->sizes = csound->Calloc(csound, sizeof(int32_t));
        array->sizes[0] = self->elementCount;
        array->dimensions = 1;
        CS_VARIABLE *var = array->arrayType->createVariable(csound,
                                                            NULL, &(self->h));
        array->arrayMemberSize = var->memBlockSize;
        array->data = csound->Calloc(csound,
                                     var->memBlockSize * self->elementCount);
    }

    return OK;
}

void Framebuffer_writeBuffer(CSOUND *csound, Framebuffer *self,
                             MYFLT *inputSamples, int32_t inputSamplesCount)
{
     IGN(csound);
    if (self->writeIndex + inputSamplesCount <= self->elementCount) {

        memcpy(&self->buffer[self->writeIndex], inputSamples,
               sizeof(MYFLT) * inputSamplesCount);
        self->writeIndex += self->ksmps;
        self->writeIndex %= self->elementCount;
    }
    else {

        int32_t firstHalf = self->elementCount - self->writeIndex;
        memcpy(&self->buffer[self->writeIndex], inputSamples,
               sizeof(MYFLT) * firstHalf);
        int32_t secondHalf = inputSamplesCount - firstHalf;
        memcpy(self->buffer, &inputSamples[firstHalf],
               sizeof(MYFLT) * secondHalf);
        self->writeIndex = secondHalf;
    }
}

void Framebuffer_readBuffer(CSOUND *csound, Framebuffer *self,
                            MYFLT *outputSamples, int32_t outputSamplesCount)
{
     IGN(csound);
    if (self->writeIndex + outputSamplesCount < self->elementCount) {

        memcpy(outputSamples, &self->buffer[self->writeIndex],
               sizeof(MYFLT) * outputSamplesCount);
    }
    else {

        int32_t firstHalf = self->elementCount - self->writeIndex;
        memcpy(outputSamples, &self->buffer[self->writeIndex],
               sizeof(MYFLT) * firstHalf);
        int32_t secondHalf = outputSamplesCount - firstHalf;
        memcpy(&outputSamples[firstHalf], self->buffer,
               sizeof(MYFLT) * secondHalf);
    }
}

void Framebuffer_processAudioInFrameOut(CSOUND *csound, Framebuffer *self)
{
    Framebuffer_writeBuffer(csound, self, self->inputArgument, self->ksmps);
    ARRAYDAT *array = (ARRAYDAT *)self->outputArgument;
    Framebuffer_readBuffer(csound, self, array->data, array->sizes[0]);
}


void Framebuffer_processFrameInAudioOut(CSOUND *csound, Framebuffer *self)
{
    ARRAYDAT *array = (ARRAYDAT *)self->inputArgument;
    Framebuffer_writeBuffer(csound, self, array->data, array->sizes[0]);
    Framebuffer_readBuffer(csound, self, self->outputArgument, self->ksmps);
}

int32_t Framebuffer_process(CSOUND *csound, Framebuffer *self)
{
    if (self->inputType == KRATE_ARRAY) {


        Framebuffer_processFrameInAudioOut(csound, self);
    }
    else if (self->inputType == ARATE_VAR) {

        Framebuffer_processAudioInFrameOut(csound, self);
    }


    return OK;
}

void Framebuffer_checkArgumentSanity(CSOUND *csound, Framebuffer *self)
{
  if (UNLIKELY((uint32_t)self->elementCount < self->h.insdshead->ksmps)) {

      csound->Die(csound, "%s", Str("framebuffer: Error, specified element "
                              "count less than ksmps value, Exiting"));
    }

    if (self->inputType == ARATE_VAR) {

      if (UNLIKELY(self->outputType != KRATE_ARRAY)) {

          csound->Die(csound, "%s", Str("framebuffer: Error, only k-rate arrays "
                                  "allowed for a-rate var inputs, Exiting"));
        }
    }
    else if (LIKELY(self->inputType == KRATE_ARRAY)) {

      if (UNLIKELY(self->outputType != ARATE_VAR)) {

          csound->Die(csound, "%s", Str("framebuffer: Error, only a-rate vars "
                                  "allowed for k-rate array inputs, Exiting"));
        }

        ARRAYDAT *array = (ARRAYDAT *) self->inputArgument;

        if (UNLIKELY(array->dimensions != 1)) {

          csound->Die(csound, "%s", Str("framebuffer: Error, k-rate array input "
                                  "must be one dimensional, Exiting"));
        }

        if (UNLIKELY(array->sizes[0] > self->elementCount)) {

          csound->Die(csound, "%s", Str("framebuffer: Error, k-rate array input "
                                  "element count must be less than\nor equal "
                                  "to specified framebuffer size, Exiting"));
        }
    }
    else {

      csound->Die(csound,
                  "%s", Str("framebuffer: Error, only a-rate var input with k-rate "
                      "array output or k-rate\narray input with a-rate var "
                      "output are valid arguments, Exiting"));
    }
}

ArgumentType Framebuffer_getArgumentType(CSOUND *csound, MYFLT *argument)
{
    const CS_TYPE *csoundType = GetTypeForArg((void *)argument);
    const char *type = csoundType->varTypeName;
    ArgumentType argumentType = UNKNOWN;

    if (strcmp("S", type) == 0) {

        argumentType = STRING_VAR;
    }
    else if (strcmp("a", type) == 0) {

        argumentType = ARATE_VAR;
    }
    else if (strcmp("k", type) == 0) {

        argumentType = KRATE_VAR;
    }
    else if (strcmp("i", type) == 0) {

        argumentType = IRATE_VAR;
    }
    else if (strcmp("[", type) == 0) {

        ARRAYDAT *array = (ARRAYDAT *)argument;

        if (strcmp("k", array->arrayType->varTypeName) == 0) {

            argumentType = KRATE_ARRAY;
        }
        else if (strcmp("a", array->arrayType->varTypeName) == 0) {

            argumentType = ARATE_ARRAY;
        }
        else if (strcmp("i", array->arrayType->varTypeName) == 0) {

            argumentType = IRATE_ARRAY;
        }
    }

    return argumentType;
}

static OENTRY framebuffer_localops[] = {

    {
        .opname = "framebuffer",
        .dsblksiz = sizeof(Framebuffer),
        .outypes = "*",
        .intypes = "*",
        .init = (SUBR)Framebuffer_initialise,
        .perf = (SUBR)Framebuffer_process,
        .deinit = NULL
    },
    {
        .opname = "olabuffer",
        .dsblksiz = sizeof(OLABuffer),
        .outypes = "a",
        .intypes = "k[]i",
        .init = (SUBR)OLABuffer_initialise,
        .perf = (SUBR)OLABuffer_process,
        .deinit = NULL
    }
};

LINKAGE_BUILTIN(framebuffer_localops)
