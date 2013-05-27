/* 
 
 MultiTouchXYActivity.java:
 
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

package com.csounds.examples.tests;

import java.io.File;

import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;
import com.csounds.examples.BaseCsoundActivity;
import com.csounds.examples.R;
import com.csounds.valueCacheable.CsoundValueCacheable;

import csnd6.CsoundMYFLTArray;

public class MultiTouchXYActivity extends BaseCsoundActivity implements
		CsoundObjCompletionListener, CsoundValueCacheable {

	public View multiTouchView;
	
	int touchIds[] = new int[10];
	float touchX[] = new float[10];
	float touchY[] = new float[10];
	CsoundMYFLTArray touchXPtr[] = new CsoundMYFLTArray[10];
	CsoundMYFLTArray touchYPtr[] = new CsoundMYFLTArray[10];

	protected int getTouchIdAssignment() {
		for(int i = 0; i < touchIds.length; i++) {
			if(touchIds[i] == -1) {
				return i;
			}
		}
		return -1;
	}
	
	protected int getTouchId(int touchId) {
		for(int i = 0; i < touchIds.length; i++) {
			if(touchIds[i] == touchId) {
				return i;
			}
		}
		return -1;
	}
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		for(int i = 0; i < touchIds.length; i++) {
			touchIds[i] = -1;
			touchX[i] = -1;
			touchY[i] = -1;
		}
		
		multiTouchView = new View(this);
		
		multiTouchView.setOnTouchListener(new OnTouchListener() {

			public boolean onTouch(View v, MotionEvent event) {
				final int action = event.getAction() & MotionEvent.ACTION_MASK;
				switch(action) {
				case MotionEvent.ACTION_DOWN:
				case MotionEvent.ACTION_POINTER_DOWN:
					
					for(int i = 0; i < event.getPointerCount(); i++) {
						int pointerId = event.getPointerId(i);
						int id = getTouchId(pointerId);
						
						if(id == -1) {
							
							id = getTouchIdAssignment();
							
							if(id != -1) {
								touchIds[id] = pointerId;
								touchX[id] = event.getX(i) / multiTouchView.getWidth();
								touchY[id] = 1 - (event.getY(i) / multiTouchView.getHeight());
								
								if(touchXPtr[id] != null) {
									touchXPtr[id].SetValue(0, touchX[id]);
									touchYPtr[id].SetValue(0, touchY[id]);
									
									csoundObj.sendScore(String.format("i1.%d 0 -2 %d", id, id));
								}
							}
						}
						
					}
					
					break;
				case MotionEvent.ACTION_MOVE:

					for(int i = 0; i < event.getPointerCount(); i++) {
						int pointerId = event.getPointerId(i);
						int id = getTouchId(pointerId);

						if(id != -1) {
							touchX[id] = event.getX(i) / multiTouchView.getWidth();
							touchY[id] = 1 - (event.getY(i) / multiTouchView.getHeight());
						}
						
					}
					break;
				case MotionEvent.ACTION_POINTER_UP:
				case MotionEvent.ACTION_UP:
				{
					int activePointerIndex = event.getActionIndex();
					int pointerId = event.getPointerId(activePointerIndex);
						
					int id = getTouchId(pointerId);
					if(id != -1) {
						touchIds[id] = -1;
						csoundObj.sendScore(String.format("i-1.%d 0 0 %d", id, id));
					}
					
				}
					break;
				}
				return true;
			}
			
		});
		
		setContentView(multiTouchView);

		String csd = getResourceFileAsString(R.raw.multitouch_xy);
		File f = createTempFile(csd);

		csoundObj.addValueCacheable(this);

		csoundObj.startCsound(f);
	}

	public void csoundObjComplete(CsoundObj csoundObj) {

	}
	
	// VALUE CACHEABLE

	public void setup(CsoundObj csoundObj) {
		for(int i = 0; i < touchIds.length; i++) {
			touchXPtr[i] = csoundObj.getInputChannelPtr(String.format("touch.%d.x", i));
			touchYPtr[i] = csoundObj.getInputChannelPtr(String.format("touch.%d.y", i));
		}
	}

	public void updateValuesToCsound() {
		for(int i = 0; i < touchX.length; i++) {
			touchXPtr[i].SetValue(0, touchX[i]);
			touchYPtr[i].SetValue(0, touchY[i]);
		}
		
	}

	public void updateValuesFromCsound() {
	}

	public void cleanup() {
		for(int i = 0; i < touchIds.length; i++) {
			touchXPtr[i].Clear();
			touchXPtr[i] = null;
			touchYPtr[i].Clear();
			touchYPtr[i] = null;
		}
	}

}
