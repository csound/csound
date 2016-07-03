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

//#include <Csound.hxx>
#include <csound.h>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <node.h>
#include <string>
#include <vector>
#include <uv.h>
#include <v8.h>
#if defined(_MSC_VER)
#include <concurrent_queue.h>
#else
#include <boost/lockfree/queue.hpp>
#endif

using namespace v8;

static CSOUND* csound_ = 0;
static bool stop_playing = true;
static bool finished = true;
static bool paused = false;
static char *orc = 0;
static char *sco = 0;
static uv_thread_t uv_csound_perform_thread;
static uv_async_t uv_csound_message_async;

//static csound::CSound Csound;

struct ScoreEvent {
    char opcode;
    std::vector<MYFLT> pfields;
};

#if defined(_MSC_VER)
static concurrency::concurrent_queue<char *> csound_messages_queue;
static concurrency::concurrent_queue<char *> csound_score_queue;
static concurrency::concurrent_queue<ScoreEvent *> csound_event_queue;
#else
static boost::lockfree::queue<char *, boost::lockfree::fixed_sized<false> > csound_messages_queue(0);
static boost::lockfree::queue<char *, boost::lockfree::fixed_sized<false> > csound_score_queue(0);
static boost::lockfree::queue<ScoreEvent *, boost::lockfree::fixed_sized<false> > csound_event_queue(0);
#endif

/**
 * Compiles the CSD file.
 */
