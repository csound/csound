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
import java.lang.reflect.Method;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;

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
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
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
import android.view.MotionEvent;
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
import csnd6.CsoundCallbackWrapper;
import csnd6.CsoundOboe;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
@SuppressWarnings("unused")
public class CsoundAppActivity extends Activity implements CsoundObjListener,
        CsoundObj.MessagePoster, SharedPreferences.OnSharedPreferenceChangeListener {
    boolean use_oboe = true;
    Uri templateUri = null;
    Button newButton = null;
    Button openButton = null;
    Button saveAsButton = null;
    Button editButton = null;
    ToggleButton startStopButton = null;
    MenuItem helpItem = null;
    MenuItem aboutItem = null;
    JSCsoundObj csound_obj = null;
    CsoundOboe csound_oboe = null;
    CsoundUI csoundUI = null;
    File csound_file = null;
    Button pad = null;
    WebView webView = null;
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
    private String screenLayout = "2";
    protected CsoundCallbackWrapper oboe_callback_wrapper = null;

    static {
        int result = 0;
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
                FileWriter filewriter = new FileWriter(csound_file);
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
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }

    // Copy the file indicated by fromAssetPath to
    // the public external storage music directory.
    // The asset directory becomes a subdirectory of the music directory.
    private File copyAsset(String fromAssetPath) {
        AssetManager assetManager = getAssets();
        InputStream in = null;
        OutputStream out = null;
        File outputFile = null;
        try {
            File externalStoragePublicDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC);
            outputFile = new File(externalStoragePublicDirectory.toString() + "/" + fromAssetPath);
            String filename = outputFile.getName();
            File directory = outputFile.getParentFile();
            directory.mkdirs();
            outputFile.createNewFile();
            in = assetManager.open(fromAssetPath);
            out = new FileOutputStream(outputFile);
            copyFile(in, out);
            in.close();
            in = null;
            out.flush();
            out.close();
            out = null;
            return outputFile;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    public void setEnv(String variable, String value) {
        try {
            Class<?> libcore = Class.forName("libcore.io.Libcore");
            if (libcore == null) {
                Log.w("Csound6", "Cannot find libcore.os.Libcore;");
                //                return;
            } else {
                final Object os = Class.forName("libcore.io.Libcore").getField("os").get(null);
                Method method = os.getClass().getMethod("setenv", String.class, String.class, boolean.class);
                method.invoke(os, variable, value, true);
            }
        } catch ( Exception e) {
            Log.w("Csound6", Log.getStackTraceString(e));
            Log.w("Csound6", e.getMessage());
        }
    }

    private void copyRawwaves() {
        try {
            String[] files = getAssets().list("rawwaves");
            for (int i = 0; i < files.length; i++) {
                String file = files[i];
                copyAsset("rawwaves/" + file.toString());
            }
            File externalStoragePublicDirectory = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MUSIC);
            String rawwavePath = externalStoragePublicDirectory.toString() + "/rawwaves/";
            setEnv("RAWWAVE_PATH", rawwavePath);
        } catch (Exception e) {
            e.printStackTrace();
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
        File outFile = null;
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
            case R.id.itemPrivacy:
                goToUrl("http://csound.github.io/csound_for_android_privacy.html");
                return true;
            case R.id.itemSettings:
                Intent intent = new Intent(this, SettingsActivity.class);
                startActivity(intent);
                return true;
            case R.id.itemTrapped: {
                outFile = copyAsset("Csound6AndroidExamples/Boulanger/trapped.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
            }
            return true;
            case R.id.itemDroneIV: {
                outFile = copyAsset("Csound6AndroidExamples/Gogins/Drone-IV.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
            }
            return true;
            case R.id.itemLuaScoregen: {
                outFile = copyAsset("Csound6AndroidExamples/Gogins/lua_scoregen.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
            }
            return true;
            case R.id.itemChimePad: {
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/0.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/1.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/2.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/3.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/4.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/5.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/10.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/11.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/12.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/13.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/14.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/15.wav");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/ChimePadReadMe.txt");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Hunkins/ChimePad/ChimePad.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
            }
            return true;
            case R.id.itemPartikkel: {
                outFile = copyAsset("Csound6AndroidExamples/Khosravi/partikkel.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
            }
            return true;
            case R.id.itemXanadu: {
                outFile = copyAsset("Csound6AndroidExamples/Kung/xanadu.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
                return true;
            }
            case R.id.itemLindenmayerCanvas: {
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/Silencio.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/ChordSpace.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/tinycolor.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/canvas_wrapper.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/numeric.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/sprintf.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/js/canvas_wrapper.js");
                if (outFile == null){
                    return true;
                }
                outFile = copyAsset("Csound6AndroidExamples/Gogins/LindenmayerCanvas.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
                return true;
            }
            case R.id.itemKoanI: {
                outFile = copyAsset("Csound6AndroidExamples/McCurdy/i.csd");
                if (outFile != null){
                    OnFileChosen(outFile);
                }
                return true;
            }
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

    private StringBuilder csoundMessageStringBuilder = new StringBuilder();
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
                // Send Csound messages to the WebView's console.log function.
                // This should happen when and only when a newline appears
                // in the message, but the newline itself should not be sent.
                for (int i = 0, n = message.length(); i < n; i++){
                    char c = message.charAt(i);
                    if (c == '\n') {
                        String line = csoundMessageStringBuilder.toString();
                        String code = String.format("console.log(\"%s\\n\");", line);
                        webView.evaluateJavascript(code, null);
                        csoundMessageStringBuilder.setLength(0);
                    } else {
                        csoundMessageStringBuilder.append(c);
                    }
                }
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
     * Opens the CSD and searches for a <CsHtml5> element.
     */
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    protected void parseWebLayout() {
        try {
            FileReader in = new FileReader(csound_file);
            StringBuilder contents = new StringBuilder();
            char[] buffer = new char[4096];
            int read = 0;
            do {
                contents.append(buffer, 0, read);
                read = in.read(buffer);
            } while (read >= 0);
            in.close();
            String csdText = contents.toString();
            int start = csdText.indexOf("<html");
            int end = csdText.indexOf("</html>") + 7;
            if (!(start == -1 || end == -1)) {
                html5Page = csdText.substring(start, end);
                if (html5Page.length() > 1) {
                    webView.setLayerType(View.LAYER_TYPE_NONE, null);
                    WebSettings settings = webView.getSettings();
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
                    File basePath = csound_file.getParentFile();
                    baseUrl = basePath.toURI().toURL();
                    webView.loadDataWithBaseURL(baseUrl.toString(),
                            html5Page, "text/html", "utf-8", null);
                }
            } else {
                webView.onPause();
                webView.pauseTimers();
            }
            View mainLayout = findViewById(R.id.mainLayout);
            mainLayout.invalidate();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void setScreenLayout(SharedPreferences sharedPreferences) {
        screenLayout = sharedPreferences.getString("screenLayout", "1");
        if (screenLayout.equals("1")) {
            channelsLayout.setVisibility(channelsLayout.GONE);
            webView.setVisibility(webView.GONE);
            messageScrollView.setVisibility(messageScrollView.VISIBLE);
        } else if (screenLayout.equals("2")) {
            channelsLayout.setVisibility(channelsLayout.GONE);
            webView.setVisibility(webView.VISIBLE);
            messageScrollView.setVisibility(messageScrollView.GONE);
        } else if (screenLayout.equals("3")) {
            channelsLayout.setVisibility(channelsLayout.GONE);
            webView.setVisibility(webView.VISIBLE);
            messageScrollView.setVisibility(messageScrollView.VISIBLE);
        } else if (screenLayout.equals("4")) {
            channelsLayout.setVisibility(channelsLayout.VISIBLE);
            webView.setVisibility(webView.GONE);
            messageScrollView.setVisibility(messageScrollView.GONE);
        } else if (screenLayout.equals("5")) {
            channelsLayout.setVisibility(channelsLayout.VISIBLE);
            webView.setVisibility(webView.GONE);
            messageScrollView.setVisibility(messageScrollView.VISIBLE);
        }
        View mainLayout = findViewById(R.id.mainLayout);
        mainLayout.invalidate();
    }

    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (key.equals(SettingsActivity.KEY_LIST_PREFERENCE)) {
            setScreenLayout(sharedPreferences);
        }
        if (key.equals(SettingsActivity.KEY_AUDIO_DRIVER_PREFERENCE)) {
            String driver = sharedPreferences.getString("audioDriver", "");
            if (driver.equalsIgnoreCase("2")) {
                use_oboe = true;
            } else {
                use_oboe = false;
            }
        }
    }

    private void OnFileChosen(File file) {
        Log.d("FILE CHOSEN", file.getAbsolutePath());
        csound_file = file;
        setTitle(csound_file.getName());
        parseWebLayout();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            csound_obj.stop();
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
        final SharedPreferences sharedPreferences = PreferenceManager
                .getDefaultSharedPreferences(this);
        sharedPreferences.registerOnSharedPreferenceChangeListener(this);
        OPCODE6DIR = sharedPreferences.getString("OPCODE6DIR", OPCODE6DIR);
        SSDIR = packageInfo.applicationInfo.dataDir + "/samples";
        SSDIR = sharedPreferences.getString("SSDIR", SSDIR);
        SFDIR = sharedPreferences.getString("SFDIR", SFDIR);
        SADIR = sharedPreferences.getString("SADIR", SADIR);
        INCDIR = sharedPreferences.getString("INCDIR", INCDIR);
        String driver = sharedPreferences.getString("audioDriver", "");
        if (driver.equalsIgnoreCase("2")) {
            use_oboe = true;
        } else {
            use_oboe = false;
        }
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
        copyRawwaves();
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
                                csound_file = new File(root, value);
                                writeTemplateFile();
                                Intent intent = new Intent(Intent.ACTION_VIEW);
                                Uri uri = Uri.parse("file://"
                                        + csound_file.getAbsolutePath());
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
                SimpleFileDialog fileOpenDialog = new SimpleFileDialog(
                        new ContextThemeWrapper(CsoundAppActivity.this, R.style.csoundAlertDialogStyle),
                        "FileOpen..",
                        new SimpleFileDialog.SimpleFileDialogListener() {
                            @Override
                            public void onChosenDir(String chosenDir) {
                                CsoundAppActivity.this.OnFileChosen(new File(chosenDir));
                            }
                        }
                );
                if (csound_file != null) {
                    fileOpenDialog.default_file_name = csound_file.getAbsolutePath();
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
                SimpleFileDialog fileOpenDialog = new SimpleFileDialog(
                        new ContextThemeWrapper(CsoundAppActivity.this, R.style.csoundAlertDialogStyle),
                        "FileSave..",
                        new SimpleFileDialog.SimpleFileDialogListener() {
                            @Override
                            public void onChosenDir(String chosenDir) {
                                int index = chosenDir.indexOf("//");
                                if (index >= 0) {
                                    chosenDir = chosenDir.substring(index + 1);
                                }
                                File newFile = new File(chosenDir);
                                if (csound_file.equals(newFile)) {
                                    Context context = getApplicationContext();
                                    CharSequence text = "'Save as' aborted; the new file is the same as the old file!";
                                    int duration = Toast.LENGTH_SHORT;
                                    Toast toast = Toast.makeText(context, text, duration);
                                    toast.show();
                                } else {
                                    try {
                                        FileInputStream in = new FileInputStream(csound_file);
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
                if (csound_file != null) {
                    fileOpenDialog.default_file_name = csound_file.getParent();
                } else {
                    fileOpenDialog.default_file_name = Environment.getExternalStorageDirectory().getAbsolutePath();
                }
                fileOpenDialog.chooseFile_or_Dir(fileOpenDialog.default_file_name);
            }
        });
        editButton = (Button) findViewById(R.id.editButton);
        editButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (csound_file != null) {
                    Intent intent = new Intent(Intent.ACTION_VIEW);
                    Uri uri = Uri.parse("file://" + csound_file.getAbsolutePath());
                    intent.setDataAndType(uri, "text/plain");
                    startActivity(intent);
                }
            }
        });
        channelsLayout = (LinearLayout) findViewById(R.id.channelsLayout);
        webView = (WebView) findViewById(R.id.htmlView);
        webView.setWebViewClient(new WebViewClient() {
            public void onReceivedError(WebView view, int errorCode,
                                        String description, String failingUrl) {
                Toast.makeText(CsoundAppActivity.this,
                        "WebView error! " + description, Toast.LENGTH_SHORT)
                        .show();
            }
        });
        messageTextView = (TextView) findViewById(R.id.messageTextView);
        messageScrollView = (ScrollView) findViewById(R.id.csoundMessages);
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
                if (csound_file == null) {
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
                    String driver = sharedPreferences.getString("audioDriver", "");
                    if (driver.equalsIgnoreCase("2")) {
                        use_oboe = true;
                    } else {
                        use_oboe = false;
                    }
                    if (use_oboe == true) {
                        csound_obj = null;
                        csound_oboe = new csnd6.CsoundOboe();
                        oboe_callback_wrapper = new CsoundCallbackWrapper(csound_oboe.GetCsound()) {
                            @Override
                            public void MessageCallback(int attr, String msg) {
                                Log.d("CsoundOboe:", msg);
                                postMessage(msg);
                            }
                        };
                        oboe_callback_wrapper.SetMessageCallback();
                        webView.addJavascriptInterface(csound_oboe, "csound");
                        webView.addJavascriptInterface(CsoundAppActivity.this, "csoundApp");
                    } else {
                        csound_oboe = null;
                        csound_obj = new JSCsoundObj();
                        csoundUI = new CsoundUI(csound_obj);
                        csound_obj.messagePoster = CsoundAppActivity.this;
                        csound_obj.setMessageLoggingEnabled(true);
                        webView.addJavascriptInterface(csound_obj, "csound");
                        webView.addJavascriptInterface(CsoundAppActivity.this,
                                "csoundApp");
                    }
                    // Csound will not be in scope of any JavaScript on the page
                    // until the page is reloaded. Also, we want to show any edits
                    // to the page.
                    parseWebLayout();
                    postMessageClear("Csound is starting...\n");
                    if (use_oboe == false) {
                        String framesPerBuffer = audioManager
                                .getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
                        postMessage("Android sample frames per audio buffer: "
                                + framesPerBuffer + "\n");
                        String framesPerSecond = audioManager
                                .getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
                        postMessage("Android sample frames per second: "
                                + framesPerSecond + "\n");
                    }
                    // Make sure this stuff really got packaged.
                    String samples[] = null;
                    try {
                        samples = getAssets().list("samples");
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    if (use_oboe == true) {
                        if (screenLayout.equals("4") || screenLayout.equals("5")) {
                            // Add slider handlers.
                            for (int i = 0; i < 5; i++) {
                                SeekBar seekBar = sliders.get(i);
                                final String channelName = "slider" + (i + 1);
                                seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                                    public void onStopTrackingTouch(SeekBar seekBar) {
                                        // TODO Auto-generated method stub
                                    }

                                    public void onStartTrackingTouch(SeekBar seekBar) {
                                        // TODO Auto-generated method stub
                                    }

                                    public void onProgressChanged(SeekBar seekBar, int progress,
                                                                  boolean fromUser) {
                                        if (fromUser) {
                                            double value = progress / (double) seekBar.getMax();
                                            csound_oboe.SetChannel(channelName, value);
                                        }
                                    }
                                });
                            }
                            // Add button handlers.
                            for (int i = 0; i < 5; i++) {
                                Button button = buttons.get(i);
                                final String channelName = "butt" + (i + 1);
                                button.setOnClickListener(new OnClickListener() {
                                    public void onClick(View v) {
                                        csound_oboe.SetChannel(channelName, 1.0);
                                    }
                                });
                            }
                            // Add trackpad handler.
                            pad.setOnTouchListener(new View.OnTouchListener() {
                                public boolean onTouch(View v, MotionEvent event) {
                                    int action = event.getAction() & MotionEvent.ACTION_MASK;
                                    double xpos = 0;
                                    double ypos = 0;
                                    boolean selected = false;
                                    switch (action) {
                                        case MotionEvent.ACTION_DOWN:
                                        case MotionEvent.ACTION_POINTER_DOWN:
                                            pad.setPressed(true);
                                            selected = true;
                                            break;
                                        case MotionEvent.ACTION_POINTER_UP:
                                        case MotionEvent.ACTION_UP:
                                            selected = false;
                                            pad.setPressed(false);
                                            break;
                                        case MotionEvent.ACTION_MOVE:
                                            break;
                                    }
                                    if (selected == true) {
                                        xpos = event.getX() / v.getWidth();
                                        ypos = 1. - (event.getY() / v.getHeight());
                                    }
                                    csound_oboe.SetChannel("trackpad.x", xpos);
                                    csound_oboe.SetChannel("trackpad.y", ypos);
                                    return true;
                                }
                            });
                            // Add motion handler.
                            SensorManager sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
                            List<Sensor> sensors = sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
                            if (sensors.size() > 0) {
                                Sensor sensor = sensors.get(0);
                                SensorEventListener motionListener = new SensorEventListener() {
                                    public void onAccuracyChanged(Sensor sensor, int accuracy) {
                                        // Not used.
                                    }
                                    public void onSensorChanged(SensorEvent event) {
                                        double accelerometerX = event.values[0];
                                        double accelerometerY = event.values[1];
                                        double accelerometerZ = event.values[2];
                                        csound_oboe.SetChannel("accelerometerX", accelerometerX);
                                        csound_oboe.SetChannel("accelerometerY", accelerometerY);
                                        csound_oboe.SetChannel("accelerometerZ", accelerometerZ);
                                    }
                                };
                                int microseconds = 1000000 / 20;
                                sensorManager.registerListener(motionListener, sensor, microseconds);
                            }
                        }
                    } else {
                        // We hook up the builtin widgets only if they are
                        // actually being used, otherwise they prevent names from
                        // being used for HTML channels.
                        if (screenLayout.equals("4") || screenLayout.equals("5")) {
                            String channelName;
                            for (int i = 0; i < 5; i++) {
                                channelName = "slider" + (i + 1);
                                csoundUI.addSlider(sliders.get(i), channelName, 0., 1.);
                                channelName = "butt" + (i + 1);
                                csoundUI.addButton(buttons.get(i), channelName, 1);
                            }
                            csoundUI.addButton(pad, "trackpad", 1);
                        }
                        CsoundMotion motion = new CsoundMotion(csound_obj);
                        motion.enableAccelerometer(CsoundAppActivity.this);
                        csound_obj.addListener(CsoundAppActivity.this);
                    }
                    // If the Csound file is a CSD, start Csound;
                    // otherwise, do not start Csound, and assume the
                    // file is HTML with JavaScript that will call
                    // csound_obj.perform() as in csound.node().
                    if (csound_file.toString().toLowerCase().endsWith(".csd")) {
                        if (use_oboe == true) {
                            int result = 0;
                            result = csound_oboe.compileCsd(csound_file.getAbsolutePath());
                            result = csound_oboe.start();
                            result = csound_oboe.perform();
                        } else {
                            csound_obj.startCsound(csound_file);
                        }
                    }
                    // Make sure this is still set after starting.
                    String getOPCODE6DIR = csnd6.csndJNI.csoundGetEnv(0,
                            "OPCODE6DIR");
                    if (use_oboe == true) {
                        csound_oboe.Message(
                                "OPCODE6DIR has been set to: " + getOPCODE6DIR
                                        + "\n");
                    } else {
                        csound_obj.Message(
                                "OPCODE6DIR has been set to: " + getOPCODE6DIR
                                        + "\n");
                    }
                } else {
                    if (use_oboe == true) {
                        csound_oboe.stop();
                    } else {
                        csound_obj.stop();
                    }
                    postMessage("Csound has been stopped.\n");
                }
            }
        });
        setScreenLayout(sharedPreferences);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent intent) {
        try {
            if (requestCode == R.id.newButton && intent != null) {
                csound_file = new File(intent.getData().getPath());
                setTitle(csound_file.getName());
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
        if (csound_obj != null) {
            Csound csound_ = csound_obj.getCsound();
            if (csound_ != null) {
                // This call is thread-safe.
                csound_.SetChannel(channelName, value);
            }
        }
    }

    @JavascriptInterface
    public double getControlChannel(String channelName) {
        double value = 0;
        if (csound_obj != null) {
            Csound csound_ = csound_obj.getCsound();
            if (csound_ != null) {
                // This call is thread-safe.
                value = csound_.GetChannel(channelName);
            }
        }
        return value;
    }
}
