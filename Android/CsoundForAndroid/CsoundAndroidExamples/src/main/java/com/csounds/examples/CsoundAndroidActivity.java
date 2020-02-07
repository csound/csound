/* 
 
 CsoundBaseActivity.java:

 
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

package com.csounds.examples;


import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.csounds.examples.BaseCsoundActivity;
import com.csounds.examples.tests.AccelerometerActivity;
import com.csounds.examples.tests.ButtonTestActivity;
import com.csounds.examples.tests.CsoundHaikuIVActivity;
import com.csounds.examples.tests.HarmonizerActivity;
import com.csounds.examples.tests.MultiTouchXYActivity;
import com.csounds.examples.tests.PingPongDelayActivity;
import com.csounds.examples.tests.SimpleTest1Activity;
import com.csounds.examples.tests.SimpleTest2Activity;
import com.csounds.examples.tests.WaveviewTestActivity;

public class CsoundAndroidActivity extends BaseCsoundActivity {

	String[] testNames = new String[] { "Simple Test 1", "Simple Test 2", "Button Test", 
			"Ping Pong Delay", "Harmonizer", "Accelerometer", "Csound Haiku IV", "MultiTouch XY", "Waveview"};
	@SuppressWarnings("rawtypes")
	Class[] activities = new Class[] { SimpleTest1Activity.class, SimpleTest2Activity.class, 
			ButtonTestActivity.class, PingPongDelayActivity.class, HarmonizerActivity.class, 
			AccelerometerActivity.class, CsoundHaikuIVActivity.class, MultiTouchXYActivity.class,
			WaveviewTestActivity.class};
	
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		ListView listView = (ListView) findViewById(R.id.list_view);

		listView.setAdapter(new ArrayAdapter<String>(this,
				android.R.layout.simple_list_item_1,
				testNames));
		listView.setOnItemClickListener(new OnItemClickListener() {

			public void onItemClick(AdapterView<?> arg0, View arg1,
					int position, long arg3) {

			
					startActivity(new Intent(CsoundAndroidActivity.this,
							activities[position]));
				

			}
		});

	}

}