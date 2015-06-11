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
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <node.h>
#include <string>
#include <v8.h>

using namespace v8;

static CSOUND* csound = 0;
static bool stop_playing = true;
static bool finished = true;
static char *orc = 0;
static char *sco = 0;

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
 * Sets the value of one Csound option. Spaces are not permitted.
 */
void setOption(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    v8::String::Utf8Value option(args[0]->ToString());
    int result = csoundSetOption(csound, *option);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Runs arbitrary JavaScript code in the caller's context.
 */
static double run_javascript(Isolate *isolate, std::string code)
{
    Handle<String> source = String::NewFromUtf8(isolate, code.c_str());
    Handle<Script> script = Script::Compile(source);
    Handle<Value> result = script->Run();
    return result->NumberValue();
}

/**
 * Compiles the CSD file, and also parses out the <html> element
 * and loads it into NW.js.
 */
void compileCsd(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    //csoundCreateMessageBuffer(csound, 1);
    int result = 0;
    v8::String::Utf8Value csd_path(args[0]->ToString());
    std::ifstream csd_file(*csd_path);
    if (csd_file.good()) {
        std::string csd_text((std::istreambuf_iterator<char>(csd_file)),
                             std::istreambuf_iterator<char>());
        csd_file.close();
        size_t html_start = csd_text.find("<html");
        if (html_start != std::string::npos) {
            size_t html_end = csd_text.find("</html>", html_start);
            if (html_end != std::string::npos) {
                std::string html_text = csd_text.substr(html_start, html_end - html_start + 7);
                std::string html_path = *csd_path;
                html_path += ".html";
                std::ofstream html_file(html_path.c_str(), std::ios_base::out | std::ios_base::binary);
                if (html_file.good()) {
                    html_file.write(html_text.c_str(), html_text.size());
                    html_file.close();
                }
                run_javascript(isolate, "location = '" + html_path + "';");
            }
        }
    }
    result = csoundCompileCsd(csound, *csd_path);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Compiles the orchestra code.
 */
void compileOrc(const FunctionCallbackInfo<Value>& args)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    //csoundCreateMessageBuffer(csound, 1);
    v8::String::Utf8Value orchestraCode(args[0]->ToString());
    orc = strdup(*orchestraCode);
    int result = csoundCompileOrc(csound, orc);
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
    double value = args[1]->ToNumber()->Value();
    csoundSetControlChannel(csound, *channelName, value);
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
    double value = csoundGetControlChannel(csound, *channelName, &result);
    args.GetReturnValue().Set(Number::New(isolate, value));
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

void csoundMessageCallback_(CSOUND *csound, int attr, const char *format, va_list valist)
{
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    char buffer[0x1000];
    std::vsprintf(buffer, format, valist);
    Local<v8::Value> args[] = { String::NewFromUtf8(isolate, buffer) };
    Local<Function> local_function = Local<Function>::New(isolate, console_function(isolate));
    local_function->Call(isolate->GetCallingContext()->Global(), 1, args);
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
    bool playing = ((stop_playing == false) && (finished == false));
    args.GetReturnValue().Set(Number::New(isolate, playing) );
}

static void consume_messages(Isolate *isolate)
{
    char buffer[0x1002];
    int pending_messages = csoundGetMessageCnt(csound);
    for (int i = 0, n = csoundGetMessageCnt(csound);  i < n; ++i) {
        const char *message = csoundGetFirstMessage(csound);
        Local<v8::Value> args[] = { String::NewFromUtf8(isolate, message) };
        Local<Function> local_function = Local<Function>::New(isolate, console_function(isolate));
        local_function->Call(isolate->GetCallingContext()->Global(), 1, args);
        csoundPopFirstMessage(csound);
    }
}

/**
 * Begins performing the score and/or producing audio.
 * It is first necessary to call compileCsd(pathname) or compileOrc(text).
 * Returns the native handle of the performance thread.
 */
void perform(const FunctionCallbackInfo<Value>& args)
{
    csoundMessage(csound, "Began JavaScript perform()...\n");
    Isolate* isolate = Isolate::GetCurrent();
    HandleScope scope(isolate);
    //consume_messages(isolate);
    csoundStart(csound);
    int result = 0;
    for (stop_playing = false, finished = false;
            ((stop_playing == false) && (finished == false)); ) {
        for (int i = 0; i < 100; ++i) {
            if (uv_run(uv_default_loop(), UV_RUN_NOWAIT) == 0) {
                break;
            }
            //consume_messages(isolate);
        }
        finished = csoundPerformBuffer(csound);
    }
    csoundMessage(csound, "Ended JavaScript perform(), cleaning up now.\n");
    result = csoundCleanup(csound);
    //consume_messages(isolate);
    csoundReset(csound);
    //csoundDestroyMessageBuffer(csound);
    args.GetReturnValue().Set(Number::New(isolate, result));
}

/**
 * Stops any ongoing Csound performance.
 */
void stop(const FunctionCallbackInfo<Value>& args)
{
    stop_playing = true;
}

void init(Handle<Object> target)
{
    csound = csoundCreate(0);
    csoundSetMessageCallback(csound, csoundMessageCallback_);
    NODE_SET_METHOD(target, "hello", hello);
    NODE_SET_METHOD(target, "getVersion", getVersion);
    NODE_SET_METHOD(target, "setOption", setOption);
    NODE_SET_METHOD(target, "compileCsd", compileCsd);
    NODE_SET_METHOD(target, "compileOrc", compileOrc);
    NODE_SET_METHOD(target, "evalCode", evalCode);
    NODE_SET_METHOD(target, "readScore", readScore);
    NODE_SET_METHOD(target, "setControlChannel", setControlChannel);
    NODE_SET_METHOD(target, "getControlChannel", getControlChannel);
    NODE_SET_METHOD(target, "message", message);
    NODE_SET_METHOD(target, "getSr", getSr);
    NODE_SET_METHOD(target, "getKsmps", getKsmps);
    NODE_SET_METHOD(target, "getNchnls", getNchnls);
    NODE_SET_METHOD(target, "isPlaying", isPlaying);
    NODE_SET_METHOD(target, "perform", perform);
    NODE_SET_METHOD(target, "stop", stop);
}

NODE_MODULE(binding, init);
