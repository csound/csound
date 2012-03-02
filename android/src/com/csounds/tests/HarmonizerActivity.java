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

public class HarmonizerActivity extends BaseCsoundActivity implements
		CsoundObjCompletionListener {

	ToggleButton startStopButton = null;

	SeekBar harmonyPitchSlider;
	SeekBar gainSlider;


	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.harmonizer);

		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);

		harmonyPitchSlider = (SeekBar) findViewById(R.id.harmony_pitch_slider);
		gainSlider = (SeekBar) findViewById(R.id.gain_slider);
		

		setSeekBarValue(harmonyPitchSlider, 0, 1, .5);
		setSeekBarValue(gainSlider, .5, 3, 1.5);

		startStopButton
				.setOnCheckedChangeListener(new OnCheckedChangeListener() {

					@Override
					public void onCheckedChanged(CompoundButton buttonView,
							boolean isChecked) {
						if (isChecked) {
							String csd = getResourceFileAsString(R.raw.harmonizer);
							File f = createTempFile(csd);

							csoundObj.addSlider(harmonyPitchSlider,
									"slider", 0, 1);
							csoundObj.addSlider(gainSlider,
									"gain", .5, 3);
							
							csoundObj.setAudioInEnabled(true);
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
