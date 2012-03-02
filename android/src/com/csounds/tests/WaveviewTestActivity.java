package com.csounds.tests;

import java.io.File;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.Bundle;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ToggleButton;

import com.csounds.BaseCsoundActivity;
import com.csounds.CsoundObj;
import com.csounds.R;
import com.csounds.valueCacheable.CsoundValueCacheable;

import csnd.Csound;
import csnd.CsoundMYFLTArray;

public class WaveviewTestActivity extends BaseCsoundActivity {

	ToggleButton startStopButton = null;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		final WaveView view = new WaveView(this);

		setContentView(view);

		String csd = getResourceFileAsString(R.raw.waveviewtest);
		File f = createTempFile(csd);

		csoundObj.addValueCacheable(view);

		csoundObj.startCsound(f);

	}

	class WaveView extends View implements CsoundValueCacheable {

		boolean tableLoaded = false;
		CsoundObj csoundObj = null;
		double[] tableData = null;
		int[] points = null;

		Paint paint;

		public WaveView(Context context) {
			super(context);
			paint = new Paint();
			paint.setDither(true);
			paint.setColor(0xFFFFFF00);
			paint.setStyle(Paint.Style.STROKE);
			paint.setStrokeJoin(Paint.Join.ROUND);
			paint.setStrokeCap(Paint.Cap.ROUND);
			paint.setStrokeWidth(3);
		}

		@Override
		public void setup(CsoundObj csoundObj) {
			tableLoaded = false;
			this.csoundObj = csoundObj;
		}

		@Override
		public void updateValuesToCsound() {
			// TODO Auto-generated method stub

		}

		@Override
		public void updateValuesFromCsound() {
			if (!tableLoaded) {
				Csound csound = csoundObj.getCsound();
				CsoundMYFLTArray table = new CsoundMYFLTArray();
				int length = csound.TableLength(1);
				csound.GetTable(table.GetPtr(), 1);
				tableData = new double[length];

				for (int i = 0; i < length; i++) {
					tableData[i] = table.GetValue(i);
				}

				tableLoaded = true;

				new Thread() {
					@Override
					public void run() {

						int width = getWidth();
						int height = getHeight();
						int middle = height / 2;

						points = new int[width];

						int tableLength = tableData.length;

						for (int i = 0; i < width; i++) {
							float percent = i / (float) (width);
							int index = (int) (percent * tableLength);
							points[i] = (int) ((tableData[index] * middle) + middle);
						}

						postInvalidate();

					}
				}.start();
			}
		}

		@Override
		public void cleanup() {
			csoundObj = null;
			tableData = null;
			points = null;
		}

		@Override
		public void onDraw(Canvas canvas) {
			super.onDraw(canvas);
			if(points == null || points.length == 0) {
				return;
			}
			
			int currentX = 0;
			int currentY = points[0];
			
			for(int i = 1;i < points.length; i++) {
				int nextX = i;
				int nextY = points[i];
				
				canvas.drawLine(currentX, currentY, nextX, nextY, paint);
				currentX = nextX;
				currentY = nextY;
			}
			
		}

	}

}
