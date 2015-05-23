/**
 * A Node.js (and io.js and NW.js) binding for Csound. This interface should
 * mirror the Csound JavaScript interface for the Chromium Embedded Framework
 * and Android. In JavaScript environments, Csound has already been
 * instantiated and initialized, and is named "csound" in the user's
 * JavaScript context.
 *
 * int getVersion ()
 * int compileCsd (String pathname)
 * void compileOrc (String orchestracode)
 * double evalCode (String orchestracode)
 * void readScore (String scorelines)
 * void setControlChannel (String channelName, double value)
 * double getControlChannel (String channelName)
 * void message (String text)
 * int getSr ()
 * int getKsmps ()
 * int getNchnls ()
 * int isPlaying ()
 * int play ()
 * int stop ()
 *
 * Copyright (C) 2015 by Michael Gogins.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Must do this on Windows: https://connect.microsoft.com/VisualStudio/feedback/details/811347/compiling-vc-12-0-with-has-exceptions-0-and-including-concrt-h-causes-a-compiler-error

#include <csound.h>
#include <node.h>
#include <memory>
#include <string>
#include <thread>
#include <v8.h>

using namespace v8;

static CSOUND* csound = 0;
static bool stop = true;
static bool finished = true;
static std::shared_ptr<std::thread> threadptr;

/**
 * This is provided so that the developer may verify that
 * the "csound" object exists in his or her JavaScript context.
 */
void hello(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    char buffer[0x100];
    std::sprintf(buffer, "Hello, world! This is Csound 0x%p.", csound);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, buffer));
}

void getVersion(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    int version = csoundGetVersion();
    args.GetReturnValue().Set(Number::New(isolate, version));
}

/**
 * Compiles the CSD file, and also parses out the <html> element
 * and loads it into NW.js.
 */
void compileCsd(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value csdpath(args[0]->ToString());
    int result = csoundCompileCsd(csound, *csdpath);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Compiles the orchestra code, and also parses out the <html>
 * element and and loads it into NW.js.
 */
void compileOrc(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value orchestraCode(args[0]->ToString());
    int result = csoundCompileOrc(csound, *orchestraCode);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Evaluates the orchestra code as an expression, and returns its value
 * as a number.
 */
void evalCode(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value orchestraCode(args[0]->ToString());
    double result = csoundEvalCode(csound, *orchestraCode);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Evaluates the string of text, which may main contain multiple lines,
 * as a Csound score for immediate performance.
 */
void readScore(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value scoreLines(args[0]->ToString());
    int result = csoundReadScore(csound, *scoreLines);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Sets the numerical value of the named Csound control channel.
 */
void setControlChannel(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value channelName(args[0]->ToString());
    double value = csoundSetControlChannel(csound, *channelName, value);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns the numerical value of the named Csound control channel.
 */
void setControlChannel(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value channelName(args[0]->ToString());
    double value = csoundSetControlChannel(csound, *channelName, value);
}

/**
 * Sends text as a message to Csound, for printing if printing is enabled.
 */
void message(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value text(args[0]->ToString());
    csoundMessage(csound, *text);
}

/**
 * Returns Csound's current sampling rate.
 */
void getSr(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    double value = csoundGetSr(csound);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns the current number of sample frames per kperiod
 * in the current Csound performance.
 */
void getKsmps(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    double value = csoundGetKsmps(csound);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns the number of audio output channels
 * in the current Csound performance.
 */
void getNchnls(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    double value = csoundGetNchnls(csound);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns 1 if Csound is currently playing (synthesizing score
 * events, or 0 otherwise.
 */
void isPlaying(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    bool playing = ((stop == false) && (finished == false));
    args.GetReturnValue().Set(Number::New(isolate, playing) );
}

static int play_routine(CSOUND *csound)
{
    int result = 0;
    for (stop = false, finished = false;
         ((stop == false) && (finished == false)); )
    {
        finished = csoundPerformBuffer(csound);
    }
    result = csoundCleanup(csound);
    csoundReset(csound);
}

/**
 * Begins performing the score and/or producing audio.
 * It is first necessary to call compileCsd(pathname) or compileOrc(text).
 * If Csound is already performing, it is first stopped.
 * The performance occurs in a separate, internal thread.
 */
void play(const FunctionCallbackInfo<Value>& args)
{
    if (threadptr->is_joinable()) {
        stop();
        threadptr->join();
    }
    threadptr = new std::thread(play_routine, csound);
}

/**
 * Stops any ongoing Csound performance.
 */
void stop(const FunctionCallbackInfo<Value>& args)
{
    stop = true;
}

void init(Handle<Object> target)
{
    csound = csoundCreate(0);
    NODE_SET_METHOD(target, "hello", hello);
    NODE_SET_METHOD(target, "getVersion", getVersion);
    NODE_SET_METHOD(target, "compileCsd", compileCsd);
}

NODE_MODULE(binding, init);
