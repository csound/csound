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
import javax.swing.JSlider;
import com.csounds.CsoundObj;
import csnd.CsoundMYFLTArray;

public class CachedSlider extends AbstractValueCacheable{
	private JSlider slider;
	private String channelName;
	private CsoundObj csoundObj;
	protected double minValue, maxValue, cachedValue;
	boolean cacheDirty = true;
	CsoundMYFLTArray ptr;

	public CachedSlider(JSlider slider, String channelName, double min, double max) {
		this.slider = slider;
		this.channelName = channelName;
		this.minValue = min;
		this.maxValue = max;
		
		slider.addChangeListener(new SliderChangeListener());

                class SliderChangeListener implements ChangeListener {
			
			@Override
			public void onStateChanged(ChangeEvent e) {
				
                                    if (!source.getValueIsAdjusting()) {
                                        double progres = source.getValue();
					double percent = progress / (double) slider.getMax();
					double value = (percent * (maxValue - minValue)) + minValue;
					
					if(value != cachedValue) {
						cachedValue = value;
						cacheDirty = true;
					}
				    
				
				    }
			}
		}

	@Override
	public void setup(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
		
		double percent = slider.getValue() / (double)slider.getMax();
		cachedValue = (percent * (maxValue - minValue)) + minValue;
		cacheDirty = true;
		
		ptr = this.csoundObj.getInputChannelPtr(channelName);

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
