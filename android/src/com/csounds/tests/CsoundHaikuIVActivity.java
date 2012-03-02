package com.csounds.tests;

import java.io.File;

import android.os.Bundle;

import com.csounds.BaseCsoundActivity;
import com.csounds.R;

public class CsoundHaikuIVActivity extends BaseCsoundActivity {


	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.csound_haiku_iv);

		String csd = getResourceFileAsString(R.raw.iv);
		File f = createTempFile(csd);
		csoundObj.setMessageLoggingEnabled(true);
		csoundObj.startCsound(f);
		
	}

}
