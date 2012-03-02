package com.csounds;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

import com.csounds.tests.AccelerometerActivity;
import com.csounds.tests.ButtonTestActivity;
import com.csounds.tests.CsoundHaikuIVActivity;
import com.csounds.tests.HarmonizerActivity;
import com.csounds.tests.MultiTouchXYActivity;
import com.csounds.tests.PingPongDelayActivity;
import com.csounds.tests.SimpleTest1Activity;
import com.csounds.tests.SimpleTest2Activity;
import com.csounds.tests.WaveviewTestActivity;

public class CsoundAndroidActivity extends BaseCsoundActivity {

	String[] testNames = new String[] { "Simple Test 1", "Simple Test 2", "Button Test", 
			"Ping Pong Delay", "Harmonizer", "Accelerometer", "Csound Haiku IV", "MultiTouch XY", "Waveview"};
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

			@Override
			public void onItemClick(AdapterView<?> arg0, View arg1,
					int position, long arg3) {

			
					startActivity(new Intent(CsoundAndroidActivity.this,
							activities[position]));
				

			}
		});

	}

}