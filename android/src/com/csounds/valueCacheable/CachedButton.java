package com.csounds.valueCacheable;

import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.csounds.CsoundObj;

import csnd.CsoundMYFLTArray;
import csnd.csndConstants;

public class CachedButton extends AbstractValueCacheable {

	private CsoundObj csoundObj;
	private Button button;
	private String channelName;
	CsoundMYFLTArray ptr;
	
	boolean selected = false;
	boolean cacheDirty = false;
	

	public CachedButton(Button button, String channelName) {
		this.button = button;
		this.channelName = channelName;
	}
	
	@Override
	public void setup(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
		button.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				selected = true;
				cacheDirty = true;
			}
		});
		ptr = csoundObj.getInputChannelPtr(channelName);
	}
	
	@Override
	public void updateValuesToCsound() {
		if (csoundObj != null && cacheDirty) {
			ptr.SetValue(0, (selected ? 1 : 0));
			
			cacheDirty = selected;
			selected = false;
		}
	}

	@Override
	public void cleanup() {
		button.setOnClickListener(null);
		ptr.Clear();
		ptr = null;
	}

}
