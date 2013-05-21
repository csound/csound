/* 
 
 PingPongDelayActivity.java:
 
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

package com.csounds.examples.tests;

import java.io.File;

import android.os.Bundle;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;
import com.csounds.examples.BaseCsoundActivity;
import com.csounds.examples.R;

public class PingPongDelayActivity extends BaseCsoundActivity implements
		CsoundObjCompletionListener {

	ToggleButton startStopButton = null;

	SeekBar leftDelaySlider;
	SeekBar leftFeedbackSlider;
	SeekBar rightDelaySlider;
	SeekBar rightFeedbackSlider;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.ping_pong_delay);

		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);

		leftDelaySlider = (SeekBar) findViewById(R.id.left_delay_slider);
		leftFeedbackSlider = (SeekBar) findViewById(R.id.left_feedback_slider);
		rightDelaySlider = (SeekBar) findViewById(R.id.right_delay_slider);
		rightFeedbackSlider = (SeekBar) findViewById(R.id.right_feedback_slider);

		setSeekBarValue(leftDelaySlider, 50, 3000, 300);
		setSeekBarValue(leftFeedbackSlider, 0, .8, .5);
		setSeekBarValue(rightDelaySlider, 50, 3000, 300);
		setSeekBarValue(rightFeedbackSlider, 0, .8, .5);

		startStopButton
				.setOnCheckedChangeListener(new OnCheckedChangeListener() {

					public void onCheckedChanged(CompoundButton buttonView,
							boolean isChecked) {
						if (isChecked) {
							String csd = getResourceFileAsString(R.raw.ping_pong_delay);
							File f = createTempFile(csd);

							csoundObj.addSlider(leftDelaySlider,
									"leftDelayTime", 50, 3000);
							csoundObj.addSlider(leftFeedbackSlider,
									"leftFeedback", 0, .8);
							csoundObj.addSlider(rightDelaySlider,
									"rightDelayTime", 50, 3000);
							csoundObj.addSlider(rightFeedbackSlider,
									"rightFeedback", 0, .8);

							csoundObj.setAudioInEnabled(true);
							csoundObj.startCsound(f);
						} else {
							csoundObj.stopCsound();
						}

					}
				});

	}

	public void csoundObjComplete(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
			}
		});
	}

}
