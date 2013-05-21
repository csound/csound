/* 
 
 CachedAccelerometer.java:
 
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

import java.util.List;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

import com.csounds.CsoundObj;

import csnd.CsoundMYFLTArray;

public class CachedAccelerometer extends AbstractValueCacheable implements SensorEventListener {

	private static final String CS_ACCEL_X = "accelerometerX";
	private static final String CS_ACCEL_Y = "accelerometerY";
	private static final String CS_ACCEL_Z = "accelerometerZ";
	
	CsoundMYFLTArray channelPtrX;
	CsoundMYFLTArray channelPtrY;
	CsoundMYFLTArray channelPtrZ;
	
	double x, y, z;
	
	Context context;
	
	SensorManager sensorManager;
	Sensor sensor;
	
	public CachedAccelerometer(Context context) {
		this.context = context;
		x = y = z = 0;
	}
	
	@Override
	public void setup(CsoundObj csoundObj) {
		
		 sensorManager = (SensorManager) context.
	                getSystemService(Context.SENSOR_SERVICE);
	        List<Sensor> sensors = sensorManager.getSensorList(
	                Sensor.TYPE_ACCELEROMETER);
	        
	    if(sensors.size() > 0) {
	    	sensor = sensors.get(0);
	    	
	    	sensorManager.registerListener(this, sensor, SensorManager.SENSOR_DELAY_FASTEST);
	    	
	    	channelPtrX = csoundObj.getInputChannelPtr(CS_ACCEL_X);
			channelPtrY = csoundObj.getInputChannelPtr(CS_ACCEL_Y);
			channelPtrZ = csoundObj.getInputChannelPtr(CS_ACCEL_Z);	
	    } else {
	    	Log.d("CachedAccelerometer", "Unable to get Accelerometer sensor.");
	    }
		
	}

	@Override
	public void updateValuesToCsound() {
		if(channelPtrX != null) {
			channelPtrX.SetValue(0, x);
			channelPtrY.SetValue(0, y);
			channelPtrZ.SetValue(0, z);
		}
	}
	
	@Override
	public void cleanup() {
		if(channelPtrX != null) {
			channelPtrX.Clear();
			channelPtrY.Clear();
			channelPtrZ.Clear();
			
			channelPtrX = null;
			channelPtrY = null;
			channelPtrZ = null;
			
			sensorManager.unregisterListener(this);
		}
	}

	// SENSOR EVENT LISTENER
	
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		
	}

	public void onSensorChanged(SensorEvent event) {
		  x = event.values[0];  
          y = event.values[1]; 
          z = event.values[2];
   	}

}
