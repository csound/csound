/*
    csound_oboe.hpp:

    Copyright (C) 2017 Michael Gogins

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

    As a special exception, if other files instantiate templates or
    use macros or inline functions from this file, this file does not
    by itself cause the resulting executable or library to be covered
    by the GNU Lesser General Public License. This exception does not
    however invalidate any other reasons why the library or executable
    file might be covered by the GNU Lesser General Public License.
*/

#ifndef __CSOUND_OBOE_HPP__
#define __CSOUND_OBOE_HPP__

#if defined(__GNUC__)
#if __cplusplus <= 199711L
  #error To use csound_threaded.hpp you need at least a C++11 compliant compiler.
#endif
#endif

#ifdef SWIG
%module csound_oboe
%{
#include "csound.hpp"
%}
%pragma(java) jniclassimports = %{
import android.webkit.JavascriptInterface;
%}
%typemap(javaimports) CsoundOboe %{
import android.webkit.JavascriptInterface;
%}
#else
#include "csound.hpp"
#include <oboe/Oboe.h>
#endif

/**
 * The purpose of this class is to expose as much as possible of the C++ form 
 * of the Csound API found in the Csound class as a Java object in the 
 * JavaScript context of the WebKit WebView in Android applications, such as 
 * the Csound for Android app; and, to do this in a form that closely follows 
 * the API signatures and the Csound performance lifecycle defined in the 
 * CsoundThreaded class that is similarly exposed in csound.node and CsoundQt.
 * The Google Oboe library (https://github.com/google/oboe) is used to 
 * interface with the Android audio driver.
 */
 
#ifdef SWIG

// All methods of the CsoundOboe object to be exposed to the JavaScript context 
// must have their generated Java method signatures annotated here with 
// "@JavascriptInterface". Please keep these in alphabetical order.
// This should expose the "core" of the Csound API without futher ado in a 
// manner that is consistent with Csound's JavaScript interface on other HTML5 
// environments.

