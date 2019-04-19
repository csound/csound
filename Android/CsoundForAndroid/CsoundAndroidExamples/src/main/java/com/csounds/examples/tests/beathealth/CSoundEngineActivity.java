package com.csounds.examples.tests.beathealth;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.SystemClock;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.SearchView;
import android.widget.SearchView.OnQueryTextListener;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import com.csounds.CsoundObj;
import com.csounds.CsoundObjListener;
import com.csounds.examples.R;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import ap.api.IAudioEngine;
import ap.api.IAudioFinishedCallback;
import ap.data.Song;
import ap.helpers.MediaHelper;
import ap.helpers.SongAdapter;
import ap.players.CsoundAudioEngine;


public class CSoundEngineActivity extends Activity implements
    CsoundObjListener, IAudioFinishedCallback, View.OnClickListener {
//to be tested MP3 player with pitch control: CSoundEngineActivity
//TODO simplify

    private static final String TAG = "bhtest1";// PlayerActivityclass.getSimpleName();
    private static final boolean LOG = true;
    private static final String BEATRUN_SONGS = "beatrunsongs.csv";
    private int logline;
    final static int POLLING_DELAY_MS = 100;
    private Float tempo = 1.f;


    protected CsoundObj csoundObj = new CsoundObj(false, false);
    protected Handler handler = new Handler();
    private List<Song> songz = new ArrayList<Song>();
    private TextView pitchView;

    private void clog(String m) {
        if (LOG)
            Log.d(TAG, m);
    }

    private static final int REQUEST_PERMISSIONS = 0;

    private static final boolean RANDOM = false;

    private static final long initCurrentTimeMs = System.currentTimeMillis();


    public static long getAccurateTimeMs() {
        return (initCurrentTimeMs + SystemClock.elapsedRealtime() - initElapsedRealtimeMs); //timestamp
    }

    private static final long initElapsedRealtimeMs = SystemClock.elapsedRealtime();
    private IAudioEngine audioengine;
    private List<Song> songList = new ArrayList<>();

    private RecyclerView songView;
    private SongAdapter mAdapter;
    private RecyclerView.LayoutManager layoutManager;
    private TextView titlev;
    private TextView logview;
    private SearchView searchView;
    private View rootView;


    @Override
    protected void onResume() {
        super.onResume();
        rootView.requestFocus();
        searchView.setQuery("", true);
        //searchView.clearFocus();
        searchView.onActionViewCollapsed();
        titlev.setTextSize(15);
        setText(titlev, "touch a title to play it !");
    }

    @Override
    public void setStatus(String s) {
        setText(titlev, s);
    }

    @Override
    public IAudioEngine getAudioEngine() {
        return audioengine;
    }

    private Song currentSong;


    private Runnable pollCsoundForUpdates = new Runnable() {
        @Override
        public void run() {
            if (audioengine.isPlaying()) {
                handler.post(new Runnable() {
                    public void run() {
                        audioengine.update();
                    }
                });
                handler.postDelayed(this, POLLING_DELAY_MS);
            }
        }
    };


    private SeekBar seekBar;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        csoundObj.setMessageLoggingEnabled(true);
        setContentView(R.layout.activity_csound_engine);
        cancelPolling();
        rootView = findViewById(R.id.sound_eng_root_view);
        searchView = (SearchView) findViewById(R.id.search_view);
        searchView.setOnQueryTextListener(onQueryTextListener);
        seekBar = (SeekBar) findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(seekBarChangeListener);
        logview = (TextView) findViewById(R.id.logView);
        pitchView = (TextView) findViewById(R.id.pitchView);
        pitchView.setText("pitch=0.0 bpm=0");

        Button newb = (Button) findViewById(R.id.btn_next);
        Button playb = (Button) findViewById(R.id.btn_play);
        Button stopb = (Button) findViewById(R.id.btn_stop);
        Button pausb = (Button) findViewById(R.id.btn_pause);
        newb.setOnClickListener(this);
        playb.setOnClickListener(this);
        stopb.setOnClickListener(this);
        pausb.setOnClickListener(this);
        songView = (RecyclerView) findViewById(R.id.songlist);
        songView.setHasFixedSize(true);
        titlev = (TextView) findViewById(R.id.textViewTitleSong);
        permissionAndStartup();
    }




    void startPolling() {
        handler.postDelayed(pollCsoundForUpdates, POLLING_DELAY_MS);
    }

    void cancelPolling() {
        handler.removeCallbacks(pollCsoundForUpdates);
    }


    @Override
    public void selectSong(final Song song) {
        handler.post(new Runnable() {
            public void run() {
                if (MediaHelper.getInstance(CSoundEngineActivity.this).startPlaybackWithTempo(song, tempo, getAccurateTimeMs())) {
                    currentSong = song;
                    startPolling();
                }
            }
        });
    }

    @Override
    public void csoundObjStarted(CsoundObj csoundObj) {
        handler.post(new Runnable() {
            public void run() {
                audioengine.csoundStart();
                //startPlayback();
            }
        });
    }

    @Override
    public void csoundObjCompleted(CsoundObj csoundObj) {
        handler.post(new Runnable() {
            public void run() {
                //audioengine.csoundCompleted();
                audioengine.csoundStop();
            }
        });
    }

    @Override
    public void log(String m) {
        //alog(m, true);
    }

    private void initCSoundEngine() {
        audioengine = CsoundAudioEngine.getInstance(this);
        audioengine.setCSoundObj(csoundObj);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (LOG) clog("onDestroy");
        cancelPolling();
        stopSession();
        if ((audioengine != null)) {//&& audioengine.isInitialised()
            audioengine.stop();
            // audioengine.releaseResources();
        }
        csoundObj.stop();
        if (LOG) clog("onDestroy completed");
    }


    private void alog(final String msg, final boolean append) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (append) {
                        logline++;
                        if (logline == 5) {
                            logline = 0;
                            logview.setText("");
                        }
                        logview.append(msg + "\n");
                    } else
                        logview.setText(msg);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
    }


    private void setText(final TextView tv_, final String msg) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    tv_.setText(msg);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
    }


    @Override
    public void audioProgress(int playPos, int trackRemainingPos, int beatsRemainingPos, float playbackTempo) {
//alog("position " + Integer.toString(position), false);
    }


    @Override
    public void logCsound(String mesg) {
        alog(mesg, false);
    }

    private void stopSession() {
        if (LOG) clog("STOP");
        // if (audioengine.isInitialised())
        audioengine.stop();
    }

    public void startNext(boolean rand) {
        if (RANDOM || rand) {
            startNextRandomSong();
        } else {
           /* if (currentSong == null)
                startSong();
            else*/
            startNextSong(tempo);
        }
    }


    @Override
    public void audioFinished() {
        clog("song finished");
        nextSong();
    }


    public void nextSong() {
        clog("next song");
        handler.post(new Runnable() {
            public void run() {
                cancelPolling();
                audioengine.stop();
                startNext(false);
            }
        });
    }



    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_next:
                nextSong();
                break;
            case R.id.btn_play:
                handler.post(new Runnable() {
                    public void run() {
                        audioengine.play();
                    }
                });
                startPolling();
                break;
            case R.id.btn_stop:
                cancelPolling();
                handler.post(new Runnable() {
                    public void run() {
                        audioengine.stop();
                    }
                });
                break;
            case R.id.btn_pause:
                cancelPolling();
                handler.post(new Runnable() {
                    public void run() {
                        audioengine.pause();
                    }
                });
                break;
        }
    }

    @Override
    public void setTitle(final String t) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    titlev.setText(t);
                    titlev.setTextSize(25);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
    }

    private OnQueryTextListener onQueryTextListener = new OnQueryTextListener() {
        @Override
        public boolean onQueryTextSubmit(String query) {

            if (TextUtils.isEmpty(query)) {  //Show all data
                // mAdapter.animateTo(numbers);
                mAdapter.notifyDataSetChanged();
                return false;
            }
            mAdapter.getFilter().filter(query);
            return false;
        }

        @Override
        public boolean onQueryTextChange(String newText) {
            return false;
        }
    };


    @Override
    public void updateTempo(final float tempo, final float bpm) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                float progr = tempo - 1.0f;
                progr *= 100.f;
                seekBar.setProgress((int) progr);
                pitchView.setText(String.format("pitch=%.2f bpm=%.3f", tempo, bpm));
            }
        });
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
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        return f;
    }

    private void startup() {
        File dir = new File(Environment.getExternalStorageDirectory() + "/Download");
        File filesInDirectory[] = dir.listFiles();
        if (filesInDirectory != null) {
            for (int i = 0; i < filesInDirectory.length; i++) {
                File f = filesInDirectory[i];
                clog(f.getAbsolutePath());
                if (f.getAbsolutePath().endsWith("mp3"))
                    ;//  copyMp3(f);
                //clog(f.getAbsolutePath());
            }
        }
        init();
        nextSong();
    }

    private void init() {
        songList = MediaHelper.getInstance(this).readAllSongs();
        layoutManager = new LinearLayoutManager(getApplicationContext());
        songView.setLayoutManager(layoutManager);
        mAdapter = new SongAdapter(this, songList);//null;//new MyAdapter(myDataset);
        songView.setAdapter(mAdapter);
        initCSoundEngine();
        audioengine.setCompletionListener(this); // song finished callback

        String csooudfile = getResourceFileAsString(R.raw.csound_file);
        File csdFile = createTempFile(csooudfile);
        csoundObj.addListener(com.csounds.examples.tests.beathealth.CSoundEngineActivity.this);
        csoundObj.setMessageLoggingEnabled(true);
        csoundObj.startCsound(csdFile);
    }

    private void permissionAndStartup() {
        int permissionCheck = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE);
        if (permissionCheck != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, REQUEST_PERMISSIONS);
        } else {
            startup();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_PERMISSIONS:
                if ((grantResults.length > 0) && (grantResults[0] == PackageManager.PERMISSION_GRANTED)) {
                    startup();
                }
                break;
            default:
                break;
        }
    }

    private void startNextRandomSong() {
        currentSong = MediaHelper.getInstance(this).startRandomSong(tempo);
        startPolling();
    }

    private void startNextSong(float tempo) {
        currentSong = MediaHelper.getInstance(this).loadNextSong(currentSong);
        if (currentSong != null) {
            if (!MediaHelper.getInstance(this).startSongImmediatelyWithTempo(currentSong, tempo, false, getAccurateTimeMs())) {
                alog("unable to start song " + currentSong.title, true);
                setText(titlev, "unable to start song " + currentSong.title);
            } else
                startPolling();
        } else {
            alog("no song available", true);
            setText(titlev, "no song available");
        }
    }


    private OnSeekBarChangeListener seekBarChangeListener = new OnSeekBarChangeListener() {

        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            tempo = 1.0f + (progress / 100.0f);
            audioengine.setTempo(tempo);
            String v = Float.toString(tempo);
            alog(v, false);
        }

        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    };


}
