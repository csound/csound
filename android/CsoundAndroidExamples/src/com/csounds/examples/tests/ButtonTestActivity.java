/* 
 
 ButtonTestActivity.java:
 
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
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjListener;
import com.csounds.bindings.ui.CsoundUI;
import com.csounds.examples.BaseCsoundActivity;
import com.csounds.examples.R;

public class ButtonTestActivity extends BaseCsoundActivity implements
		CsoundObjListener {
	
	ToggleButton startStopButton = null;

	Button eventButton;
	Button valueButton;
	
	SeekBar durationSlider;
	SeekBar attackSlider;
	SeekBar decaySlider;
	SeekBar sustainSlider;
	SeekBar releaseSlider;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.button_test);

		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);

		eventButton = (Button) findViewById(R.id.eventButton);
		valueButton = (Button) findViewById(R.id.valueButton);
		
		durationSlider = (SeekBar) findViewById(R.id.durationSlider);
		attackSlider = (SeekBar) findViewById(R.id.attackSlider);
		decaySlider = (SeekBar) findViewById(R.id.decaySlider);
		sustainSlider = (SeekBar) findViewById(R.id.sustainSlider);
		releaseSlider = (SeekBar) findViewById(R.id.releaseSlider);

		setSeekBarValue(durationSlider, .5, 4, 1.5);
		setSeekBarValue(attackSlider, .001, 2, .05);
		setSeekBarValue(decaySlider, .05, 2, .05);
		setSeekBarValue(sustainSlider, 0, 1, .7);
		setSeekBarValue(releaseSlider, .05, 4, 1.5);

		startStopButton
				.setOnCheckedChangeListener(new OnCheckedChangeListener() {

					public void onCheckedChanged(CompoundButton buttonView,
							boolean isChecked) {
						if (isChecked) {
							String csd = getResourceFileAsString(R.raw.button_test);
							File f = createTempFile(csd);

							CsoundUI csoundUI = new CsoundUI(csoundObj);
							csoundUI.addSlider(durationSlider, "duration", .5,
									4);
							csoundUI.addSlider(attackSlider, "attack", 0, 2);
							csoundUI.addSlider(decaySlider, "decay", .05, 2);
							csoundUI.addSlider(sustainSlider, "sustain", 0, 1);
							csoundUI.addSlider(releaseSlider, "release", 0, 4);

							csoundUI.addButton(valueButton, "button1");
							
//							eventButton.setOnClickListener(new OnClickListener() {
//								
//								public void onClick(View v) {
//									
//									Log.d("TEST", "Event Button");
//									
//									float value = durationSlider.getProgress() / (float)durationSlider.getMax();
//									float min = .5f;
//									float max = 4f;
//									float range = max - min;
//									value = (value * range) + min;
//									String event = String.format("i2 0 %f", value);
//									
//									csoundObj.inputMessage(event);
//								}
//							});
							
							eventButton.setOnTouchListener(new OnTouchListener() {
								
								public boolean onTouch(View v, MotionEvent evt) {
									
									if(evt.getAction() != MotionEvent.ACTION_DOWN) {
										
										return false;
									}
									
									float value = durationSlider.getProgress() / (float)durationSlider.getMax();
									float min = .5f;
									float max = 4f;
									float range = max - min;
									value = (value * range) + min;
									String event = String.format("i2 0 %f", value);
									
									csoundObj.inputMessage(event);
									
									return true;
								}
							});
							
							csoundObj.startCsound(f);
						} else {
							csoundObj.stop();
						}

					}
				});

	}
	
	
	public void csoundObjStarted(CsoundObj csoundObj) {}

	public void csoundObjCompleted(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
			}
		});
	}

}
