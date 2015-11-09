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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 */

#include "Framebuffer.h"

ArgumentType Framebuffer_getArgumentType(CSOUND *csound, MYFLT *argument);
void Framebuffer_checkArgumentSanity(CSOUND *csound, Framebuffer *self);

int Framebuffer_initialise(CSOUND *csound, Framebuffer *self)
{
    self->inputType = Framebuffer_getArgumentType(csound, self->inputArgument);
    self->outputType = Framebuffer_getArgumentType(csound, self->outputArgument);
    self->elementCount = *self->sizeArgument;
    self->ksmps = csound->GetKsmps(csound);

    Framebuffer_checkArgumentSanity(csound, self);

    csound->AuxAlloc(csound, self->elementCount * sizeof(MYFLT),
                     &self->bufferMemory);
    self->buffer = self->bufferMemory.auxp;

    if (self->outputType == KRATE_ARRAY) {

        ARRAYDAT *array = (ARRAYDAT *) self->outputArgument;
        array->sizes = csound->Calloc(csound, sizeof(int));
        array->sizes[0] = self->elementCount;
        array->dimensions = 1;
        CS_VARIABLE *var = array->arrayType->createVariable(csound, NULL);
        array->arrayMemberSize = var->memBlockSize;
        array->data = csound->Calloc(csound,
                                     var->memBlockSize * self->elementCount);
    }

    return OK;
}

void Framebuffer_writeBuffer(CSOUND *csound, Framebuffer *self,
                             MYFLT *inputSamples, int inputSamplesCount)
{
    if (self->writeIndex + inputSamplesCount <= self->elementCount) {

        memcpy(&self->buffer[self->writeIndex], inputSamples,
               sizeof(MYFLT) * inputSamplesCount);
        self->writeIndex += self->ksmps;
        self->writeIndex %= self->elementCount;
    }
    else {

        int firstHalf = self->elementCount - self->writeIndex;
        memcpy(&self->buffer[self->writeIndex], inputSamples,
               sizeof(MYFLT) * firstHalf);
        int secondHalf = inputSamplesCount - firstHalf;
        memcpy(self->buffer, &inputSamples[firstHalf],
               sizeof(MYFLT) * secondHalf);
        self->writeIndex = secondHalf;
    }
}

void Framebuffer_readBuffer(CSOUND *csound, Framebuffer *self,
                            MYFLT *outputSamples, int outputSamplesCount)
{
    if (self->writeIndex + outputSamplesCount < self->elementCount) {

        memcpy(outputSamples, &self->buffer[self->writeIndex],
               sizeof(MYFLT) * outputSamplesCount);
    }
    else {

        int firstHalf = self->elementCount - self->writeIndex;
        memcpy(outputSamples, &self->buffer[self->writeIndex],
               sizeof(MYFLT) * firstHalf);
        int secondHalf = outputSamplesCount - firstHalf;
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

int Framebuffer_process(CSOUND *csound, Framebuffer *self)
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
    if (self->elementCount < csound->GetKsmps(csound)) {

      csound->Die(csound, Str("framebuffer: Error, specified element "
                              "count less than ksmps value, Exiting"));
    }

    if (self->inputType == ARATE_VAR) {

        if (self->outputType != KRATE_ARRAY) {

          csound->Die(csound, Str("framebuffer: Error, only k-rate arrays "
                                  "allowed for a-rate var inputs, Exiting"));
        }
    }
    else if (self->inputType == KRATE_ARRAY) {

        if (self->outputType != ARATE_VAR) {

          csound->Die(csound, Str("framebuffer: Error, only a-rate vars "
                                  "allowed for k-rate array inputs, Exiting"));
        }

        ARRAYDAT *array = (ARRAYDAT *) self->inputArgument;

        if (array->dimensions != 1) {

          csound->Die(csound, Str("framebuffer: Error, k-rate array input "
                                  "must be one dimensional, Exiting"));
        }

        if (array->sizes[0] > self->elementCount) {

          csound->Die(csound, Str("framebuffer: Error, k-rate array input "
                                  "element count must be less than \nor equal "
                                  "to specified framebuffer size, Exiting"));
        }
    }
    else {

      csound->Die(csound,
                  Str("framebuffer: Error, only a-rate var input with k-rate "
                      "array output or k-rate\narray input with a-rate var "
                      "output are valid arguments, Exiting"));
    }
}

ArgumentType Framebuffer_getArgumentType(CSOUND *csound, MYFLT *argument)
{
    const CS_TYPE *csoundType = csound->GetTypeForArg((void *)argument);
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
