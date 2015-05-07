package com.csounds.Csound6;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.util.ArrayList;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ContextThemeWrapper;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.webkit.JavascriptInterface;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListAdapter;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import com.csounds.CsoundObj;
import com.csounds.CsoundObjListener;
import com.csounds.bindings.motion.CsoundMotion;
import com.csounds.bindings.ui.CsoundUI;

import csnd6.Csound;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
@SuppressWarnings("unused")
public class CsoundAppActivity extends Activity implements CsoundObjListener,
		CsoundObj.MessagePoster {
	Uri templateUri = null;
	Button newButton = null;
	Button openButton = null;
	Button saveAsButton = null;
	Button editButton = null;
	ToggleButton startStopButton = null;
	MenuItem helpItem = null;
	MenuItem aboutItem = null;
	JSCsoundObj csound = null;
	CsoundUI csoundUI = null;
	File csd = null;
	Button pad = null;
	WebView webLayout = null;
	LinearLayout channelsLayout = null;
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
	String html5Page = null;
	URL baseUrl = null;
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
		} catch (Throwable e) {
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
			// java.lang.System.exit(1);
		}
		try {
			result = csnd6.csnd
					.csoundInitialize(csnd6.csnd.CSOUNDINIT_NO_ATEXIT);
		} catch (Throwable e) {
			java.lang.System.err.println("Csound6: csoundInitialize failed.\n"
					+ e);
			java.lang.System.err.println(e.toString());
			// java.lang.System.exit(1);
		}
	}

	public void csoundObjStarted(CsoundObj csoundObj) {
	}

	public void csoundObjCompleted(CsoundObj csoundObj) {
		runOnUiThread(new Runnable() {
			public void run() {
				startStopButton.setChecked(false);
				postMessage("Csound has finished.\n");
			}
		});
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
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

	private static void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while((read = in.read(buffer)) != -1){
          out.write(buffer, 0, read);
        }
    }
	
	private boolean copyAsset(String fromAssetPath, String toPath) {
		AssetManager assetManager = getAssets();
        InputStream in = null;
        OutputStream out = null;
        try {
          in = assetManager.open(fromAssetPath);
          new File(toPath).createNewFile();
          out = new FileOutputStream(toPath);
          copyFile(in, out);
          in.close();
          in = null;
          out.flush();
          out.close();
          out = null;
          return true;
        } catch(Exception e) {
            e.printStackTrace();
            return false;
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
			goToUrl("http://csound.github.io/docs/manual/indexframes.html");
			return true;
		case R.id.itemAbout:
			goToUrl("http://csound.github.io/");
			return true;
		case R.id.itemSettings:
			Intent intent = new Intent(this, SettingsActivity.class);
			startActivity(intent);
			return true;
		case R.id.itemTrapped:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "trapped.csd");
	        if (copyAsset("Csound6AndroidExamples/Boulanger/trapped.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemDroneIV:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "Drone-IV.csd");
	        if (copyAsset("Csound6AndroidExamples/Gogins/Drone-IV.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemLuaScoregen:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "lua_scoregen.csd");
	        if (copyAsset("Csound6AndroidExamples/Gogins/lua_scoregen.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemChimePad:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "0.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/0.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "1.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/1.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "2.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/2.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "3.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/3.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "4.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/4.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "5.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/5.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "10.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/10.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "11.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/11.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "12.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/12.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "13.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/13.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "14.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/14.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "15.wav");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/15.wav", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "ChimePadReadMe.txt");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/ChimePadReadMe.txt", outFile.getAbsolutePath()) == false) {
	        	return true;
	        } 
	        outFile = new File(getExternalFilesDir(null), "ChimePad.csd");
	        if (copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/ChimePad.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemPartikkel:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "partikkel.csd");
	        if (copyAsset("Csound6AndroidExamples/Khosravi/partikkel.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemXanadu:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "xanadu.csd");
	        if (copyAsset("Csound6AndroidExamples/Kung/xanadu.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		case R.id.itemKoanI:
		{
			// Copy the asset to storage and set the filename.
	        File outFile = new File(getExternalFilesDir(null), "i.csd");
	        if (copyAsset("Csound6AndroidExamples/McCurdy/i.csd", outFile.getAbsolutePath()) == true) {
	        	OnFileChosen(outFile);
	        } 
		}
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}

	@JavascriptInterface
	public void postMessage(String message_) {
		postMessageClear_(message_, false);
	}

	@JavascriptInterface
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

	/**
	 * Opens the CSD and searches for a <CsHtml5> element. If found, hide the
	 * channelsLayout, show the webLayout, and set the contents of the CsHtml5
	 * element as the content of the Web view.
	 */
	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	protected void parseWebLayout() {
		try {
			FileReader in = new FileReader(csd);
			StringBuilder contents = new StringBuilder();
			char[] buffer = new char[4096];
			int read = 0;
			do {
				contents.append(buffer, 0, read);
				read = in.read(buffer);
			} while (read >= 0);
			in.close();
			String csdText = contents.toString();
			int start = csdText.indexOf("<CsHtml5>") + 9;
			int end = csdText.indexOf("</CsHtml5>");
			if (!(start == -1 || end == -1)) {
				html5Page = csdText.substring(start, end);
				if (html5Page.length() > 1) {
					webLayout.setVisibility(View.VISIBLE);
					channelsLayout.setVisibility(View.GONE);
					WebSettings settings = webLayout.getSettings();
					// Page itself must specify utf-8 in meta tag?
					settings.setDefaultTextEncodingName("utf-8");
					settings.setDomStorageEnabled(true);
					settings.setDatabaseEnabled(true);
					settings.setBuiltInZoomControls(true);
					settings.setDisplayZoomControls(false);
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
						settings.setAllowFileAccessFromFileURLs(true);
						settings.setAllowUniversalAccessFromFileURLs(true);
					}
					if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
						settings.setJavaScriptEnabled(true);
					}
					File basePath = csd.getParentFile();
					baseUrl = basePath.toURI().toURL();
					webLayout.loadDataWithBaseURL(baseUrl.toString(),
							html5Page, "text/html", "utf-8", null);
				}
			} else {
				webLayout.onPause();
				webLayout.pauseTimers();
				webLayout.setVisibility(View.GONE);
				channelsLayout.setVisibility(View.VISIBLE);
			}
			View mainLayout = findViewById(R.id.mainLayout);
			mainLayout.invalidate();
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	private void OnFileChosen(File file) {
		Log.d("FILE CHOSEN", file.getAbsolutePath());
		csd = file;
		setTitle(csd.getName());
		parseWebLayout();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		try {
			csound.stop();
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

	/**
	 * Called when the activity is first created.
	 */
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
		final AudioManager audioManager = (AudioManager) getSystemService(AUDIO_SERVICE);
		audioManager.setRingerMode(AudioManager.RINGER_MODE_SILENT);
		PreferenceManager.setDefaultValues(this, R.xml.settings, false);
		OPCODE6DIR = getBaseContext().getApplicationInfo().nativeLibraryDir;
		SharedPreferences sharedPreferences = PreferenceManager
				.getDefaultSharedPreferences(this);
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
				//loadFileList();
				//showDialog(BROWSE_DIALOG);
			    //Create FileOpenDialog and register a callback
			    SimpleFileDialog fileOpenDialog =  new SimpleFileDialog(
			        new ContextThemeWrapper(CsoundAppActivity.this, R.style.csoundAlertDialogStyle),
			        "FileOpen..",
			        new SimpleFileDialog.SimpleFileDialogListener()
			        {
			            @Override
			            public void onChosenDir(String chosenDir) 
			            {
			                CsoundAppActivity.this.OnFileChosen(new File(chosenDir));
			            }
			        }
			    );
			    if (csd != null){
			    	fileOpenDialog.default_file_name = csd.getAbsolutePath();
			    } else {
			    	fileOpenDialog.default_file_name = Environment.getExternalStorageDirectory().getAbsolutePath();
			    }
			    fileOpenDialog.chooseFile_or_Dir(fileOpenDialog.default_file_name);
			}				
		});
		saveAsButton = (Button) findViewById(R.id.saveAsButton);
		saveAsButton.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				//loadFileList();
				//showDialog(BROWSE_DIALOG);
			    //Create FileOpenDialog and register a callback
			    SimpleFileDialog fileOpenDialog =  new SimpleFileDialog(
			        new ContextThemeWrapper(CsoundAppActivity.this, R.style.csoundAlertDialogStyle),
			        "FileSave..",
			        new SimpleFileDialog.SimpleFileDialogListener()
			        {
			            @Override
			            public void onChosenDir(String chosenDir) 
			            {
			            	int index = chosenDir.indexOf("//");
			            	if (index >= 0) {
			            		chosenDir = chosenDir.substring(index + 1);
			            	}
			            	File newFile = new File(chosenDir);
			            	if (csd.equals(newFile)) {
			            		Context context = getApplicationContext();
			            		CharSequence text = "'Save as' aborted; the new file is the same as the old file!";
			            		int duration = Toast.LENGTH_SHORT;
			            		Toast toast = Toast.makeText(context, text, duration);
			            		toast.show();			            		
			            	} else {
			                    try {
			                        FileInputStream in = new FileInputStream(csd);
			                        FileOutputStream out = new FileOutputStream(newFile);
			                        copyFile(in, out);
			                        in.close();
			                        in = null;
			                        out.flush();
			                        out.close();
			                        out = null;
					                CsoundAppActivity.this.OnFileChosen(newFile);
			                      } catch (Exception e) {
			                          e.printStackTrace();
			                      }			            		
			            	}
			            }
			        }
			    );
			    if (csd != null){
			    	fileOpenDialog.default_file_name = csd.getParent();
			    } else {
			    	fileOpenDialog.default_file_name = Environment.getExternalStorageDirectory().getAbsolutePath();
			    }
			    fileOpenDialog.chooseFile_or_Dir(fileOpenDialog.default_file_name);
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
		channelsLayout = (LinearLayout) findViewById(R.id.channelsLayout);
		// By default, the WebView is not shown, and the channels layout is
		// shown.
		webLayout = (WebView) findViewById(R.id.webLayout);
		webLayout.setWebViewClient(new WebViewClient() {
			public void onReceivedError(WebView view, int errorCode,
					String description, String failingUrl) {
				Toast.makeText(CsoundAppActivity.this,
						"WebView error! " + description, Toast.LENGTH_SHORT)
						.show();
			}
		});
		webLayout.setVisibility(View.GONE);
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
					// This must be set before the Csound object is created.
					csnd6.csndJNI.csoundSetGlobalEnv("OPCODE6DIR", OPCODE6DIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SFDIR", SFDIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SSDIR", SSDIR);
					csnd6.csndJNI.csoundSetGlobalEnv("SADIR", SADIR);
					csnd6.csndJNI.csoundSetGlobalEnv("INCDIR", INCDIR);
					csound = new JSCsoundObj();
					csoundUI = new CsoundUI(csound);
					csound.messagePoster = CsoundAppActivity.this;
					csound.setMessageLoggingEnabled(true);
					webLayout.addJavascriptInterface(csound, "csound");
					webLayout.addJavascriptInterface(CsoundAppActivity.this,
							"csoundApp");
					// Csound will not be in scope of any JavaScript on the page
					// until
					// the page is reloaded. Also, we want to show any edits to
					// the page.
					parseWebLayout();
					postMessageClear("Csound is starting...\n");
					String framesPerBuffer = audioManager
							.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
					postMessage("Android sample frames per audio buffer: "
							+ framesPerBuffer + "\n");
					String framesPerSecond = audioManager
							.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
					postMessage("Android sample frames per second: "
							+ framesPerSecond + "\n");
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
						csoundUI.addSlider(sliders.get(i), channelName, 0., 1.);
						channelName = "butt" + (i + 1);
						csoundUI.addButton(buttons.get(i), channelName, 1);
					}
					csoundUI.addButton(pad, "trackpad", 1);
					CsoundMotion motion = new CsoundMotion(csound);
					motion.enableAccelerometer(CsoundAppActivity.this);
					csound.addListener(CsoundAppActivity.this);
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
					csound.stop();
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
				@SuppressWarnings("deprecation")
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

	@JavascriptInterface
	public void setControlChannel(String channelName, double value) {
		if (csound != null) {
			Csound csound_ = csound.getCsound();
			if (csound_ != null) {
				// This call is thread-safe.
				csound_.SetChannel(channelName, value);
			}
		}
	}

	@JavascriptInterface
	public double getControlChannel(String channelName) {
		double value = 0;
		if (csound != null) {
			Csound csound_ = csound.getCsound();
			if (csound_ != null) {
				// This call is thread-safe.
				value = csound_.GetChannel(channelName);
			}
		}
		return value;
	}
}