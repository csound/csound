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

import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.widget.Button;

import com.csounds.CsoundObj;

import csnd.CsoundMYFLTArray;

public class CachedButton extends AbstractValueCacheable {

	private CsoundObj csoundObj;
	private Button button;
	private String channelName;
	private int type;
	CsoundMYFLTArray ptr, ptrX, ptrY;
	double xpos, ypos;
	
	boolean selected = false;
	boolean cacheDirty = false;
	
	public CachedButton(Button button, String channelName){
		this.button = button;
		this.channelName = channelName;
		this.type = 0; 
	}
	
	public CachedButton(Button button, String channelName, int type) {
		this.button = button;
		this.channelName = channelName;
		this.type = type;
	}
	
	@Override
	public void setup(CsoundObj csoundObj) {
		this.csoundObj = csoundObj;
		
		if(type == 0){	
		button.setOnClickListener(new OnClickListener() {	
			public void onClick(View v) {
				selected = true;
				cacheDirty = true;
			}
		});
		}
		else {
			button.setOnTouchListener(new OnTouchListener(){	
				public boolean onTouch(View v, MotionEvent event) {
					final int action = event.getAction() & MotionEvent.ACTION_MASK;
					switch (action){
					case MotionEvent.ACTION_DOWN:
					case MotionEvent.ACTION_POINTER_DOWN:
					button.setPressed(true);
					selected = true;
					break;
					case MotionEvent.ACTION_POINTER_UP:
					case MotionEvent.ACTION_UP:
					selected = false;
					button.setPressed(false);
					break;
					case MotionEvent.ACTION_MOVE:
					break;
					}
					if (selected){
					xpos = event.getX()/v.getWidth();
					ypos = 1. - (event.getY()/v.getHeight());
					} else xpos = ypos = 0.;
					return true;
				}
			});
			ptrX = csoundObj.getInputChannelPtr(channelName + ".x");
			ptrY = csoundObj.getInputChannelPtr(channelName + ".y");
		}
		ptr = csoundObj.getInputChannelPtr(channelName);
	}
	
	
	@Override
	public void updateValuesToCsound() {
		if(type == 0){
		if (csoundObj != null && cacheDirty) {
			ptr.SetValue(0, (selected ? 1. : 0.));		
			cacheDirty = selected;
			selected = false;
		}
		} else {
			ptr.SetValue(0, (selected ? 1. : 0.));		
			ptrX.SetValue(0, xpos);	
			ptrY.SetValue(0, ypos);	
		}
		
	}

	@Override
	public void cleanup() {
		if(type==0) button.setOnClickListener(null);
		else button.setOnTouchListener(null);
		ptr.Clear();
		ptr = null;
	}

}
