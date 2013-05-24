package com.csounds.CsoundApp;

import java.io.BufferedReader;
import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListAdapter;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;



public class CsoundAppActivity extends Activity  implements CsoundObjCompletionListener {
	Button browseButton;
	ToggleButton startStopButton = null;
	CsoundObj csound = null;
	File csd = null;
	Button pad;
	ArrayList<SeekBar> sliders = new ArrayList<SeekBar>();
	ArrayList<Button> buttons = new ArrayList<Button>();
	ArrayList<String> str = new ArrayList<String>();
	private Boolean firstLvl = true;
	private Item[] fileList;
	private File path = new File(Environment.getExternalStorageDirectory() + "");
	private String chosenFile;
	private static final int BROWSE_DIALOG = 0xFFFFFFFF;
	private static final int ERROR_DIALOG = 0xFFFFFFF0;
	ListAdapter adapter;
	protected Handler handler = new Handler();
	boolean running = false;
	String errorMessage;

	public void csoundObjComplete(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
				if(csound.getError() != 0) displayError(csound.getError());
				running = false;
			}
		});
	}

   private void displayError(int error){
	  // errorMessage = "Csound Compilation Error ";
	  // showDialog(ERROR_DIALOG); 
      try {
	      Process process = Runtime.getRuntime().exec("logcat -dt 8 AndroidCsound:I *:S");
	      BufferedReader bufferedReader = new BufferedReader(
	      new InputStreamReader(process.getInputStream()));
	                       
	      StringBuilder log=new StringBuilder();
	      String line;
	      log.append("Csound Compile error:\n");
	      while ((line = bufferedReader.readLine()) != null) {
	        log.append(line + "\n");
	      }
	      TextView tv = (TextView)findViewById(R.id.textView7);
	      tv.setText(log.toString());
	       
   } catch (IOException e) { }
	   
   }

	private void OnFileChosen(File file){
		Log.d("FILE CHOSEN", file.getAbsolutePath());
		csd = file;		
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		try {
			csound.stopCsound();
		} catch (Exception e){
			Log.e("error", "could not stop csound");
		}

	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		browseButton = (Button) findViewById(R.id.browseButton);
		browseButton.setOnClickListener(new OnClickListener(){ 
			public void onClick(View v){
				if(!running) {
					loadFileList();
					showDialog(BROWSE_DIALOG); 
				}
			}
		});
		
		sliders.add((SeekBar)findViewById(R.id.seekBar1));
		sliders.add((SeekBar)findViewById(R.id.seekBar2));
		sliders.add((SeekBar)findViewById(R.id.seekBar3));
		sliders.add((SeekBar)findViewById(R.id.seekBar4));
		sliders.add((SeekBar)findViewById(R.id.seekBar5));

		buttons.add((Button)findViewById(R.id.button1));
		buttons.add((Button)findViewById(R.id.button2));
		buttons.add((Button)findViewById(R.id.button3));
		buttons.add((Button)findViewById(R.id.button4));
		buttons.add((Button)findViewById(R.id.button5));
		
		pad = (Button)findViewById(R.id.pad);

		startStopButton = (ToggleButton) findViewById(R.id.onOffButton);
		startStopButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
				if (csd == null) {
					buttonView.toggle();
					return;
				}
				Log.d("CSD", csd.getAbsolutePath());		
				if(isChecked) {
					csound = new CsoundObj();
					
					String channelName;
					for(int i = 0; i < 5; i++){
						channelName = "slider" + (i+1);
						csound.addSlider(sliders.get(i),channelName, 0., 1.);
						channelName = "butt" + (i+1);
						csound.addButton(buttons.get(i),channelName, 1);
					}
					csound.addButton(pad,"trackpad", 1);
					csound.enableAccelerometer(CsoundAppActivity.this);
					csound.addCompletionListener(CsoundAppActivity.this);
					csound.startCsound(csd);
					TextView tv = (TextView)findViewById(R.id.textView7);
				    tv.setText("Csound is running...");
			
				} else {
					csound.stopCsound();
					TextView tv = (TextView)findViewById(R.id.textView7);
				    tv.setText("Csound is stopped...");
					running = false;
				}
			}
		});
	}
	private void loadFileList() {
		try {
			path.mkdirs();
		} catch (SecurityException e) {
			Log.e("error", "unable to write on the sd card ");
		}

		
		if (path.exists()) {
			FilenameFilter filter = new FilenameFilter() {
				public boolean accept(File dir, String filename) {
					File sel = new File(dir, filename);
					return (sel.isFile() || sel.isDirectory())
							&& !sel.isHidden();
				}
			};

			String[] fList = path.list(filter);
			fileList = new Item[fList.length];
			for (int i = 0; i < fList.length; i++) {
				fileList[i] = new Item(fList[i], R.drawable.file_icon);
				File sel = new File(path, fList[i]);
				if (sel.isDirectory()) {
					fileList[i].icon = R.drawable.directory_icon;
				} 
			}
			if (!firstLvl) {
				Item temp[] = new Item[fileList.length + 1];
				for (int i = 0; i < fileList.length; i++) {
					temp[i + 1] = fileList[i];
				}
				temp[0] = new Item("Up", R.drawable.directory_up);
				fileList = temp;
			}
		} 
		adapter = new ArrayAdapter<Item>(this,
				android.R.layout.select_dialog_item, android.R.id.text1,
				fileList) {
			@Override
			public View getView(int position, View convertView, ViewGroup parent) {
				View view = super.getView(position, convertView, parent);
				TextView textView = (TextView) view
						.findViewById(android.R.id.text1);
				textView.setCompoundDrawablesWithIntrinsicBounds(
						fileList[position].icon, 0, 0, 0);
				int dp5 = (int) (5 * getResources().getDisplayMetrics().density + 0.5f);
				textView.setCompoundDrawablePadding(dp5);
				return view;
			}
		};
	}

	private class Item {
		public String file;
		public int icon;
		public Item(String file, Integer icon) {
			this.file = file;
			this.icon = icon;
		}

		@Override
		public String toString() {
			return file;
		}
	}

	@Override
	protected Dialog onCreateDialog(int id) {
		Dialog dialog = null;
		AlertDialog.Builder builder = new Builder(this);
		if (fileList == null) {
			dialog = builder.create();
			return dialog;
		}

		switch (id) {
		case BROWSE_DIALOG:
			builder.setTitle("Choose your file");
			builder.setAdapter(adapter, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					chosenFile = fileList[which].file;
					File sel = new File(path + "/" + chosenFile);
					if (sel.isDirectory()) {
						firstLvl = false;
						str.add(chosenFile);
						fileList = null;
						path = new File(sel + "");
						loadFileList();
						removeDialog(BROWSE_DIALOG);
						showDialog(BROWSE_DIALOG);
					}
					else if (chosenFile.equalsIgnoreCase("up") && !sel.exists()) {
						String s = str.remove(str.size() - 1);
						path = new File(path.toString().substring(0,
								path.toString().lastIndexOf(s)));
						fileList = null;
						if (str.isEmpty()) {
							firstLvl = true;
						}
						loadFileList();
						removeDialog(BROWSE_DIALOG);
						showDialog(BROWSE_DIALOG);
					}
					else OnFileChosen(sel);
				}
			});
			break;
		  case  ERROR_DIALOG:
			  builder.setTitle(errorMessage);
			  break;
		}
		dialog = builder.show();
		return dialog;
	}
}