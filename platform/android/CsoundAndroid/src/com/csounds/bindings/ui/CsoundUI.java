package com.csounds.bindings.ui;

import android.webkit.JavascriptInterface;
import android.widget.Button;
import android.widget.SeekBar;

import com.csounds.CsoundObj;
import com.csounds.bindings.CsoundBinding;

public class CsoundUI {
	CsoundObj csoundObj;
	
	public CsoundUI(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
	}
	

	@JavascriptInterface
	public CsoundBinding addSlider(SeekBar seekBar, String channelName,
			double min, double max) {
		CsoundSliderBinding cachedSlider = new CsoundSliderBinding(seekBar, channelName, min,
				max);
		csoundObj.addBinding(cachedSlider);
		return cachedSlider;
	}

	@JavascriptInterface
	public CsoundBinding addButton(Button button, String channelName,
			int type) {
		CsoundButtonBinding cachedButton = new CsoundButtonBinding(button, channelName, type);
		csoundObj.addBinding(cachedButton);
		return cachedButton;
	}

	@JavascriptInterface
	public CsoundBinding addButton(Button button, String channelName) {
		CsoundButtonBinding cachedButton = new CsoundButtonBinding(button, channelName);
		csoundObj.addBinding(cachedButton);
		return cachedButton;
	}	
}
