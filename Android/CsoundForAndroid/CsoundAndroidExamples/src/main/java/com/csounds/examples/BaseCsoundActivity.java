package com.csounds.examples;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.SeekBar;
import android.Manifest;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.csounds.CsoundObj;

@SuppressLint("NewApi")
public class BaseCsoundActivity extends Activity {

    private static final int PERMISSION_REQUEST_RECORD_AUDIO = 1;

    protected CsoundObj csoundObj = new CsoundObj(false, true);
    protected Handler handler = new Handler();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Request microphone permission
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.RECORD_AUDIO},
                PERMISSION_REQUEST_RECORD_AUDIO);
        } else {
            initializeCsound();
        }
    }

    private void initializeCsound() {
        csoundObj.setMessageLoggingEnabled(true);
        Log.d("CsoundObj", "Csound initialized");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String[] permissions,
                                           int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_RECORD_AUDIO) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                initializeCsound();
            } else {
                Log.e("Permission", "RECORD_AUDIO permission denied");
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        csoundObj.stop();
    }

    public void setSeekBarValue(SeekBar seekBar, double min, double max, double value) {
        double range = max - min;
        double percent = (value - min) / range;
        seekBar.setProgress((int) (percent * seekBar.getMax()));
    }

    protected String getResourceFileAsString(int resId) {
        StringBuilder str = new StringBuilder();
        InputStream is = getResources().openRawResource(resId);
        BufferedReader r = new BufferedReader(new InputStreamReader(is));
        String line;

        try {
            while ((line = r.readLine()) != null) {
                str.append(line).append("\n");
            }
        } catch (IOException ios) {
            ios.printStackTrace();
        }
        return str.toString();
    }

    protected File createTempFile(String csd) {
        File f = null;
        try {
            f = File.createTempFile("temp", ".csd", this.getCacheDir());
            FileOutputStream fos = new FileOutputStream(f);
            fos.write(csd.getBytes());
            fos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return f;
    }
}
