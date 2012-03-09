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

package com.csounds.tests;

import java.io.File;
import java.text.MessageFormat;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.ToggleButton;

import com.csounds.BaseCsoundActivity;
import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;
import com.csounds.R;
import com.csounds.valueCacheable.CsoundValueCacheable;

public class ButtonTestActivity extends BaseCsoundActivity implements
		CsoundObjCompletionListener {
	
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

					@Override
					public void onCheckedChanged(CompoundButton buttonView,
							boolean isChecked) {
						if (isChecked) {
							String csd = getResourceFileAsString(R.raw.button_test);
							File f = createTempFile(csd);

							csoundObj.addSlider(durationSlider, "duration", .5,
									4);
							csoundObj.addSlider(attackSlider, "attack", 0, 2);
							csoundObj.addSlider(decaySlider, "decay", .05, 2);
							csoundObj.addSlider(sustainSlider, "sustain", 0, 1);
							csoundObj.addSlider(releaseSlider, "release", 0, 4);

							csoundObj.addButton(valueButton, "button1");
							
							eventButton.setOnClickListener(new OnClickListener() {
								
								@Override
								public void onClick(View v) {
									
									Log.d("TEST", "Event Button");
									
									float value = durationSlider.getProgress() / (float)durationSlider.getMax();
									float min = .5f;
									float max = 4f;
									float range = max - min;
									value = (value * range) + min;
									String event = String.format("i2 0 %f", value);
									
									csoundObj.sendScore(event);
								}
							});
							
							csoundObj.startCsound(f);
						} else {
							csoundObj.stopCsound();
						}

					}
				});

	}

	@Override
	public void csoundObjComplete(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
			}
		});
	}

}
