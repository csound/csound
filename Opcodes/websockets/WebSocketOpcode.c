/*
 WebSocketOpcode.c
 WebSocketOpcode

 Copyright (C) 2015, 2016
 Edward Costello

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

#include "WebSocketOpcode.h"
#include <sys/param.h>
#include <stdio.h>
#include <libwebsockets.h>
#include <stdlib.h>

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


struct OpcodeArgument{

    void *dataPointer;
    MYFLT *argumentPointer;
    bool receivedData;
    ArgumentType argumentType;
    AUXCH auxillaryMemory;
    void *circularBuffer;
    char *name;
    void *readBuffer;
    int32_t itemsCount;
    uint32_t bytesCount;
    uint32_t bytesWritten;
    bool iRateVarSent;
};

struct WebSocket{

    struct lws_context *context;
    struct lws *websocket;
    struct lws_protocols *protocols;
    void *processThread;
    unsigned char *messageBuffer;
    struct lws_context_creation_info info;
};

typedef enum ArgumentOffset {

    OUTPUT_OFFSET = 1000, INPUT_OFFSET = 2000
} ArgumentOffset;

static const size_t writeBufferBytesCount = 2048;
static const size_t stringVarMaximumBytesCount = 4096;
static const size_t ringBufferItemsCount = 2048 * 16;

void WebSocketOpcode_initialiseWebSocket(WebSocketOpcode *self, CSOUND *csound);
int32_t WebSocketOpcode_finish(CSOUND *csound, void *opaqueReference);
void WebSocketOpcode_initialiseArguments(WebSocketOpcode *self, CSOUND *csound);
ArgumentType WebSocketOpcode_getArgumentType(CSOUND *csound, MYFLT *argument);

int32_t WebSocketOpcode_initialise(CSOUND *csound, WebSocketOpcode *self)
{
    self->inputArgumentCount = self->INOCOUNT - 1;
    self->outputArgumentCount = self->OUTOCOUNT;
    self->csound = csound;
    WebSocketOpcode_initialiseArguments(self, csound);
    WebSocketOpcode_initialiseWebSocket(self, csound);
    csound->RegisterDeinitCallback(csound, self, WebSocketOpcode_finish);
    return OK;
}

void WebSocketOpcode_sendInputArgumentData(CSOUND *csound, WebSocketOpcode *self)
{
    int32_t i;
    for (i = 0; i < self->inputArgumentCount; ++i) {

      OpcodeArgument *currentArgument = &self->inputArguments[i];

      if (currentArgument->iRateVarSent == true) {

        continue;
      }

      int32_t itemsWritten =
        csoundWriteCircularBuffer(csound,
                                  currentArgument->circularBuffer,
                                  currentArgument->dataPointer,
                                  currentArgument->itemsCount);

      if (UNLIKELY(itemsWritten != currentArgument->itemsCount)) {

        csound->Message(csound,
                        Str("websocket: variable %s data not sent, "
                            "buffer overrrun\n"), currentArgument->name);
      }
    }
}

void WebSocketOpcode_receiveOutputArgumentData(CSOUND *csound,
                                               WebSocketOpcode *self)
{
    int32_t i;
    for (i = 0; i < self->outputArgumentCount; ++i) {

      OpcodeArgument *currentArgument = &self->outputArguments[i];

      if (currentArgument->receivedData == true) {

        if (currentArgument->iRateVarSent == true) {

          continue;
        }

        csoundReadCircularBuffer(csound, currentArgument->circularBuffer,
                                 currentArgument->dataPointer,
                                 currentArgument->itemsCount);

        if (currentArgument->argumentType == STRING_VAR) {

          STRINGDAT *string = (STRINGDAT *)currentArgument->argumentPointer;
          string->size = (int32_t)strlen(currentArgument->dataPointer);
        }

        currentArgument->receivedData = false;
      }

    }
}

int32_t WebSocketOpcode_process(CSOUND *csound, WebSocketOpcode *self)
{
    WebSocketOpcode_sendInputArgumentData(csound, self);
    WebSocketOpcode_receiveOutputArgumentData(csound, self);
    return OK;
}

int32_t WebSocketOpcode_finish(CSOUND *csound, void *opaqueReference)
{
    int32_t i;
    WebSocketOpcode *self = opaqueReference;
    self->isRunning = false;

    csound->JoinThread(self->webSocket->processThread);

    lws_cancel_service(self->webSocket->context);
    lws_context_destroy(self->webSocket->context);

    for (i = 0; i < self->outputArgumentCount; ++i) {

      csoundDestroyCircularBuffer(csound, self->outputArguments[i].circularBuffer);
    }

    for (i = 0; i < self->inputArgumentCount; ++i) {

      csoundDestroyCircularBuffer(csound, self->inputArguments[i].circularBuffer);
    }

    csound->Free(csound, self->webSocket->protocols);
    csound->Free(csound, self->webSocket->messageBuffer);
    csound->Free(csound, self->webSocket);
    if (self->inputArgumentCount > 0) {

      csound->Free(csound, self->inputArguments);
    }
    if (self->outputArgumentCount > 0) {

      csound->Free(csound, self->outputArguments);
    }
    return OK;
}

int32_t WebSocketOpcode_getArrayElementCount(ARRAYDAT *array)
{
    int32_t elementCount = array->sizes[0];
    int32_t i;
    for (i = 1; i < array->dimensions; ++i) {

      elementCount *= array->sizes[i];
    }

    return elementCount;
}


void WebSocketOpcode_allocateStringArgument(MYFLT *argument,
                                            OpcodeArgument *argumentArrayItem,
                                            CSOUND *csound, bool isInputArgument)
{
    STRINGDAT *string = (STRINGDAT *)argument;

    if (UNLIKELY(isInputArgument == true)) {

      csound->Die(csound,
                  "%s", Str("websocket: this opcode does not send strings, only "
                      "receiving them is supported\nExiting"));
    }
    else {

      if (UNLIKELY(string->size != 0)) {

        csound->Die(csound,
                     Str("websocket: error output string variable %s must not "
                        "be initialised\nExiting"),
                    argumentArrayItem->name);
      }
      else {

        argumentArrayItem->itemsCount = stringVarMaximumBytesCount;
        string->data = csound->ReAlloc(csound, string->data,
                                       stringVarMaximumBytesCount);
        memset(string->data, 0, stringVarMaximumBytesCount);
      }
    }

    argumentArrayItem->dataPointer = string->data;
    argumentArrayItem->bytesCount = sizeof(char) * stringVarMaximumBytesCount;
    argumentArrayItem->circularBuffer =
      csoundCreateCircularBuffer(csound,
                                 argumentArrayItem->itemsCount *
                                 ringBufferItemsCount + 1,
                                 sizeof(char));
    csound->AuxAlloc(csound, argumentArrayItem->bytesCount,
                     &argumentArrayItem->auxillaryMemory);
    argumentArrayItem->readBuffer = argumentArrayItem->auxillaryMemory.auxp;
}

void WebSocketOpcode_allocateArrayArgument(MYFLT *argument,
                                           OpcodeArgument *argumentArrayItem,
                                           CSOUND *csound)
{
    ARRAYDAT *array = (ARRAYDAT *)argument;

    if (UNLIKELY(array->dimensions == 0)) {

      csound->Die(csound,
                  Str("websocket: error array variable %s has not been "
                      "initialised\nExiting"),
                  argumentArrayItem->name);
    }

    argumentArrayItem->dataPointer = array->data;
    argumentArrayItem->itemsCount = WebSocketOpcode_getArrayElementCount(array);
    argumentArrayItem->bytesCount =
      array->arrayMemberSize * argumentArrayItem->itemsCount;
    argumentArrayItem->circularBuffer =
      csoundCreateCircularBuffer(csound,
                                 argumentArrayItem->itemsCount *
                                 ringBufferItemsCount + 1,
                                 array->arrayMemberSize);
    csound->AuxAlloc(csound, argumentArrayItem->bytesCount,
                     &argumentArrayItem->auxillaryMemory);
    argumentArrayItem->readBuffer = argumentArrayItem->auxillaryMemory.auxp;
}

void WebSocketOpcode_allocateVariableArgument(MYFLT *argument,
                                              OpcodeArgument *argumentArrayItem,
                                              CSOUND *csound, int32_t bytesCount)
{
    argumentArrayItem->dataPointer = argument;
    argumentArrayItem->itemsCount = 1;
    argumentArrayItem->bytesCount = bytesCount;
    argumentArrayItem->circularBuffer =
      csoundCreateCircularBuffer(csound, ringBufferItemsCount + 1,
                                 argumentArrayItem->bytesCount);
    csound->AuxAlloc(csound, argumentArrayItem->bytesCount,
                     &argumentArrayItem->auxillaryMemory);
    argumentArrayItem->readBuffer = argumentArrayItem->auxillaryMemory.auxp;
}

void WebSocketOpcode_initialiseArgumentsArray(CSOUND *csound,
                                              WebSocketOpcode *self,
                                              OpcodeArgument *argumentsArray,
                                              size_t argumentsCount,
                                              MYFLT **arguments,
                                              bool areInputArguments)
{
    uint32_t i;
    for (i = 0; i < argumentsCount; ++i) {

      OpcodeArgument *argumentArrayItem = &argumentsArray[i];
      argumentArrayItem->argumentPointer = arguments[i];
      argumentArrayItem->argumentType =
        WebSocketOpcode_getArgumentType(csound, arguments[i]);
      argumentArrayItem->name =
        areInputArguments ? csound->GetInputArgName((void *)self, i + 1)
        : csound->GetOutputArgName((void *)self, i);

      switch (argumentsArray[i].argumentType) {

      case IRATE_ARRAY:
      case ARATE_ARRAY:
      case KRATE_ARRAY: {

        WebSocketOpcode_allocateArrayArgument(arguments[i],
                                              argumentArrayItem, csound);
        break;
      }
      case STRING_VAR: {

        WebSocketOpcode_allocateStringArgument(arguments[i],
                                               argumentArrayItem, csound,
                                               areInputArguments);
        break;
      }
      case ARATE_VAR: {

        WebSocketOpcode_allocateVariableArgument(arguments[i],
                                         argumentArrayItem, csound,
                                         sizeof(MYFLT) * csound->GetKsmps(csound));
        break;
      }
      case IRATE_VAR:
      case KRATE_VAR: {

        WebSocketOpcode_allocateVariableArgument(arguments[i], argumentArrayItem,
                                                 csound, sizeof(MYFLT));
        break;
      }
      default: {

        csound->Die(csound, "%s", Str("websocket: error, incompatible argument "
                                "detected\nExiting"));
        break;
      }
      }
    }
}

void WebSocketOpcode_initialiseArguments(WebSocketOpcode *self, CSOUND *csound)
{
    self->inputArguments =
      csound->Calloc(csound, sizeof(OpcodeArgument) * self->inputArgumentCount);
    self->outputArguments =
      csound->Calloc(csound, sizeof(OpcodeArgument) * self->outputArgumentCount);

    WebSocketOpcode_initialiseArgumentsArray(csound, self, self->inputArguments,
                                 self->inputArgumentCount,
                                 &self->arguments[self->outputArgumentCount + 1],
                                 true);
    WebSocketOpcode_initialiseArgumentsArray(csound, self, self->outputArguments,
                                             self->outputArgumentCount,
                                             self->arguments, false);
}

void WebSocketOpcode_writeOnce(WebSocketOpcode *self, OpcodeArgument *inputArgument,
                               void *messageBuffer, struct lws *websocket)
{
  IGN(self);
    unsigned char *inputData = (unsigned char *)inputArgument->readBuffer;
    memcpy(messageBuffer, inputData, inputArgument->bytesCount);
    lws_write(websocket, messageBuffer, inputArgument->bytesCount,
              LWS_WRITE_BINARY);
}

void WebSocketOpcode_writeFragments(WebSocketOpcode *self,
                                    OpcodeArgument *inputArgument,
                                    void *messageBuffer, struct lws *websocket)
{
  IGN(self);
    unsigned char *inputData =
      &((unsigned char *)inputArgument->readBuffer)[inputArgument->bytesWritten];

    // *** FIXME *** signed/usigned comparison
    if (inputArgument->bytesWritten + writeBufferBytesCount <
        inputArgument->bytesCount) {

      int32_t writeFlags = LWS_WRITE_NO_FIN;
      writeFlags |=
        inputArgument->bytesWritten==0 ? LWS_WRITE_BINARY : LWS_WRITE_CONTINUATION;
      memcpy(messageBuffer, inputData, writeBufferBytesCount);
      inputArgument->bytesWritten += lws_write(websocket, messageBuffer,
                                               writeBufferBytesCount, writeFlags);
    }
    else {

      size_t fragmentBytesCount =
        inputArgument->bytesCount - inputArgument->bytesWritten;
      memcpy(messageBuffer, inputData, fragmentBytesCount);
      lws_write(websocket, messageBuffer, fragmentBytesCount,
                LWS_WRITE_CONTINUATION);
      inputArgument->bytesWritten = 0;
    }
}

void WebSocketOpcode_writeMessage(WebSocketOpcode *self,
                                  OpcodeArgument *inputArgument,
                                  void *messageBuffer, struct lws *websocket)
{
    if (inputArgument->bytesCount <= writeBufferBytesCount) {

      WebSocketOpcode_writeOnce(self, inputArgument, messageBuffer, websocket);
    }
    else {

      WebSocketOpcode_writeFragments(self, inputArgument, messageBuffer, websocket);
    }
}

void WebSocketOpcode_handleServerWritable(struct lws *websocket,
                                          WebSocketOpcode *self, CSOUND *csound,
                                          void *messageBuffer)
{
    const struct lws_protocols *protocol = lws_get_protocol(websocket);

    if (protocol->id / OUTPUT_OFFSET == 1) { // If it's an output argument

      usleep(100);
      return;
    }

    int32_t inputIndex = 0;

    OpcodeArgument *argument = &self->inputArguments[inputIndex];

    int32_t readItems = 0;

    if (argument->bytesWritten == 0) {

      readItems = csoundReadCircularBuffer(csound, argument->circularBuffer,
                                           argument->readBuffer,
                                           argument->itemsCount);
    }
    if (readItems != 0 || argument->bytesWritten != 0) {

      WebSocketOpcode_writeMessage(self, argument, messageBuffer, websocket);

      if (argument->argumentType == IRATE_VAR ||
          argument->argumentType == IRATE_ARRAY) {

        argument->iRateVarSent = true;
      }
    }

    usleep(100);

    if (argument->iRateVarSent == false) {

      lws_callback_on_writable(websocket);
    }
}

void WebSocketOpcode_handleReceive(struct lws *websocket, WebSocketOpcode *self,
                                   CSOUND *csound, size_t inputDataSize,
                                   void *inputData)
{
    const struct lws_protocols *protocol = lws_get_protocol(websocket);

    if (protocol->id / OUTPUT_OFFSET != 1) {

      return;
    }

    OpcodeArgument *argument = &self->outputArguments[protocol->id - OUTPUT_OFFSET];

    if (argument->iRateVarSent == true) {

      return;
    }

    // *** FIXME *** signed/unsigned comparison
    if (UNLIKELY(argument->bytesCount != inputDataSize
                 &&
                 argument->argumentType != STRING_VAR)) {

      csound->Message(csound, Str("websocket: received message from is not "
                                  "correct size for variable %s, message dumped"),
                      protocol->name);
      return;
    }

    if (UNLIKELY(argument->argumentType == STRING_VAR
                 &&
                 argument->bytesCount > stringVarMaximumBytesCount)) {

      csound->Message(csound, Str("websocket: received string message from "
                                  "%s is too large, message dumped"),
                      protocol->name);

      return;
    }

    int32_t writtenItems =
      csoundWriteCircularBuffer(csound, argument->circularBuffer,
                                inputData, argument->itemsCount);
    argument->receivedData = true;

    if (UNLIKELY(writtenItems == 0)) {

      csound->Message(csound, Str("websocket: received message from %s "
                                  "dumped, buffer overrrun"), argument->name);
    }
    else {

      if (argument->argumentType == IRATE_VAR
          ||
          argument->argumentType == IRATE_ARRAY) {

        argument->iRateVarSent = true;
      }
    }
}


static int32_t Websocket_callback(struct lws *websocket,
                              enum lws_callback_reasons reason,
                              void *user, void *inputData, size_t inputDataSize)
{

   IGN(user);

    if (reason != LWS_CALLBACK_ESTABLISHED &&
        reason != LWS_CALLBACK_SERVER_WRITEABLE && reason != LWS_CALLBACK_RECEIVE) {
      return OK;
    }

    const struct lws_protocols *protocol = lws_get_protocol(websocket);
    WebSocketOpcode *self = lws_get_protocol(websocket)->user;
    CSOUND *csound = self->csound;

    switch (reason) {

    case LWS_CALLBACK_ESTABLISHED: {


      csound->Message(csound, Str("websocket: connection established for %s\n"),
                      protocol->name);
      break;
    }
    case LWS_CALLBACK_SERVER_WRITEABLE: {

      void *messageBuffer =
        (void *)&self->webSocket->messageBuffer[LWS_SEND_BUFFER_PRE_PADDING];

      WebSocketOpcode_handleServerWritable(websocket, self, csound, messageBuffer);
      break;
    }
    case LWS_CALLBACK_RECEIVE: {

      WebSocketOpcode_handleReceive(websocket, self, csound,
                                    inputDataSize, inputData);
      break;
    }
    case LWS_CALLBACK_CLOSED: {

      printf("closed\n");       /* Why not sound->Message ? */
    }
    default: {

      break;
    }
    }

    return OK;
}

