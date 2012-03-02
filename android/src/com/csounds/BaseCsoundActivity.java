package com.csounds;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.widget.SeekBar;

public class BaseCsoundActivity extends Activity {
	
	protected CsoundObj csoundObj = new CsoundObj();
	protected Handler handler = new Handler();
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
	}
	
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		csoundObj.stopCsound();
		
	}
	
	public void setSeekBarValue(SeekBar seekBar, double min, double max, double value) {
		double range = max - min;
		double percent = (value - min) / range;
		
		seekBar.setProgress((int)(percent * seekBar.getMax()));
	}

	
	protected String getResourceFileAsString(int resId) {
		StringBuilder str = new StringBuilder();
		
		InputStream is = getResources().openRawResource(resId);
		BufferedReader r = new BufferedReader(new InputStreamReader(is));
		String line;
		
		try {
			while ((line = r.readLine()) != null) {
				str.append(line).append("\n");
			}
		} catch (IOException ios) {

		}
		
		return str.toString();
	}

	protected File createTempFile(String csd) {
		File f = null;
		
		try {
			f = File.createTempFile("temp", ".csd", this.getCacheDir());
			FileOutputStream fos = new FileOutputStream(f);
			fos.write(csd.getBytes());
			fos.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return f;
	}
}
