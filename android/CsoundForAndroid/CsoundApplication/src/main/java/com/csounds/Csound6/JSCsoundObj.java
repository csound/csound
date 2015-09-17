/*

 CsoundObj.java:

 Copyright (C) 2011 Victor Lazzarini, Steven Yi

 This file is part of Csound Android Examples.

 The Csound Android Examples is free software; you can redistribute it
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

package com.csounds.Csound6;

import java.io.File;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;
import android.webkit.JavascriptInterface;
import android.widget.Button;
import android.widget.SeekBar;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjListener;
import com.csounds.bindings.CsoundBinding;
import com.csounds.bindings.motion.CsoundMotion;
import com.csounds.bindings.ui.CsoundUI;

import csnd6.AndroidCsound;
import csnd6.Csound;
import csnd6.CsoundCallbackWrapper;
import csnd6.CsoundMYFLTArray;
import csnd6.controlChannelType;

public class JSCsoundObj extends CsoundObj {

	CsoundUI csoundUI = new CsoundUI(this);
	CsoundMotion csoundMotion = new CsoundMotion(this);

	public JSCsoundObj() {
		super(false);
	}

	public JSCsoundObj(boolean useAudioTrack) {
		super(useAudioTrack, useAudioTrack);
	}

	@JavascriptInterface
	public boolean isAudioInEnabled() {
		return super.isAudioInEnabled();
	}

	@JavascriptInterface
	public void setAudioInEnabled(boolean audioInEnabled) {
		super.setAudioInEnabled(audioInEnabled);
	}

	@JavascriptInterface
	public boolean isMessageLoggingEnabled() {
		return super.isMessageLoggingEnabled();
	}

	@JavascriptInterface
	public void setMessageLoggingEnabled(boolean messageLoggingEnabled) {
		super.setMessageLoggingEnabled(messageLoggingEnabled);
	}

	@JavascriptInterface
	public CsoundBinding addSlider(SeekBar seekBar, String channelName,
			double min, double max) {
		return csoundUI.addSlider(seekBar, channelName, min, max);
	}

	@JavascriptInterface
	public CsoundBinding addButton(Button button, String channelName, int type) {
		return csoundUI.addButton(button, channelName, type);
	}

	@JavascriptInterface
	public CsoundBinding addButton(Button button, String channelName) {
		return csoundUI.addButton(button, channelName);
	}

	@JavascriptInterface
	public Csound getCsound() {
		return super.getCsound();
	}

	@JavascriptInterface
	public boolean isMuted() {
		return super.isMuted();
	}

	@JavascriptInterface
	public void setMuted(boolean muted) {
		super.setMuted(muted);
	}

	@JavascriptInterface
	public void addValueCacheable(CsoundBinding valueCacheable) {
		super.addBinding(valueCacheable);
	}

	@JavascriptInterface
	public synchronized void inputMessage(String mess) {
		super.inputMessage(mess);
	}

	@JavascriptInterface
	public synchronized void removeValueCachable(CsoundBinding valueCacheable) {
		super.removeBinding(valueCacheable);
	}

	@JavascriptInterface
	public CsoundBinding enableAccelerometer(Context context) {
		return csoundMotion.enableAccelerometer(context);
	}

	@JavascriptInterface
	public CsoundMYFLTArray getInputChannelPtr(String channelName,
			controlChannelType channelType) {
		return super.getInputChannelPtr(channelName, channelType);
	}

	@JavascriptInterface
	public CsoundMYFLTArray getOutputChannelPtr(String channelName,
			controlChannelType channelType) {
		return super.getOutputChannelPtr(channelName, channelType);
	}

	@JavascriptInterface
	public void sendScore(String score) {
		super.sendScore(score);
	}

	@JavascriptInterface
	public void readScore(String score) {
		super.readScore(score);
	}

	@JavascriptInterface
	public void addCompletionListener(CsoundObjListener listener) {
		super.addListener(listener);
	}

	@JavascriptInterface
	public void startCsound(final File csdFile) {
		super.startCsound(csdFile);
	}

	@JavascriptInterface
	public void togglePause() {
		super.togglePause();
	}

	@JavascriptInterface
	public void pause() {
		super.pause();
	}

	@JavascriptInterface
	public void play() {
		super.play();
	}

	@JavascriptInterface
	public synchronized void stopCsound() {
		super.stop();
	}

	@JavascriptInterface
	public int getNumChannels() {
		return super.getNumChannels();
	}

	@JavascriptInterface
	public int getKsmps() {
		return super.getKsmps();
	}

	@JavascriptInterface
	public int getError() {
		return super.getError();
	}

	@JavascriptInterface
	public void setControlChannel(String channelName, double value) {
        Csound csound_ = super.getCsound();
        if (csound_ != null) {
            csound_.SetChannel(channelName, value);
        }
	}

	@JavascriptInterface
	public double getControlChannel(String channelName) {
        Csound csound_ = super.getCsound();
        if (csound_ != null) {
            return csound_.GetChannel(channelName);
        } else {
            return 0.0;
        }
	}
	
	@JavascriptInterface
	public void Message(String text) {
        Csound csound_ = super.getCsound();
        if (csound_ != null) {
            csound_.Message(text);
        }
	}
	@JavascriptInterface
	public void message(String text) {
        Csound csound_ = super.getCsound();
        if (csound_ != null) {
            csound_.Message(text);
        }
	}

}
