package com.csounds.valueCacheable;

import com.csounds.CsoundObj;

public abstract class AbstractValueCacheable implements CsoundValueCacheable {

	@Override
	public abstract void setup(CsoundObj csoundObj);
	
	@Override
	public void updateValuesToCsound() {
	}

	@Override
	public void updateValuesFromCsound() {
	}

	@Override
	public abstract void cleanup();
}