%javamethodmodifiers CsoundOboe::cleanup() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::compileCsd(const char *csd) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::compileCsdText(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::compileOrc(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::evalCode(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::get0dBFS() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getAPIVersion() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getVersion() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getAudioChannel(const char *name, MYFLT *samples)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getChannel(const char *name)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getControlChannel(const char *name)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getCurrentTimeSamples()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getEnv(const char *name)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getKr()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getKsmps()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getMessageLevel()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getNchnls()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getNchnlsInput()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getOutputName()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getScoreOffsetSeconds()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getScoreTime()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getSr()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getStringChannel(const char *name, char *string)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::getVersion()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::inputMessage(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::isScorePending()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::keyPressed(char c)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::message(const char *text)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::perform()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::readScore(const char *text)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::reset()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::rewindScore()
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setControlChannel(const char *name, double value)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setGlobalEnv(const char *name, const char *value)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setInput(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setOption(const char *text) 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setOutput(const char *name,const char *type,const char *format)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setScoreOffsetSeconds(double time)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setScorePending(int pending)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::setStringChannel(const char *name, char *string)
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::start() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::stop() 
%{@JavascriptInterface 
public 
%}  
%javamethodmodifiers CsoundOboe::tableGet(int table, int index)
%{@JavascriptInterface 
public 
%}
%javamethodmodifiers CsoundOboe::tableLength(int table)
%{@JavascriptInterface 
public 
%}
%javamethodmodifiers CsoundOboe::tableSet(int table, int index, double value)
%{@JavascriptInterface 
public 
%}
%javamethodmodifiers CsoundOboe::isPlaying()
%{@JavascriptInterface 
public 
%}
#endif

class PUBLIC CsoundOboe : public Csound, public oboe::AudioStreamCallback
{
public:
    CsoundOboe() : 
        timeout_nanoseconds(1000000),
        frames_per_kperiod(0),
        is_playing(false),
        audio_stream_in(0),
        spin(0),
        input_channel_count(0),
        spin_size(0),
        spout(0),
        output_channel_count(0),
        spout_size(0),
        audio_stream_out(0),
        short_buffer(0),
        float_buffer(0),
        zerodBFS(1)
    {
    }
    virtual ~CsoundOboe()
    {
    }
    /** 
     * This is the Oboe audio callback. It fires when the audio output stream 
     * needs audio data, i.e. once per kperiod. The audio _input_ stream is 
     * read in a blocking fashion.
     */
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream,
            void *audio_data,
            int32_t frame_count)
    {
        int csound_result = 0;
        int frames_read = 0;
        if (input_channel_count > 0 && audio_stream_in) {
            frames_read = audio_stream_in->read(audio_data, frame_count, timeout_nanoseconds);
            // If ksmps input audio frames have not yet been consumed, tell the 
            // Oboe driver to continue.
            if (frames_read == frame_count) {
                return oboe::DataCallbackResult::Continue;
            }
            // Otherwise, copy the input stream audio to Csound.
            // The buffer type may differ by platform.
            if (frames_read >= 0) {
                if (oboe_audio_format == oboe::AudioFormat::Float){
                    float_buffer = static_cast<float *>(audio_data);
                    for (int i = 0; i < frames_per_kperiod; i++) {
                        for (int j = 0; j < output_channel_count; j++) {
                            spin[i * output_channel_count + j] = float_buffer[i * output_channel_count + j];
                        }
                    }
                } else {
                    short_buffer = static_cast<int16_t *>(audio_data);
                    for (int i = 0; i < frames_per_kperiod; i++) {
                        for (int j = 0; j < output_channel_count; j++) {
                            spin[i * output_channel_count + j] = short_buffer[i * output_channel_count + j] * 32767;
                        }
                    }
                }            
            }
        }
        // Consume one kperiod of audio input from spin,
        // and produce one kperiod of audio output to spout.
        csound_result = PerformKsmps();
        // If the Csound performance has finished, tell the Oboe driver 
        // to stop.
        if (csound_result) {
            is_playing = false;
            return oboe::DataCallbackResult::Stop;
            audio_stream_out->requestStop();
        }
        // Otherwise, copy the Csound output audio to the Oboe output 
        // buffer. The buffer type may differ by platform.
        if (oboe_audio_format == oboe::AudioFormat::Float){
            float_buffer = static_cast<float *>(audio_data);
            for (int i = 0; i < frames_per_kperiod; i++) {
                for (int j = 0; j < output_channel_count; j++) {
                    float_buffer[i * output_channel_count + j] = spout[i * output_channel_count + j];
                }
            }
        } else {
            short_buffer = static_cast<int16_t *>(audio_data);
            for (int i = 0; i < frames_per_kperiod; i++) {
                for (int j = 0; j < output_channel_count; j++) {
                    short_buffer[i * output_channel_count + j] = spout[i * output_channel_count + j] * 32767;
                }
            }
        }            
        return oboe::DataCallbackResult::Continue;
    }
    virtual int Start()
    {
        Message("CsoundOboe::Start...\n");
        // If and only if -odac, enable host-implemented audio.
        std::string output_name = GetOutputName();
        auto position = output_name.find("dac");
        int csound_result = 0;
        if (position != std::string::npos) {
            zerodBFS = Get0dBFS();
            SetHostImplementedAudioIO(1, 0);
            csound_result = Csound::Start();
            if (csound_result != 0) {
                Message("Csound::Start returned: %d.\n", csound_result);
                return csound_result;
            }
            oboe::Result result;
            // Create the oboe input and output streams.
            audio_stream_builder.setCallback(this);
            audio_stream_builder.setSharingMode(oboe::SharingMode::Exclusive);
            audio_stream_builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
            audio_stream_builder.setSampleRate(GetSr());
            frames_per_kperiod = GetKsmps();
            audio_stream_builder.setFramesPerCallback(frames_per_kperiod);
            if (input_channel_count > 0) {
                spin = GetSpin();
                input_channel_count = GetNchnlsInput();
                spin_size = sizeof(MYFLT) * frames_per_kperiod * input_channel_count;
                audio_stream_builder.setChannelCount(input_channel_count);
                audio_stream_builder.setDirection(oboe::Direction::Input);
                result = audio_stream_builder.openStream(&audio_stream_in);
                if (result != oboe::Result::OK){
                    Message("CsoundOboe::Start: Failed to create Oboe input stream stream. Error: %s.\n", oboe::convertToText(result));
                    return -1;
                } 
                // We assume that Oboe's input format is always the same as
                // its output format! But input and output may open without 
                // the other.
                oboe_audio_format = audio_stream_in->getFormat();
                Message("CsoundOboe::Start: Audio input stream format is: %s.\n", oboe::convertToText(oboe_audio_format));           
                Message("CsoundOboe::Start: Starting Oboe audio input stream...\n");
                // Is this correct?
                audio_stream_in->start();
            }
            spout = GetSpout();
            output_channel_count = GetNchnls();
            spout_size = sizeof(MYFLT) * frames_per_kperiod * output_channel_count;
            audio_stream_builder.setChannelCount(output_channel_count);
            audio_stream_builder.setDirection(oboe::Direction::Output);
            result = audio_stream_builder.openStream(&audio_stream_out);
            if (result != oboe::Result::OK){
                Message("CsoundOboe::Start: Failed to create Oboe output stream stream. Error: %s.\n", oboe::convertToText(result));
                return -1;
            }                
            // Start oboe.
            oboe_audio_format = audio_stream_out->getFormat();
            Message("CsoundOboe::Start: Audio output stream format is: %s.\n", oboe::convertToText(oboe_audio_format));           
            Message("CsoundOboe::Start: Starting Oboe audio output stream...\n");
            audio_stream_out->requestStart();
        } else {
            csound_result = Csound::Start();
            if (csound_result != 0) {
                Message("Csound::Start returned: %d.\n", csound_result);
                return csound_result;
            }
        }
        // Start Csound.
        is_playing = true;
        return 0;
    }
    /**
     * Because Oboe is driving the performance, this is a dummy.
     */
    virtual int Perform()
    {
        Message("CsoundOboe::Perform...\n");
        return 0;
    }
    virtual void Stop()
    {
        Message("CsoundOboe::Stop...\n");
        if (audio_stream_in) {
            audio_stream_in->requestStop();
            audio_stream_in = 0;
        }
        if (audio_stream_out) {
            audio_stream_out->requestStop();
            audio_stream_out = 0;
        }
        Csound::Stop();
    }
    // For the purpose of making the Csound API consistent across all of the 
    // JavaScript-enabled platforms supported by Csound, all methods exposed 
    // to the JavaScript context are redeclared here in camel case, and are 
    // implemented by delegating to the real definitions declared with initial 
    // caps. These methods also are annotated for exposure to WebKit by SWIG 
    // at the head of this file. NOTE: Please keep these methods in 
    // alphabetical order. Also, it is IMPERATIVE to keep the semantics 
    // completely consistent with csound.hpp.
    virtual int cleanup(){
        return Cleanup();
    }
    virtual int compileCsd(const char *csd){
        return CompileCsd(csd);
    }
    virtual int compileCsdText(const char *csd_text){
        return CompileCsdText(csd_text);
    }
    virtual int compileOrc(const char *str){
        return CompileOrc(str);
    }
    virtual MYFLT evalCode(const char *str){
        return EvalCode(str);
    }
    virtual MYFLT get0dBFS(){
        return Get0dBFS();
    }
    virtual int getAPIVersion(){
        return GetAPIVersion();
    }
    virtual void getAudioChannel(const char *name, MYFLT *samples){
        GetAudioChannel(name,samples);
    }
    virtual MYFLT getChannel(const char *name){
        return GetControlChannel(name, 0);
    }
    virtual MYFLT getControlChannel(const char *name){
        return GetControlChannel(name, 0);
    }
    virtual long getCurrentTimeSamples(){
        return GetCurrentTimeSamples();
    }
    virtual const char *getEnv(const char *name){
        return GetEnv(name);
    }
    virtual MYFLT getKr(){
        return GetKr();
    }
    virtual int getKsmps(){
        return GetKsmps();
    }
    virtual int getMessageLevel(){
        return GetMessageLevel();
    }
    virtual int getNchnls(){
        return GetNchnls();
    }
    virtual int getNchnlsInput(){
        return GetNchnlsInput();
    }
    virtual const char *getOutputName(){
        return GetOutputName();
    }
    virtual MYFLT getScoreOffsetSeconds(){
        return GetScoreOffsetSeconds();
    }
    virtual double getScoreTime(){
        return GetScoreTime();
    }
     virtual MYFLT getSr(){
        return GetSr();
    }
    virtual void getStringChannel(const char *name, char *string){
        GetStringChannel(name,string);
    }
    virtual int getVersion(){
        return GetVersion();
    }
    virtual void inputMessage(const char *message){
        InputMessage(message);
    }
    virtual int isScorePending(){
        return IsScorePending();
    }
    virtual void keyPressed(char c){
        KeyPressed(c);
    }
    virtual void message(const char *text){
        Message(text);
    }
    virtual int perform(){
        return Perform();
    }
    virtual int pvsinSet(const PVSDATEXT* value, const char *name){
        return PvsinSet(value, name);
    }
    virtual int pvsoutGet(PVSDATEXT* value, const char *name){
        return PvsoutGet(value, name);
    }
    virtual int readScore(const char *str){
        return ReadScore(str);
    }
    virtual void reset(){
        Reset();
    }
    virtual void rewindScore(){
        RewindScore();
    }
    virtual void setInput(const char *name){
        SetInput(name);
    }
    virtual int setGlobalEnv(const char *name, const char *value){
        return SetGlobalEnv(name, value);
    }
    virtual void setMessageLevel(int messageLevel){
        SetMessageLevel(messageLevel);
    }
    virtual void setMIDIFileInput(const char *name){
        SetMIDIFileInput(name);
    }
    virtual void setMIDIFileOutput(const char *name){
        SetMIDIFileOutput(name);
    }
    virtual void setMIDIInput(const char *name){
        SetMIDIInput(name);
    }
    virtual void setMIDIOutput(const char *name){
        SetMIDIOutput(name);
    }
    virtual int setOption(const char *option){
        return SetOption(option);
    }
    virtual void setOutput(const char *name,const char *type,const char *format){
        SetOutput(name, type, format);
    }
    virtual void setScoreOffsetSeconds(double time){
        SetScoreOffsetSeconds((MYFLT) time);
    }
    virtual void setScorePending(int pending){
        SetScorePending(pending);
    }
    virtual int start(){
        return Start();
    }
    virtual void stop(){
        Stop();
    }
    virtual int scoreEvent(char type, const MYFLT *pFields, long numFields){
        return ScoreEvent(type, pFields, numFields);
    }
    virtual int runUtility(const char *name, int argc, char **argv)
    {
        return RunUtility(name, argc, argv);
    }
    virtual void setAudioChannel(const char *name, MYFLT *samples){
        SetChannel(name, samples);
    }
    virtual void setChannel(const char *name, double value){
        SetControlChannel(name, value);
    }
    virtual void setControlChannel(const char *name, double value){
        SetControlChannel(name, value);
    }
    virtual void setChannel(const char *name, char *string){
        SetStringChannel(name, string);
    }
    virtual void setStringChannel(const char *name, char *string){
        SetStringChannel(name, string);
    }
    virtual int tableLength(int table){
        return TableLength(table);
    }
    virtual MYFLT tableGet(int table, int index){
        return TableGet(table, index);
    }
    virtual void tableSet(int table, int index, double value){
        TableSet(table, index, (MYFLT) value);
    }
protected:
    int timeout_nanoseconds;
    uint32_t frames_per_kperiod;
    std::atomic<bool> is_playing;
    oboe::AudioStream *audio_stream_in;
    MYFLT *spin;
    uint32_t input_channel_count;
    size_t spin_size;
    MYFLT *spout;
    uint32_t output_channel_count;
    size_t spout_size;
    oboe::AudioStream *audio_stream_out;
    int16_t *short_buffer;
    float *float_buffer;
    MYFLT zerodBFS;
    oboe::AudioFormat oboe_audio_format;
    oboe::AudioStreamBuilder audio_stream_builder;
};

#endif  // __CSOUND_OBOE_HPP__
