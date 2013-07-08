package com.csounds.Csound6;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
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
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjCompletionListener;

import csnd6.Csound;
import csnd6.CsoundCallbackWrapper;

public class CsoundAppActivity extends Activity implements
		CsoundObjCompletionListener, CsoundObj.MessagePoster {
	Uri templateUri = null;
	Button newButton = null;
	Button openButton = null;
	Button editButton = null;
	ToggleButton startStopButton = null;
	MenuItem helpItem = null;
	MenuItem aboutItem = null;
	CsoundObj csound = null;
	File csd = null;
	Button pad = null;
	ArrayList<SeekBar> sliders = new ArrayList<SeekBar>();
	ArrayList<Button> buttons = new ArrayList<Button>();
	ArrayList<String> str = new ArrayList<String>();
	private Boolean firstLvl = true;
	private Item[] fileList = null;
	private File path = new File(Environment.getExternalStorageDirectory() + "");
	private String chosenFile;
	private static final int BROWSE_DIALOG = 0xFFFFFFFF;
	private static final int ERROR_DIALOG = 0xFFFFFFF0;
	ListAdapter adapter = null;
	protected Handler handler = new Handler();
	private TextView messageTextView = null;
	private ScrollView messageScrollView = null;
	String errorMessage = null;
	String csdTemplate = null;
	PackageInfo packageInfo = null;
	// Csound environment variables managed on Android.
	static String OPCODE6DIR = null;
	static String SFDIR = null;
	static String SSDIR = null;
	static String SADIR = null;
	static String INCDIR = null;
	WebView webview = null;

	static {
		int result = 0;
		try {
			java.lang.System.loadLibrary("gnustl_shared");
		} catch (Throwable e){
			java.lang.System.err
			.println("Csound6: gnustl_shared native code library failed to load.\n");
			java.lang.System.err.println(e.toString());
		}
		try {
			java.lang.System.loadLibrary("sndfile");
		} catch (Throwable e) {
			java.lang.System.err
			.println("Csound6: sndfile native code library failed to load.\n");
			java.lang.System.err.println(e.toString());
		}
		try {
			java.lang.System.loadLibrary("csoundandroid");
		} catch (Throwable e) {
			java.lang.System.err
					.println("Csound6: csoundandroid native code library failed to load.\n"
							+ e);
			java.lang.System.err.println(e.toString());
			//java.lang.System.exit(1);
		}
		try {
			result = csnd6.csnd.csoundInitialize(csnd6.csnd.CSOUNDINIT_NO_ATEXIT);
		} catch (Throwable e) {
			java.lang.System.err
					.println("Csound6: csoundInitialize failed.\n"
							+ e);
			java.lang.System.err.println(e.toString());
			//java.lang.System.exit(1);
		}
	}

	public void csoundObjComplete(CsoundObj csoundObj) {
		runOnUiThread(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
				postMessage("Csound has finished.\n");
			}
		});
	}

	protected void writeTemplateFile() {
		File root = Environment.getExternalStorageDirectory();
		try {
			if (root.canWrite()) {
				FileWriter filewriter = new FileWriter(csd);
				BufferedWriter out = new BufferedWriter(filewriter);
				out.write(csdTemplate);
				out.close();
			}
		} catch (IOException e) {
			Log.e("Csound6", "Could not write file " + e.getMessage());
		}
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
		case R.id.itemGuide:
			if (webview == null) {
				webview = new WebView(this);
				setContentView(webview);
				webview.loadUrl("file:///android_asset/Csound6_User_Guide.html");
			} else {
				setContentView(webview);
			}
			return true;
		case R.id.itemHelp:
			goToUrl("http://www.csounds.com/manual/html/index.html");
			return true;
		case R.id.itemAbout:
			goToUrl("http://www.csounds.com/about");
			return true;
		case R.id.itemSettings:
            Intent intent = new Intent(this, SettingsActivity.class);
            startActivity(intent);			
            return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	public void postMessage(String message_) {
		postMessageClear_(message_, false);
	}

	public void postMessageClear(String message_) {
		postMessageClear_(message_, true);
	}

	private synchronized void postMessageClear_(String message_,
			boolean doClear_) {
		final String message = message_;
		final boolean doClear = doClear_;
		CsoundAppActivity.this.runOnUiThread(new Runnable() {
			@Override
			public void run() {
				if (doClear == true) {
					messageTextView.setText("");
				}
				if (message == null) {
					return;
				}
				messageTextView.append(message);
				messageScrollView.fullScroll(ScrollView.FOCUS_DOWN);
				Log.i("Csound6", message);
			}
		});
	}

	private void goToUrl(String url) {
		Uri uriUrl = Uri.parse(url);
		Intent launchBrowser = new Intent(Intent.ACTION_VIEW, uriUrl);
		startActivity(launchBrowser);
	}

	private void displayLog() {
		try {
			Process process = Runtime.getRuntime().exec(
					"logcat -dt 16 CsoundObj:D AndroidCsound:D *:S");
			BufferedReader bufferedReader = new BufferedReader(
					new InputStreamReader(process.getInputStream()));
			String line;
			postMessage("Csound system log:\n");
			while ((line = bufferedReader.readLine()) != null) {
				postMessage(line + "\n");
			}
		} catch (IOException e) {
		}
	}

	public void setTitle(String title) {
		String fullTitle = "Csound6";
		if (title != null) {
			fullTitle = fullTitle + ": " + title;
		}
		super.setTitle(fullTitle);
	}

	private void OnFileChosen(File file) {
		Log.d("FILE CHOSEN", file.getAbsolutePath());
		csd = file;
		setTitle(csd.getName());
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		try {
			csound.stopCsound();
		} catch (Exception e) {
			Log.e("error", "could not stop csound");
		}
	}

	/**
	 * Quit performing on leaving the user interface of the app.
	 */
	public synchronized void onBackPressed() {
		if (webview != null) {
			setContentView(R.layout.main);
			webview = null;
		} else {
			finish();
		}
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		// We ask for the data directory in case Android changes on
		// us without warning.
		try {
			packageInfo = getPackageManager().getPackageInfo(getPackageName(),
					0);
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		}
		PreferenceManager.setDefaultValues(this, R.xml.settings, false);
		OPCODE6DIR = getBaseContext().getApplicationInfo().nativeLibraryDir;
		SharedPreferences sharedPreferences = PreferenceManager.getDefaultSharedPreferences(this);
		OPCODE6DIR = sharedPreferences.getString("OPCODE6DIR", OPCODE6DIR);
		SSDIR = packageInfo.applicationInfo.dataDir + "/samples";
		SSDIR = sharedPreferences.getString("SSDIR", SSDIR);
		SFDIR = sharedPreferences.getString("SFDIR", SFDIR);
		SADIR = sharedPreferences.getString("SADIR", SADIR);
		INCDIR = sharedPreferences.getString("INCDIR", INCDIR);
		// Pre-load plugin opcodes, not only to ensure that Csound
		// can load them, but for easier debugging if they fail to load.
		File file = new File(OPCODE6DIR);
		File[] files = file.listFiles();
		for (int i = 0; i < files.length; i++) {
			String pluginPath = files[i].getAbsoluteFile().toString();
			try {
				System.load(pluginPath);
			} catch (Throwable e) {
				postMessage(e.toString() + "\n");
			}
		}
		csdTemplate = "<CsoundSynthesizer>\n" + "<CsLicense>\n"
				+ "</CsLicense>\n" + "<CsOptions>\n" + "</CsOptions>\n"
				+ "<CsInstruments>\n" + "</CsInstruments>\n" + "<CsScore>\n"
				+ "</CsScore>\n" + "</CsoundSynthesizer>\n";
		setContentView(R.layout.main);
		newButton = (Button) findViewById(R.id.newButton);
		newButton.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				AlertDialog.Builder alert = new AlertDialog.Builder(
						CsoundAppActivity.this);
				alert.setTitle("New CSD...");
				alert.setMessage("Filename:");
				final EditText input = new EditText(CsoundAppActivity.this);
				alert.setView(input);
				alert.setPositiveButton("Ok",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									int whichButton) {
								String value = input.getText().toString();
								File root = Environment
										.getExternalStorageDirectory();
								csd = new File(root, value);
								writeTemplateFile();
								Intent intent = new Intent(Intent.ACTION_VIEW);
								Uri uri = Uri.parse("file://"
										+ csd.getAbsolutePath());
								intent.setDataAndType(uri, "text/plain");
								startActivity(intent);
							}
						});
				alert.setNegativeButton("Cancel",
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									int whichButton) {
							}
						});
				alert.show();
			}
		});
		openButton = (Button) findViewById(R.id.openButton);
		openButton.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				loadFileList();
				showDialog(BROWSE_DIALOG);
			}
		});
		editButton = (Button) findViewById(R.id.editButton);
		editButton.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				if (csd != null) {
					Intent intent = new Intent(Intent.ACTION_VIEW);
					Uri uri = Uri.parse("file://" + csd.getAbsolutePath());
					intent.setDataAndType(uri, "text/plain");
					startActivity(intent);
				}
			}
		});
		messageTextView = (TextView) findViewById(R.id.messageTextView);
		messageScrollView = (ScrollView) findViewById(R.id.messageScrollView);
		sliders.add((SeekBar) findViewById(R.id.seekBar1));
		sliders.add((SeekBar) findViewById(R.id.seekBar2));
		sliders.add((SeekBar) findViewById(R.id.seekBar3));
		sliders.add((SeekBar) findViewById(R.id.seekBar4));
		sliders.add((SeekBar) findViewById(R.id.seekBar5));
		buttons.add((Button) findViewById(R.id.button1));
		buttons.add((Button) findViewById(R.id.button2));
		buttons.add((Button) findViewById(R.id.button3));
		buttons.add((Button) findViewById(R.id.button4));
		buttons.add((Button) findViewById(R.id.button5));
		pad = (Button) findViewById(R.id.pad);
		startStopButton = (ToggleButton) findViewById(R.id.runButton);
		startStopButton.setOnClickListener(new OnClickListener() {
			public synchronized void onClick(View v) {
				if (csd == null) {
					startStopButton.toggle();
					return;
				}
				if (startStopButton.isChecked()) {
					File file = new File(OPCODE6DIR);
					File[] files = file.listFiles();
					CsoundAppActivity.this
							.postMessage("Loading Csound plugins:\n");
					for (int i = 0; i < files.length; i++) {
						String pluginPath = files[i].getAbsoluteFile()
								.toString();
						try {
							CsoundAppActivity.this.postMessage(pluginPath
									+ "\n");
							System.load(pluginPath);
						} catch (Throwable e) {
							CsoundAppActivity.this.postMessage(e.toString()
									+ "\n");
						}
					}
					// Evidently, this has to be set before
					// the very first Csound object is created.
					csnd6.csndJNI.csoundSetGlobalEnv("OPCODE6DIR", OPCODE6DIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SFDIR", SFDIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SSDIR", SSDIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SADIR", SADIR);
					csnd6.csndJNI.csoundSetGlobalEnv("INCDIR", INCDIR);
					csound = new CsoundObj();
					csound.messagePoster = CsoundAppActivity.this;
					csound.setMessageLoggingEnabled(true);
					postMessageClear("Csound is starting...\n");
					// csound.getCsound().SetGlobalEnv("OPCODE6DIR",
					// OPCODE6DIR);
					// csound.getCsound().SetGlobalEnv("SSDIR", SSDIR);
					// Make sure this stuff really got packaged.
					String samples[] = null;
					try {
						samples = getAssets().list("samples");
					} catch (IOException e) {
						e.printStackTrace();
					}
					String channelName;
					for (int i = 0; i < 5; i++) {
						channelName = "slider" + (i + 1);
						csound.addSlider(sliders.get(i), channelName, 0., 1.);
						channelName = "butt" + (i + 1);
						csound.addButton(buttons.get(i), channelName, 1);
					}
					csound.addButton(pad, "trackpad", 1);
					csound.enableAccelerometer(CsoundAppActivity.this);
					csound.addCompletionListener(CsoundAppActivity.this);
					csound.startCsound(csd);
					// Make sure these are still set after starting.
					String getOPCODE6DIR = csnd6.csndJNI.csoundGetEnv(0,
							"OPCODE6DIR");
					csound.getCsound().Message(
							"OPCODE6DIR has been set to: " + getOPCODE6DIR
									+ "\n");
					csound.getCsound()
							.Message(
									"SSDIR has been set to: "
											+ csound.getCsound()
													.GetEnv("SSDIR") + "\n");
				} else {
					csound.stopCsound();
					postMessage("Csound has been stopped.\n");
				}
			}
		});
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode,
			Intent intent) {
		try {
			if (requestCode == R.id.newButton && intent != null) {
				csd = new File(intent.getData().getPath());
				setTitle(csd.getName());
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
					} else if (chosenFile.equalsIgnoreCase("up")
							&& !sel.exists()) {
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
					} else
						OnFileChosen(sel);
				}
			});
			break;
		case ERROR_DIALOG:
			builder.setTitle(errorMessage);
			break;
		}
		dialog = builder.show();
		return dialog;
	}
}