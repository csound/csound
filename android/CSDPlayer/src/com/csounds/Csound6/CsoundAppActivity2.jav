package com.csounds.Csound6;

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
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListAdapter;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;

import csnd6.Csound;
import csnd6.CsoundCallbackWrapper;

public class CsoundAppActivity extends Activity  implements CsoundObjCompletionListener {
	Button browseButton;
	Button newButton;
	Button editButton;
	ToggleButton startStopButton = null;
	CsoundObj csound = null;
	File csd = null;
	Button pad;
	ArrayList<SeekBar> sliders = new ArrayList<SeekBar>();
	ArrayList<Button> buttons = new ArrayList<Button>();
	ArrayList<String> str = new ArrayList<String>();
	CsoundCallbackWrapper callbacks = null;
	private Boolean firstLvl = true;
	private Item[] fileList;
	private File path = new File(Environment.getExternalStorageDirectory() + "");
	private String chosenFile;
	private static final int BROWSE_DIALOG = 0xFFFFFFFF;
	private static final int ERROR_DIALOG = 0xFFFFFFF0;
	ListAdapter adapter;
	protected Handler handler = new Handler();
	boolean running = false;
	private TextView messageTextView = null;
	private ScrollView messageScrollView = null;
	String errorMessage;

	public void csoundObjComplete(CsoundObj csoundObj) {
		handler.post(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
				displayLog();
				running = false;
			}
		});
		//displayLog();		
	}	
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.menu, menu);
	    return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    switch (item.getItemId()) {
	        case R.id.itemNew:
				if(!running) {
					Intent intent = new Intent(Intent.ACTION_GET_CONTENT); 
					Uri uri = Uri.parse("file://template.csd"); 
					intent.setDataAndType(uri, "text/plain"); 
					startActivityForResult(intent, R.id.itemNew);						
				}
	            return true;
	        case R.id.itemOpen:
				if(!running) {
					loadFileList();
					showDialog(BROWSE_DIALOG); 
				}
	            return true;
	        case R.id.itemEdit:
				if(!running && csd != null) {
					Intent intent = new Intent(Intent.ACTION_VIEW); 
					Uri uri = Uri.parse("file://" + csd.getAbsolutePath()); 
					intent.setDataAndType(uri, "text/plain"); 
					startActivity(intent);						
				}
	            return true;
	        case R.id.itemRun:
				if (csd == null) {
					return true;
				}
				Log.d("CSD", csd.getAbsolutePath());		
				if(item.getTitle() == this.getText(R.string.Start)) {
				    messageTextView.setText("Csound is starting...\n");
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
					callbacks = new CsoundCallbackWrapper(csound.getCsound()) {
						@Override
						public void MessageCallback(int attr, String msg) {
							postMessage(msg);
							super.MessageCallback(attr, msg);
						}
					};
					item.setTitle(getString(R.string.Stop));
					callbacks.SetMessageCallback();
				} else {
					csound.stopCsound();
					item.setTitle(getString(R.string.Start));
					running = false;
				}
	            return true;
	        default:
	            return super.onOptionsItemSelected(item);
	    }
	}
	private synchronized void postMessage(String message_)
	{
		final String message = message_;
		CsoundAppActivity.this.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				messageTextView.append(message);
				messageTextView.invalidate();
				messageScrollView.fullScroll(ScrollView.FOCUS_DOWN);
			}
		});
	}

	private void displayLog(){
      try {
	      Process process = Runtime.getRuntime().exec("logcat -dt 8 CsoundObj:D AndroidCsound:D *:S");
	      BufferedReader bufferedReader = new BufferedReader(
	      new InputStreamReader(process.getInputStream()));
	      String line;
	      postMessage("Csound system log:\n");
	      while ((line = bufferedReader.readLine()) != null) {
	        postMessage(line + "\n");
	      }
	      postMessage("Csound has stopped.\n");
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
		newButton = (Button) findViewById(R.id.newButton);
		newButton.setOnClickListener(new OnClickListener(){ 
			public void onClick(View v){
				if(!running) {
					Intent intent = new Intent(Intent.ACTION_GET_CONTENT); 
					Uri uri = Uri.parse("file://template.csd"); 
					intent.setDataAndType(uri, "text/plain"); 
					startActivityForResult(intent, R.id.newButton);						
				}
			}
		});
		browseButton = (Button) findViewById(R.id.browseButton);
		browseButton.setOnClickListener(new OnClickListener(){ 
			public void onClick(View v){
				if(!running) {
					loadFileList();
					showDialog(BROWSE_DIALOG); 
				}
			}
		});
		editButton = (Button) findViewById(R.id.editButton);
		editButton.setOnClickListener(new OnClickListener(){ 
			public void onClick(View v){
				if(!running && csd != null) {
					Intent intent = new Intent(Intent.ACTION_VIEW); 
					Uri uri = Uri.parse("file://" + csd.getAbsolutePath()); 
					intent.setDataAndType(uri, "text/plain"); 
					startActivity(intent);						
				}
			}
		});
		messageTextView = (TextView)findViewById(R.id.messageTextView);
		messageScrollView = (ScrollView) findViewById(R.id.messageScrollView);
		
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
				    messageTextView.setText("");
				    postMessage("Csound is running...\n");
					callbacks = new CsoundCallbackWrapper(csound.getCsound()) {
						@Override
						public void MessageCallback(int attr, String msg) {
							postMessage(msg);
							super.MessageCallback(attr, msg);
						}
					};
					callbacks.SetMessageCallback();
				} else {
					csound.stopCsound();
				    displayLog();
				    postMessage("Csound has stopped.");
					running = false;
				}
			}
		});
	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent intent)
	{
		try {
			if (requestCode == R.id.newButton && intent != null){
				csd = new File(intent.getData().getPath());
			}
		} catch (Exception e) {
			Log.e("error", e.toString());
		}
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