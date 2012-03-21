/* 
 
   CsoundObj.java:
 
   Copyright (C) 2011 Victor Lazzarini, Steven Yi
 
   This file is part of Csound.
 
   Csound is free software; you can redistribute it
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

package com.csounds;

import java.io.File;
import java.util.ArrayList;
import javax.swing.JSlider;
import javax.swing.JButton;
import javax.sound.sampled.*;

import com.csounds.valueCacheable.CachedButton;
import com.csounds.valueCacheable.CachedSlider;
import com.csounds.valueCacheable.CsoundValueCacheable;

import csnd.Csound;
import csnd.CsoundCallbackWrapper;
import csnd.CsoundMYFLTArray;
import csnd.csndConstants;

public class CsoundObj {

    private Csound csound;
    private ArrayList<CsoundValueCacheable> valuesCache;
    private ArrayList<CsoundObjCompletionListener> completionListeners;
    private boolean muted = false;
    private boolean stopped = false;
    private Thread thread;
    private boolean audioInEnabled = false;
    private boolean messageLoggingEnabled = false;
    private boolean useJavaSound = true;
    int retVal = 0;


    private CsoundCallbackWrapper callbacks;

    public CsoundObj() {
	valuesCache = new ArrayList<CsoundValueCacheable>();
	completionListeners = new ArrayList<CsoundObjCompletionListener>();
    }
	
   public CsoundObj(boolean useJavaSound) {
	valuesCache = new ArrayList<CsoundValueCacheable>();
	completionListeners = new ArrayList<CsoundObjCompletionListener>();
        this.useJavaSound = useJavaSound;
    }

    /* VALUE CACHEABLE */

    public boolean isAudioInEnabled() {
	return audioInEnabled;
    }

    public void setAudioInEnabled(boolean audioInEnabled) {
	this.audioInEnabled = audioInEnabled;
    }

    public boolean isMessageLoggingEnabled() {
	return messageLoggingEnabled;
    }

    public void setMessageLoggingEnabled(boolean messageLoggingEnabled) {
	this.messageLoggingEnabled = messageLoggingEnabled;
    }

    public CsoundValueCacheable addSlider(JSlider slider, String channelName,
					  double min, double max) {
	CachedSlider cachedSlider = new CachedSlider(slider, channelName, min,
						     max);
	valuesCache.add(cachedSlider);

	return cachedSlider;
    }

	
    public CsoundValueCacheable addButton(JButton button, String channelName) {
	CachedButton cachedButton = new CachedButton(button, channelName);
	valuesCache.add(cachedButton);

	return cachedButton;
    }

    public Csound getCsound() {
	return csound;
    }

    public boolean isMuted() {
	return muted;
    }

    public void setMuted(boolean muted) {
	this.muted = muted;
    }

    public void addValueCacheable(CsoundValueCacheable valueCacheable) {
	valuesCache.add(valueCacheable);
    }

    public void removeValueCacheable(CsoundValueCacheable valueCacheable) {
	valuesCache.remove(valueCacheable);
    }

    
    public CsoundMYFLTArray getInputChannelPtr(String channelName) {
	CsoundMYFLTArray ptr = new CsoundMYFLTArray(1);
	getCsound().GetChannelPtr(
				  ptr.GetPtr(),
				  channelName,
				  csndConstants.CSOUND_CONTROL_CHANNEL
				  | csndConstants.CSOUND_INPUT_CHANNEL);
	return ptr;
    }

    public void sendScore(String score) {
	csound.InputMessage(score);
    }

    public void addCompletionListener(CsoundObjCompletionListener listener) {
	completionListeners.add(listener);
    }

    public void startCsound(final File csdFile) {
	stopped = false;
	thread = new Thread() {
		public void run() {
		    setPriority(Thread.MAX_PRIORITY);;
		    if(useJavaSound== false)
			runCsound(csdFile);
		    else
			runCsoundJavaSound(csdFile);
		}
	    };
	thread.start();
    }

    public void stopCsound() {
	stopped = true;
	thread = null;
    }

    public int getNumChannels() {
	return csound.GetNchnls();
    }

    public int getKsmps() {
	return csound.GetKsmps();
    }

    public int getError(){
	return retVal;
    }
	
    /* Render Methods */

    private void runCsound(File f) {

	csound = new Csound();
	retVal = csound.PreCompile();

	retVal = csound.Compile(f.getAbsolutePath());

	if (retVal == 0) {
	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.setup(this);
	    }

	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.updateValuesToCsound();
	    }

	    while (csound.PerformKsmps() == 0 && !stopped) {
		for (CsoundValueCacheable cacheable : valuesCache) {
		    cacheable.updateValuesFromCsound();
		}
		for (CsoundValueCacheable cacheable : valuesCache) {
		    cacheable.updateValuesToCsound();
		}
	    }

	    csound.Stop();
	    csound.Cleanup();
	    csound.Reset();

	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.cleanup();
	    }

	    for (CsoundObjCompletionListener listener : completionListeners) {
		listener.csoundObjComplete(this);
	    }

	} else {			
	    for (CsoundObjCompletionListener listener : completionListeners) {
		listener.csoundObjComplete(this);
	    }
			
	}

    }

    private void runCsoundJavaSound(File f) {

	csound = new Csound();

	if (messageLoggingEnabled) {
	    callbacks = new CsoundCallbackWrapper(csound) {
		    @Override
			public void MessageCallback(int attr, String msg) {
			// super.MessageCallback(attr, msg);
		    }

		};
	    callbacks.SetMessageCallback();
	}
	retVal = csound.PreCompile();
	csound.SetHostImplementedAudioIO(1, 0);
	retVal = csound.Compile(f.getAbsolutePath(), "-+rtaudio=null");

	if (retVal == 0) {
	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.setup(this);
	    }

	    int pos = 0; int i,j;
	    int ksmps = csound.GetKsmps();
	    int nchnls = csound.GetNchnls();
	    int buffsizeBytes = csound.GetOutputBufferSize()*2;
	    byte[] byteBuffer = new byte[buffsizeBytes];
	    AudioFormat format = new AudioFormat((float) csound.GetSr(), 16, csound.GetNchnls(),true,false);
	    DataLine.Info targetInfo = new DataLine.Info(SourceDataLine.class, format, buffsizeBytes);
	    SourceDataLine line; 	
	  
	    double conv = 32768.0/csound.Get0dBFS();
	    short sample;
            try {	
	        line = (SourceDataLine) AudioSystem.getLine(targetInfo);
	    	line.open(format, buffsizeBytes);
            } catch (Exception e) {
		return;
            } 

	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.updateValuesToCsound();
	    }

	    line.start();
	    while (csound.PerformKsmps() == 0 && !stopped) {
		for(i=0; i < ksmps; i++){
		    for(j=0; j < nchnls; j++){
			sample = (short) (csound.GetSpoutSample(i,j)*conv);
			byteBuffer[pos++] =  (byte) (sample & 0xFF);
			byteBuffer[pos++] =  (byte) ((sample >> 8)  & 0xFF);
		    }
		    if(pos == buffsizeBytes) {
			line.write(byteBuffer, 0, buffsizeBytes);
			pos = 0;
		    }
		}
                for (CsoundValueCacheable cacheable : valuesCache) {
		    cacheable.updateValuesFromCsound();
		}
		for (CsoundValueCacheable cacheable : valuesCache) {
		    cacheable.updateValuesToCsound();
		}

			       
	    }

	    line.drain();
	    line.stop();
	    line.close();
	    line = null;

	    csound.Stop();
	    csound.Cleanup();
	    csound.Reset();

	    for (CsoundValueCacheable cacheable : valuesCache) {
		cacheable.cleanup();
	    }

	    for (CsoundObjCompletionListener listener : completionListeners) {
		listener.csoundObjComplete(this);
	    }

	}
	else {
	    for (CsoundObjCompletionListener listener : completionListeners) {
		listener.csoundObjComplete(this);
	    }
	}
    } 		
}
