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

					@Override
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

	@Override
	public void csoundObjComplete(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
			}
		});
	}

}
