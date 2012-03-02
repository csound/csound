package com.csounds.valueCacheable;

import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.csounds.CsoundObj;

import csnd.CsoundMYFLTArray;

public class CachedSlider extends AbstractValueCacheable{
	private SeekBar seekBar;
	private String channelName;
	private CsoundObj csoundObj;
	protected double minValue, maxValue, cachedValue;
	boolean cacheDirty = true;
	CsoundMYFLTArray ptr;

	public CachedSlider(SeekBar seekBar, String channelName, double min, double max) {
		this.seekBar = seekBar;
		this.channelName = channelName;
		this.minValue = min;
		this.maxValue = max;
		
		seekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
			
			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				
				if(fromUser) {
					double percent = progress / (double)seekBar.getMax();
					double value = (percent * (maxValue - minValue)) + minValue;
					
					if(value != cachedValue) {
						cachedValue = value;
						cacheDirty = true;
					}
				}
				
			}
		});
	}

	@Override
	public void setup(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
		
		double percent = seekBar.getProgress() / (double)seekBar.getMax();
		cachedValue = (percent * (maxValue - minValue)) + minValue;
		cacheDirty = true;
		
		ptr = csoundObj.getInputChannelPtr(channelName);

	}
	
	@Override
	public void updateValuesToCsound() {
		if (cacheDirty) {
			ptr.SetValue(0, this.cachedValue);
			cacheDirty = false;
		}
	}

	@Override
	public void cleanup() {
		seekBar.setOnSeekBarChangeListener(null);
		ptr.Clear();
		ptr = null;
	}
	
	
}