uintptr_t WebSocketOpcode_processThread(void *opaquePointer)
{
    WebSocketOpcode *self = opaquePointer;

    while (self->isRunning == 1) {


      lws_service(self->webSocket->context, 10);
      lws_callback_on_writable_all_protocol(self->webSocket->context,
                                            &self->webSocket->protocols[0]);
    }

    return 0;
}

void WebSocketOpcode_initialiseWebSocket(WebSocketOpcode *self, CSOUND *csound)
{
    int32_t i;
    size_t argumentsCount = self->inputArgumentCount + self->outputArgumentCount;

    self->webSocket = csound->Calloc(csound, sizeof(WebSocket));
    self->webSocket->protocols = //Last protocol is null
      csound->Calloc(csound, sizeof(struct lws_protocols) * (argumentsCount + 1));
    size_t argumentIndex = 0;

    for (i = 0; i < self->inputArgumentCount; ++i, argumentIndex++) {

      self->webSocket->protocols[argumentIndex].name = self->inputArguments[i].name;
      self->webSocket->protocols[argumentIndex].callback = Websocket_callback;
      self->webSocket->protocols[argumentIndex].id = INPUT_OFFSET + i;
      self->webSocket->protocols[argumentIndex].user = self;
      self->webSocket->protocols[argumentIndex].per_session_data_size =
        sizeof(WebSocketOpcode *);
    }

    for (i = 0; i < self->outputArgumentCount; ++i, ++argumentIndex) {

      self->webSocket->protocols[argumentIndex].name =
        self->outputArguments[i].name;
      self->webSocket->protocols[argumentIndex].callback = Websocket_callback;
      self->webSocket->protocols[argumentIndex].id = OUTPUT_OFFSET + i;
      self->webSocket->protocols[argumentIndex].user = self;
      self->webSocket->protocols[argumentIndex].per_session_data_size =
        sizeof(WebSocketOpcode *);
    }


    self->webSocket->info.port = *self->arguments[self->outputArgumentCount];
    self->webSocket->info.protocols = self->webSocket->protocols;
    self->webSocket->info.gid = -1;
    self->webSocket->info.uid = -1;

    lws_set_log_level(LLL_DEBUG, NULL);
    self->webSocket->context = lws_create_context(&self->webSocket->info);
    self->webSocket->messageBuffer =
      csound->Calloc(csound, LWS_SEND_BUFFER_PRE_PADDING +
                     (sizeof(char) * writeBufferBytesCount) +
                     LWS_SEND_BUFFER_POST_PADDING);
    if (UNLIKELY(self->webSocket->context == NULL)) {

      csound->Die(csound,
                  "%s", Str("websocket: could not initialise websocket, Exiting"));
    }


    self->isRunning = true;
    self->webSocket->processThread =
      csound->CreateThread(WebSocketOpcode_processThread, self);
}

ArgumentType WebSocketOpcode_getArgumentType(CSOUND *csound, MYFLT *argument)
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

static OENTRY localops[] = {

  {
    .opname = "websocket",
    .dsblksiz = sizeof(WebSocketOpcode),
    .thread = 3,
    .outypes = "*",
    .intypes = "i.",
    .iopadr = (SUBR)WebSocketOpcode_initialise,
    .kopadr = (SUBR)WebSocketOpcode_process,
    .aopadr = NULL
  }

};


LINKAGE
