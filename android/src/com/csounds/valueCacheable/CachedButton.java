/* 
 
 CachedButton.java:
 
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
