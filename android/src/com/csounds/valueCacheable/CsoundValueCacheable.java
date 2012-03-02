package com.csounds.valueCacheable;

import com.csounds.CsoundObj;

public interface CsoundValueCacheable {
	
	public void setup(CsoundObj csoundObj);
	
	public void updateValuesToCsound();
	
	public void updateValuesFromCsound();
	
	public void cleanup();
	
}
