/* 
 
 CsoundBinding.java:
 
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

package com.csounds;

import java.io.File;
import java.util.ArrayList;

import android.content.Context;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.util.Log;
import android.webkit.JavascriptInterface;

import com.csounds.bindings.CsoundBinding;

import csnd6.AndroidCsound;
import csnd6.Csound;
import csnd6.CsoundCallbackWrapper;
import csnd6.CsoundMYFLTArray;
import csnd6.controlChannelType;

public class CsoundObj {
	/** Used to post Csound runtime messages to the host. */
	public interface MessagePoster {
		/** Clear the message display and post the message. */
		public void postMessageClear(String message);

		/** Append the message to the message display. */
		public void postMessage(String message);
	};

	private Csound csound;
	private ArrayList<CsoundBinding> bindings;
	private ArrayList<CsoundObjListener> listeners;
	private ArrayList<String> scoreMessages;
	private boolean muted = false;
	private boolean stopped = true;
	private Thread thread;
	private boolean audioInEnabled = false;
	private boolean messageLoggingEnabled = false;
	private boolean useAudioTrack = false;
	int retVal = 0;
	private boolean pause = false;
	private CsoundCallbackWrapper callbacks;
	private Object mLock = new Object();
	public MessagePoster messagePoster = null;
	private long stime = 0;
	private double systime = System.nanoTime()*1.0e-6;
	private double startTime = System.nanoTime()*1.0e-6;
	private boolean isAsync = true;

	public CsoundObj() {
		this(false);
	}
	
	public CsoundObj(boolean useAudioTrack){
		  this(useAudioTrack,true);
	}

	public CsoundObj(boolean useAudioTrack, boolean isAsync) {
		bindings = new ArrayList<CsoundBinding>();
		listeners = new ArrayList<CsoundObjListener>();
		scoreMessages = new ArrayList<String>();
		this.useAudioTrack = useAudioTrack;
        this.isAsync = isAsync;
        
		if (useAudioTrack) {
			// Log.d("CsoundObj", "audio track");
			csound = new Csound();
		} else {
			Log.d("CsoundObj", "creating new AndroidCsound: " + (isAsync ? 0 : 1));
			csound = new AndroidCsound(isAsync);
		}
	}

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

	public Csound getCsound() {
		return csound;
	}

	public boolean isMuted() {
		return muted;
	}

	public void setMuted(boolean muted) {
		this.muted = muted;
	}

	public boolean isPaused() {
		return pause;
	}

	public boolean isStopped() {
		return stopped;
	}

	public void addBinding(CsoundBinding binding) {
		if (!stopped)
			binding.setup(this);
		synchronized (mLock) {
			bindings.add(binding);
		}
	}

	@JavascriptInterface
	public/* synchronized */void inputMessage(String mess) {
		if(isAsync){
		synchronized (mLock) {
			String message = new String(mess);
			scoreMessages.add(message);
		}
		} else csound.InputMessage(mess);
	}

	public/* synchronized */void removeBinding(CsoundBinding binding) {
		synchronized (mLock) {
			bindings.remove(binding);
		}
	}

	public CsoundMYFLTArray getInputChannelPtr(String channelName,
			controlChannelType channelType) {

		int channelSize = (channelType == controlChannelType.CSOUND_AUDIO_CHANNEL) ? getCsound()
				.GetKsmps() : 1;
		CsoundMYFLTArray ptr = new CsoundMYFLTArray(channelSize);

		getCsound().GetChannelPtr(
				ptr.GetPtr(),
				channelName,
				channelType.swigValue()
						| controlChannelType.CSOUND_INPUT_CHANNEL.swigValue());
		return ptr;
	}

	public CsoundMYFLTArray getOutputChannelPtr(String channelName,
			controlChannelType channelType) {
		int channelSize = (channelType == controlChannelType.CSOUND_AUDIO_CHANNEL) ? getCsound()
				.GetKsmps() : 1;
		CsoundMYFLTArray ptr = new CsoundMYFLTArray(channelSize);

		getCsound().GetChannelPtr(
				ptr.GetPtr(),
				channelName,
				channelType.swigValue()
						| controlChannelType.CSOUND_OUTPUT_CHANNEL.swigValue());
		return ptr;
	}

	public void sendScore(String score) {
		inputMessage(score);
	}

	public void readScore(String score) {
		sendScore(score);
	}

	public void updateOrchestra(String orchestraString) {
		csound.CompileOrc(orchestraString);
	}

	public void addListener(CsoundObjListener listener) {
		synchronized(mLock) {
			listeners.add(listener);
		}
	}

	public void removeListener(CsoundObjListener listener) {
		synchronized(mLock) {
			listeners.remove(listener);
		}
	}
	
	public void startCsound(final File csdFile) {
		stopped = false;
		thread = new Thread() {
			public void run() {
				setPriority(Thread.MAX_PRIORITY);
				if (useAudioTrack == false) {
					// Log.d("CsoundObj", "USING OPENSL");
					runCsoundOpenSL(csdFile);

				} else {
					// Log.d("CsoundObj", "USING AUDIO TRACK");
					runCsoundAudioTrack(csdFile);
				}
			}
		};
		thread.start();
	}

	public void togglePause() {
		pause = !pause;
		if (!isAsync) ((AndroidCsound)csound).Pause(pause);
	}

	public void pause() {
		pause = true;
		if (!isAsync) ((AndroidCsound)csound).Pause(pause);
	}

	public void play() {
		pause = false;
		if (!isAsync) ((AndroidCsound)csound).Pause(pause);
	}

	public synchronized void stop() {
		stopped = true;
		if (thread != null) {
			try {
				thread.join();
				thread = null;
			} catch (InterruptedException e) {
				Log.d("CsoundObj", e.toString());
				e.printStackTrace();
			}
		}
	 } 
 
	public boolean getAsyncStatus() { return isAsync; }

	public int getNumChannels() {
		return csound.GetNchnls();
	}

	public int getKsmps() {
		return csound.GetKsmps();
	}

	public int getError() {
		return retVal;
	}

	/* Render Methods */

	private void runCsoundOpenSL(File f) {
		Log.d("CsoundObj", "THREAD START");
		((AndroidCsound) csound).setOpenSlCallbacks();
		if (messageLoggingEnabled) {
			callbacks = new CsoundCallbackWrapper(csound) {
				@Override
				public void MessageCallback(int attr, String msg) {
					Log.d("CsoundObj", msg);
					if (messagePoster != null) {
						messagePoster.postMessage(msg);
					}
					super.MessageCallback(attr, msg);
				}
			};
			callbacks.SetMessageCallback();
		}
		if(!isAsync) this.pause();
		retVal = csound.Compile(f.getAbsolutePath());
		Log.d("CsoundObj", "Return Value2: " + retVal);
		if (retVal == 0) {
			for (int i = 0; i < bindings.size(); i++) {
				CsoundBinding cacheable = bindings.get(i);
				cacheable.setup(this);
			}
			stopped = false;
			for (int i = 0; i < bindings.size(); i++) {
				CsoundBinding cacheable = bindings.get(i);
				cacheable.updateValuesToCsound();
			}

			for (int i = 0; i < listeners.size(); i++) {
				CsoundObjListener listener = listeners.get(i);
				listener.csoundObjStarted(this);
			}
			
			startTime = System.nanoTime()*1.0e-6;
			//double tmptime = startTime;
			if(!isAsync) this.play();
			while(!stopped) {
			 int ret = 0;
             if(isAsync){
            	 ret = csound.PerformKsmps();
            	 if(ret != 0) break;
    			 stime += csound.GetKsmps();
    		     
    			 systime = System.nanoTime()*1.0e-6;
    	         //Log.d("CsoundObj", "java time:" + (systime - startTime));
    	         //Log.d("CsoundObj", "java diff:" + (systime - tmptime));
    	         //tmptime = systime;
    	       
    				synchronized (mLock) {
    					CsoundBinding cacheable;
    					String mess;
    					for (int i = 0; i < bindings.size(); i++) {
    						cacheable = bindings.get(i);
    						cacheable.updateValuesFromCsound();
    					}
    					for (int i = 0; i < scoreMessages.size(); i++) {
    						mess = scoreMessages.get(i);
    						csound.InputMessage(mess);
    					}
    					scoreMessages.clear();
    					for (int i = 0; i < bindings.size(); i++) {
    						cacheable = bindings.get(i);
    						cacheable.updateValuesToCsound();
    					}
    				}
    				while (pause)
    					try {
    						Thread.sleep(1);
    					} catch (InterruptedException e) {
    						e.printStackTrace();
    					}
             } else {

            	 try {
						Thread.sleep(100);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
            	 
             }
			}
			if (!isAsync) {
				csound.InputMessage("e 0");
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
			//csound.Stop();
			//csound.Cleanup();
			csound.Reset();
			
			synchronized (mLock) {
				for (int i = 0; i < bindings.size(); i++) {
					CsoundBinding cacheable = bindings.get(i);
					cacheable.cleanup();
				}
				for (int i = 0; i < listeners.size(); i++) {
					CsoundObjListener listener = listeners.get(i);
					listener.csoundObjCompleted(this);
				}
			}

		} else {
			synchronized (mLock) {
				for (int i = 0; i < listeners.size(); i++) {
					CsoundObjListener listener = listeners.get(i);
					listener.csoundObjCompleted(this);
				}
			}
		}
		Log.d("CsoundObj", "THREAD END");
	}
	

	private void runCsoundAudioTrack(File f) {
		csound.SetHostImplementedAudioIO(1, 0);

		if (messageLoggingEnabled) {
			callbacks = new CsoundCallbackWrapper(csound) {
				@Override
				public void MessageCallback(int attr, String msg) {
					Log.d("CsoundObj", msg);
					if (messagePoster != null) {
						messagePoster.postMessage(msg);
					}
					super.MessageCallback(attr, msg);
				}
			};
			callbacks.SetMessageCallback();
		}
		retVal = csound.Compile(f.getAbsolutePath());
		Log.d("CsoundObj", "Return Value2: " + retVal);
		if (retVal == 0) {
			synchronized (mLock) {
				for (int i = 0; i < bindings.size(); i++) {
					CsoundBinding cacheable = bindings.get(i);
					cacheable.setup(this);
				}
			}
			int channelConfig = (csound.GetNchnls() == 2) ? AudioFormat.CHANNEL_OUT_STEREO
					: AudioFormat.CHANNEL_OUT_MONO;

			int channelInConfig = AudioFormat.CHANNEL_IN_MONO;

			int minSize = AudioTrack.getMinBufferSize((int) csound.GetSr(),
					channelConfig, AudioFormat.ENCODING_PCM_16BIT);

			if (audioInEnabled) {
				int recordMinSize = AudioRecord.getMinBufferSize(
						(int) csound.GetSr(), channelInConfig,
						AudioFormat.ENCODING_PCM_16BIT);
				minSize = (minSize > recordMinSize) ? minSize : recordMinSize;
			}

			AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC,
					(int) csound.GetSr(), channelConfig,
					AudioFormat.ENCODING_PCM_16BIT, minSize,
					AudioTrack.MODE_STREAM);
			Log.d("CsoundObj", "Buffer Size: " + minSize);

			AudioRecord audioRecord = null;
			CsoundMYFLTArray audioIn = null;

			if (audioInEnabled) {

				audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
						(int) csound.GetSr(), channelInConfig,
						AudioFormat.ENCODING_PCM_16BIT, minSize);

				if (audioRecord.getState() != AudioRecord.STATE_INITIALIZED) {
					Log.d("CsoundObj",
							"AudioRecord unable to be initialized. Error "
									+ audioRecord.getState());
					audioRecord.release();
					audioRecord = null;
				} else {
					try {
						audioRecord.startRecording();
						if (audioRecord.getRecordingState() != AudioRecord.RECORDSTATE_RECORDING) {
							Log.d("CsoundObj",
									"AudioRecord unable to be initialized. Error "
											+ audioRecord.getRecordingState());
						}
						audioIn = new CsoundMYFLTArray();
						audioIn.SetPtr(csound.GetSpin());
					} catch (IllegalStateException e) {
						audioRecord.release();
						audioRecord = null;
						audioIn = null;
					}
				}
			}
			audioTrack.play();
			int counter = 0;
			int nchnls = csound.GetNchnls();
			int recBufferSize = csound.GetKsmps();
			int bufferSize = recBufferSize * nchnls;
			short[] samples = new short[bufferSize];
			float multiplier = (float) (Short.MAX_VALUE / csound.Get0dBFS());
			float recMultiplier = 1 / multiplier;
			Log.d("CsoundObj", "Multiplier: " + multiplier + " : "
					+ recMultiplier);
			stopped = false;
			for (CsoundBinding cacheable : bindings) {
				cacheable.updateValuesToCsound();
			}
			short recordSample[] = new short[recBufferSize];
			if (audioRecord != null) {
				audioRecord.read(recordSample, 0, recBufferSize);
				for (int i = 0; i < csound.GetKsmps(); i++) {
					short sample = recordSample[i];
					if (nchnls == 2) {
						int index = i * 2;
						audioIn.SetValues(index,
								(double) (sample * recMultiplier),
								(double) (sample * recMultiplier));
					} else {
						audioIn.SetValue(i, sample);
					}
				}
			}
			while (csound.PerformKsmps() == 0 && !stopped) {
				for (int i = 0; i < csound.GetKsmps(); i++) {
					samples[counter++] = (short) (csound.GetSpoutSample(i, 0) * multiplier);
					if (nchnls > 1) {
						samples[counter++] = (short) (csound.GetSpoutSample(i,
								1) * multiplier);
					}
				}
				if (counter >= bufferSize) {
					audioTrack.write(samples, 0, bufferSize);
					counter = 0;
				}
				synchronized (mLock) {
					for (int i = 0; i < bindings.size(); i++) {
						CsoundBinding cacheable = bindings.get(i);
						cacheable.updateValuesFromCsound();
					}
					for (int i = 0; i < scoreMessages.size(); i++) {
						String mess = scoreMessages.get(i);
						csound.InputMessage(mess);
					}
					scoreMessages.clear();
					for (int i = 0; i < bindings.size(); i++) {
						CsoundBinding cacheable = bindings.get(i);
						cacheable.updateValuesToCsound();
					}
				}
				if (audioRecord != null) {
					audioRecord.read(recordSample, 0, recBufferSize);
					for (int i = 0; i < csound.GetKsmps(); i++) {
						short sample = recordSample[i];
						if (nchnls == 2) {
							int index = i * 2;
							audioIn.SetValues(index,
									(double) (sample * recMultiplier),
									(double) (sample * recMultiplier));
						} else {
							audioIn.SetValue(i, sample);
						}
					}
				}
				while (pause)
					try {
						Thread.sleep(1);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
			}
			audioTrack.stop();
			audioTrack.release();
			if (audioRecord != null) {
				audioRecord.stop();
				audioRecord.release();
				audioIn.Clear();
			}
			csound.Stop();
			csound.Cleanup();
			csound.Reset();
			synchronized (mLock) {
				for (int i = 0; i < bindings.size(); i++) {
					CsoundBinding cacheable = bindings.get(i);
					cacheable.cleanup();
				}
				for (int i = 0; i < listeners.size(); i++) {
					CsoundObjListener listener = listeners.get(i);
					listener.csoundObjCompleted(this);
				}
			}
		} else {
			synchronized (mLock) {
				for (int i = 0; i < listeners.size(); i++) {
					CsoundObjListener listener = listeners.get(i);
					listener.csoundObjCompleted(this);
				}
			}
		}
	}
}
