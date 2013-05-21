/* 
 
 CachedSlider.java:
 
 Copyright (C) 2011 Victor Lazzarini, Steven Yi
 
 This file is part of Csound Android Examples.
 
 The Csound Android Examples is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.   
 
 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

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
	CsoundMYFLTArray ptr = null;

	public CachedSlider(SeekBar seekBar, String channelName, double min, double max) {
		this.seekBar = seekBar;
		this.channelName = channelName;
		this.minValue = min;
		this.maxValue = max;
		
		seekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
			
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
			
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}
			
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
		
		ptr = this.csoundObj.getInputChannelPtr(channelName);

	}
	
	@Override
	public void updateValuesToCsound() {
		if (cacheDirty) {
			if(ptr != null) ptr.SetValue(0, this.cachedValue);
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
