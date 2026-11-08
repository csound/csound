package com.csounds.bindings.motion;

import android.content.Context;
import android.webkit.JavascriptInterface;

import com.csounds.CsoundObj;
import com.csounds.bindings.CsoundBinding;

public class CsoundMotion {
	CsoundObj csoundObj;
	
	public CsoundMotion(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
	}
	
	@JavascriptInterface
	public CsoundBinding enableAccelerometer(Context context) {
		CsoundAccelerometerBinding accelerometer = new CsoundAccelerometerBinding(context);
		csoundObj.addBinding(accelerometer);
		return accelerometer;
	}

}
