/* 
 
 SimpleTest1Activity.java:
 
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

import android.os.Bundle;
import android.util.Log;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ToggleButton;

import com.csounds.BaseCsoundActivity;
import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;
import com.csounds.R;
import com.csounds.valueCacheable.CsoundValueCacheable;

public class SimpleTest1Activity extends BaseCsoundActivity implements CsoundObjCompletionListener {

	ToggleButton startStopButton = null;
	SeekBar fSlider;
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.simple_test_1);
		
		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);
		fSlider = (SeekBar) findViewById(R.id.slider);
		startStopButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				if(isChecked) {
					String csd = getResourceFileAsString(R.raw.test);
					File f = createTempFile(csd);
					csoundObj.addSlider(fSlider, "slider", 0.,
							1.);
					csoundObj.addCompletionListener(SimpleTest1Activity.this);
					csoundObj.startCsound(f);
				} else {
					csoundObj.stopCsound();
				}
				
			}
		});
		
	
		
		csoundObj.addValueCacheable(new CsoundValueCacheable() {
			
			@Override
			public void updateValuesToCsound() {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void updateValuesFromCsound() {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void setup(CsoundObj csoundObj) {
				Log.d("CsoundAndroidActivity", "ValueCacheable setup called");
			}
			
			@Override
			public void cleanup() {
				Log.d("CsoundAndroidActivity", "ValueCacheable cleanup called");
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