void compileCsd(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    //csoundCreateMessageBuffer(csound_, 1);
    int result = 0;
    v8::String::Utf8Value csd_path(args[0]->ToString());
    result = csoundCompileCsd(csound_, *csd_path);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Compiles the orchestra code.
 */
void compileOrc(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    //csoundCreateMessageBuffer(csound_, 1);
    v8::String::Utf8Value orchestraCode(args[0]->ToString());
    orc = strdup(*orchestraCode);
    int result = csoundCompileOrc(csound_, orc);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

static Persistent<Function, CopyablePersistentTraits<Function>> console_function(Isolate *isolate)
{
    static Persistent<Function, CopyablePersistentTraits<Function>> function;
    static bool initialized = false;
    if (initialized == false) {
        initialized = true;
        auto code = String::NewFromUtf8(isolate, "(function(arg) {\n\
            console.log(arg);\n\
        })");
        auto result = Script::Compile(code)->Run();
        auto function_handle = Handle<Function>::Cast(result);
        function.Reset(isolate, function_handle);
    }
    return function;
}

void csoundMessageCallback_(CSOUND *csound_, int attr, const char *format, va_list valist)
{
    char buffer[0x1000];
    std::vsprintf(buffer, format, valist);
    // Actual data...
    csound_messages_queue.push(strdup(buffer));
    // ... and notification that data is ready.
    uv_async_send(&uv_csound_message_async);
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
    double result = csoundEvalCode(csound_, *orchestraCode);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Returns the numerical value of the named Csound control channel.
 */
void getControlChannel(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value channelName(args[0]->ToString());
    int result = 0;
    double value = csoundGetControlChannel(csound_, *channelName, &result);
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
    double value = csoundGetKsmps(csound_);
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
    double value = csoundGetNchnls(csound_);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns Csound's current sampling rate.
 */
void getSr(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    double value = csoundGetSr(csound_);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

/**
 * Returns the time in seconds from the beginning of performance.
 */
void getScoreTime(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    double value = csoundGetScoreTime(csound_);
    args.GetReturnValue().Set(Number::New(isolate, value));
}

void getVersion(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    int version = csoundGetVersion();
    args.GetReturnValue().Set(Number::New(isolate, version));
}

/**
 * This is provided so that the developer may verify that
 * the "csound" object exists in his or her JavaScript context.
 */
void hello(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    char buffer[0x100];
    std::sprintf(buffer, "Hello, world! This is Csound 0x%p.", csound_);
    args.GetReturnValue().Set(String::NewFromUtf8(isolate, buffer));
}

/**
 * Evaluates the string of text, which may main contain multiple lines,
 * as a Csound score for immediate performance. The score is assumed
 * to be presorted.
 */
void inputMessage(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value scoreLines(args[0]->ToString());
    csound_score_queue.push(strdup(*scoreLines));
    args.GetReturnValue().Set(Number::New(isolate, 0));
}

/**
 * Returns 1 if Csound is currently playing (synthesizing score
 * events, or 0 otherwise.
 */
void isPlaying(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    bool playing = ((stop_playing == false) && (finished == false));
    args.GetReturnValue().Set(Number::New(isolate, playing) );
}

void isScorePending(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    bool is_pending = csoundIsScorePending(csound_);
    args.GetReturnValue().Set(Boolean::New(isolate, is_pending));
}

/**
 * Sends text as a message to Csound, for printing if printing is enabled.
 */
void message(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value text(args[0]->ToString());
    csoundMessage(csound_, *text);
}

void on_exit()
{
    uv_close((uv_handle_t *)&uv_csound_message_async, 0);
}

void pause(const FunctionCallbackInfo<Value>& args)
{
    paused = true;
}

void uv_csound_perform_thread_routine(void * arg);

/**
 * Begins performing the score and/or producing audio.
 * It is first necessary to call compileCsd(pathname) or compileOrc(text).
 * Returns the native handle of the performance thread.
 */
void perform(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    int result = uv_thread_create(&uv_csound_perform_thread, uv_csound_perform_thread_routine, csound_);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Evaluates the string of text, which may main contain multiple lines,
 * as a Csound score for immediate performance.
 */
void readScore(const FunctionCallbackInfo<Value>& args)
{
    int result = 0;
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value scoreLines(args[0]->ToString());
    csound_score_queue.push(strdup(*scoreLines));
    args.GetReturnValue().Set(Number::New(isolate, result));
}

void resume(const FunctionCallbackInfo<Value>& args)
{
    paused = false;
}

void rewindScore(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    csoundRewindScore(csound_);
}

/**
 * Runs arbitrary JavaScript code in the caller's context.
 */
double run_javascript(Isolate *isolate, std::string code)
{
    Handle<String> source = String::NewFromUtf8(isolate, code.c_str());
    Handle<Script> script = Script::Compile(source);
    Handle<Value> result = script->Run();
    return result->NumberValue();
}

/**
 * Evaluates a singisle score event, sent as opcode and pfields,
 * relative to the current performance time. The number of pfields
 * is read from the length of the array, not from the Csound API
 * parameter.
 */
void scoreEvent(const FunctionCallbackInfo<Value>& args)
{
    int result = 0;
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value javascript_opcode(args[0]->ToString());
    ScoreEvent *event = new ScoreEvent;
    char *opcode_ = *javascript_opcode;
    event->opcode = opcode_[0];
    v8::Local<v8::Array> javascript_pfields = v8::Local<v8::Array>::Cast(args[1]);
    int javascript_pfields_count = javascript_pfields->Length();
    for(int i = 0; i < javascript_pfields_count; i++) {
        v8::Local<v8::Value> element = javascript_pfields->Get(i);
        event->pfields.push_back(element->NumberValue());
    }
    csound_event_queue.push(event);
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
    double value = args[1]->ToNumber()->Value();
    csoundSetControlChannel(csound_, *channelName, value);
}

/**
 * Sets the value of one Csound option. Spaces are not permitted.
 */
void setOption(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value option(args[0]->ToString());
    int result = csoundSetOption(csound_, *option);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

void setScorePending(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    bool is_pending = args[0]->BooleanValue();
    csoundSetScorePending(csound_, is_pending);
}

/**
 * Stops any ongoing Csound performance.
 */
void stop(const FunctionCallbackInfo<Value>& args)
{
    stop_playing = true;
}

void uv_csound_message_callback(uv_async_t *handle)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    char *message;
#if defined(_MSC_VER)
    while (csound_messages_queue.try_pop(message)) {
#else
    while (csound_messages_queue.pop(message)) {
#endif
        Local<v8::Value> args[] = { String::NewFromUtf8(isolate, message) };
        Local<Function> local_function = Local<Function>::New(isolate, console_function(isolate));
        local_function->Call(isolate->GetCurrentContext()->Global(), 1, args);
        std::free(message);
    }
}

void uv_csound_perform_thread_routine(void * arg)
{
    csoundMessage(csound_, "Began JavaScript perform()...\n");
    csoundStart(csound_);
    int result = 0;
    ScoreEvent *event = 0;
    char *score_text = 0;
    for (stop_playing = false, finished = false, paused = false;
            ((stop_playing == false) && (finished == false)); ) {
        if (paused == false) {
#if defined(_MSC_VER)
            while (csound_event_queue.try_pop(event)) {
#else
            while (csound_event_queue.pop(event)) {
#endif
                csoundScoreEvent(csound_, event->opcode, event->pfields.data(), event->pfields.size());
                delete event;
            }
#if defined(_MSC_VER)
            while (csound_score_queue.try_pop(score_text)) {
#else
            while (csound_score_queue.pop(score_text)) {
#endif
                csoundReadScore(csound_, score_text);
                free(score_text);
            }
            finished = csoundPerformKsmps(csound_);
        }
    }
    csoundMessage(csound_, "Ended JavaScript perform(), cleaning up now.\n");
    result = csoundCleanup(csound_);
    csoundReset(csound_);
#if defined(_MSC_VER)
    while (csound_event_queue.try_pop(event)) {
#else
    while (csound_event_queue.pop(event)) {
#endif
        delete event;
    }
#if defined(_MSC_VER)
    while (csound_score_queue.try_pop(score_text)) {
#else
    while (csound_score_queue.pop(score_text)) {
#endif
        free(score_text);
    }
}

void init(Handle<Object> target)
{
    csound_ = csoundCreate(0);
    csoundSetMessageCallback(csound_, csoundMessageCallback_);
    // Keep these in alphabetical order.
    NODE_SET_METHOD(target, "compileCsd", compileCsd);
    NODE_SET_METHOD(target, "compileOrc", compileOrc);
    NODE_SET_METHOD(target, "getControlChannel", getControlChannel);
    NODE_SET_METHOD(target, "getKsmps", getKsmps);
    NODE_SET_METHOD(target, "getNchnls", getNchnls);
    NODE_SET_METHOD(target, "getScoreTime", getScoreTime);
    NODE_SET_METHOD(target, "getSr", getSr);
    NODE_SET_METHOD(target, "getVersion", getVersion);
    NODE_SET_METHOD(target, "hello", hello);
    NODE_SET_METHOD(target, "inputMessage", inputMessage);
    NODE_SET_METHOD(target, "isPlaying", isPlaying);
    NODE_SET_METHOD(target, "isScorePending", isScorePending);
    NODE_SET_METHOD(target, "message", message);
    NODE_SET_METHOD(target, "perform", perform);
    NODE_SET_METHOD(target, "pause", pause);
    NODE_SET_METHOD(target, "readScore", readScore);
    NODE_SET_METHOD(target, "resume", resume);
    NODE_SET_METHOD(target, "rewindScore", rewindScore);
    NODE_SET_METHOD(target, "scoreEvent", scoreEvent);
    NODE_SET_METHOD(target, "setControlChannel", setControlChannel);
    NODE_SET_METHOD(target, "setOption", setOption);
    NODE_SET_METHOD(target, "setScorePending", setScorePending);
    NODE_SET_METHOD(target, "stop", stop);
    uv_async_init(uv_default_loop(), &uv_csound_message_async, uv_csound_message_callback);
    std::atexit(&on_exit);
}

NODE_MODULE(binding, init);
