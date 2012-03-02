package com.csounds.tests;

import java.io.File;

import android.os.Bundle;
import android.util.Log;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.SeekBar;
import android.widget.ToggleButton;

import com.csounds.BaseCsoundActivity;
import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;
import com.csounds.R;
import com.csounds.valueCacheable.CsoundValueCacheable;

public class SimpleTest2Activity extends BaseCsoundActivity implements
		CsoundObjCompletionListener {

	ToggleButton startStopButton = null;

	SeekBar noteRateSlider;
	SeekBar durationSlider;
	SeekBar attackSlider;
	SeekBar decaySlider;
	SeekBar sustainSlider;
	SeekBar releaseSlider;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.simple_test_2);

		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);

		noteRateSlider = (SeekBar) findViewById(R.id.noteRateSlider);
		durationSlider = (SeekBar) findViewById(R.id.durationSlider);
		attackSlider = (SeekBar) findViewById(R.id.attackSlider);
		decaySlider = (SeekBar) findViewById(R.id.decaySlider);
		sustainSlider = (SeekBar) findViewById(R.id.sustainSlider);
		releaseSlider = (SeekBar) findViewById(R.id.releaseSlider);

		setSeekBarValue(noteRateSlider, 1, 4, 1.5);
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
							String csd = getResourceFileAsString(R.raw.test2);
							File f = createTempFile(csd);

							csoundObj.addSlider(noteRateSlider, "noteRate", 1,
									4);
							csoundObj.addSlider(durationSlider, "duration", .5,
									4);
							csoundObj.addSlider(attackSlider, "attack", 0, 2);
							csoundObj.addSlider(decaySlider, "decay", .05, 2);
							csoundObj.addSlider(sustainSlider, "sustain", 0, 1);
							csoundObj.addSlider(releaseSlider, "release", 0, 4);

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
