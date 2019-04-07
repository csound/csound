/*
 * This file is part of BeatHealth_2020_SW.
 *
 * Copyright (C) 2018 The National University of Ireland Maynooth, Maynooth University
 *
 * The use of the code within this file and all code within files that
 * make up the software that is BeatHealth_2020_SW is permitted for
 * non-commercial purposes only.  The full terms and conditions that
 * apply to the code within this file are detailed within the
 * LICENCE.txt file and at
 * http://www.eeng.nuim.ie/research/BeatHealth_2020_SW/LICENCE.txt
 * unless explicitly stated.
 *
 * By downloading this file you agree to comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then
 * please email commercialisation@mu.ie
 */
package ap.players;


import android.content.Context;

import android.os.SystemClock;
import android.util.Log;

import com.csounds.CsoundObj;

import ap.api.IAudioEngine;
import ap.api.IAudioFinishedCallback;

import ap.data.Song;
import csnd6.Csound;


/**
 * <h1>CsoundAudioEngine</h1>
 * <p>Implementation of the IAudioEngine Interface using Csound 6.02</p>
 *
 * @author econway
 * Created: Feb 21, 2014 12:15:48 PM
 */
public class CsoundAudioEngine implements IAudioEngine<IAudioFinishedCallback> {
    private static final String TAG_ = "bhtestb";

    private static final int MAX_POS = -1;


    private static final boolean LOG = true;
    private static final String CS_TRACK = "track";
    private static final String CS_OFFSET = "offset";
    private static final String CS_LOAD = "load";
    private static final String CS_LOADED = "loaded";
    private static final String CS_LIMIT = "limit";
    private static final String CS_FADE_IN = "fadein";
    private static final String CS_FADE_OUT = "fadeout";
    private static final String CS_START = "start";
    private static final String CS_PLAY = "play";
    private static final String CS_END = "end";
    private static final String CS_TEMPO1 = "tempo1.";
    private static final String CS_TIME1 = "time1.";
    private static final String CS_TIME_TOEND1 = "timeToEnd1.";
    private IAudioFinishedCallback client;
    private int audioMixPercent = 50;
    private int audioFadeInMs = 0;
    private int audioFadeOutMs = 250;



    private static CsoundAudioEngine instance;
    private boolean songLoaded;

    public static CsoundAudioEngine getInstance(Context context) {
        if (instance == null)
            instance = new CsoundAudioEngine(context);
        return instance;
    }

    private void clog(String m) {
        if (LOG)
            Log.d(TAG_, m);
    }

    private Context context = null;
    float playbackBPM = -1;
    float playbackTempoMod = 1.f; // initially no tempo change
    private TrackInfo tracks[]; // see ctor
    private TrackInfo currentTrack;
    private TrackInfo queueTrack;

    private Csound csound;

    enum PlayState {STOPPED, PAUSED, PLAYING}

    private PlayState playState = PlayState.STOPPED;


    private IAudioFinishedCallback onCompletionListener;


    private int getCurrentPosition() {
        return (int) csound.GetChannel(CS_TIME1 + currentTrack.trackNumber);
    }

    private int getTrackRemainingPos() {
        return (int) csound.GetChannel(CS_TIME_TOEND1 + currentTrack.trackNumber);
    }


    @Override
    public void update() {
        if (currentTrack.finished) {
            clog("current track is already finished");
        } else {
            // update current playback position in [ms]
            int currentPlayPos = getCurrentPosition();
            int trackRemainingPos = getTrackRemainingPos();
            boolean endz = false;//usableLength > 0 && currentPlayPos >= usableLength / 10; //simulation
            if (endz || currentPlayPos < 0 || (MAX_POS != -1 && currentPlayPos > MAX_POS)) {
                playState = PlayState.STOPPED;
                currentTrack.finishSong();
                clog(String.format("audio finished: %s", currentTrack.song));
                onCompletionListener.audioFinished();
            } else {
                int beatsRemainingPos = 0;//TOTO//getBeatsRemainingPos(currentPlayPos);
                float playbackTempo = getAndUpdateCurrentTrackPlaybackTempo();
                String alog = String.format("audio remaining: currentPosMs=%d, trackRemainingPosMs=%d,  tempoMod=%.3f",
                        currentPlayPos, trackRemainingPos, playbackTempo);
              //  String alog=String.format("audio remaining: currentPosMs=%d,  beatsRemainingPosMs=%d, tempoMod=%.3f", currentPlayPos, trackRemainingPos,  getPlaybackTempo());
                onCompletionListener.audioProgress(currentPlayPos, trackRemainingPos, beatsRemainingPos, playbackTempo);
                onCompletionListener.logCsound(alog);

            }
        }
    }



    class TrackInfo {
        public Song song;
        public int trackNumber;
        public int prevBeatIndex;
        public boolean finished;

        public TrackInfo(int trackNumber) {
            song = null;
            this.trackNumber = trackNumber;
            prevBeatIndex = 0;
            finished = false;
        }

        public void setSong(Song song) {
            this.song = song;
            prevBeatIndex = 0;
            finished = false;
        }

        public void finishSong() {
            finished = true;
        }

        public boolean isSongUsable() {
            return (song != null) && (!finished);
        }
    }


    @Override
    public void setCSoundObj(CsoundObj csoundObj) {
        // todo read preferences
        audioMixPercent = 50;
        audioFadeInMs = 0;
        audioFadeOutMs = 250;
        csound = csoundObj.getCsound();
        csound.SetChannel(CS_LIMIT, ((float) audioMixPercent) / 100);
        csound.SetChannel(CS_FADE_IN, ((float) audioFadeInMs) / 1000);
        csound.SetChannel(CS_FADE_OUT, ((float) audioFadeOutMs) / 1000);
    }

    @Override
    public void csoundStart() {

    }

    @Override
    public void csoundStop() {
        stop();
    }


    private CsoundAudioEngine() {
    }

    private CsoundAudioEngine(Context context) {
        this.context = context;
        if (!(context instanceof IAudioFinishedCallback))
            throw new RuntimeException("MediaplayerAudioEngine client  must be an IAudioFinishedCallback");
        this.client = (IAudioFinishedCallback) context;
        tracks = new TrackInfo[2];
        tracks[0] = new TrackInfo(1);
        tracks[1] = new TrackInfo(2);
        currentTrack = tracks[0];
        queueTrack = tracks[1];
    }

    public Song getCurrentSong() {
        if (currentTrack.isSongUsable())
            return currentTrack.song;
        else
            return null;
    }

    private void swapTracks() {
        TrackInfo tmpTrack = currentTrack;
        currentTrack = queueTrack;
        queueTrack = tmpTrack;
    }


    @Override
    public void enqueue(Song song, int offsetMs) {
        doEnqueue(song, offsetMs);
    }


    private void doEnqueue(Song song, int offsetMs) {
        queueTrack.setSong(song);
        csound.SetChannel(CS_TRACK + queueTrack.trackNumber, queueTrack.song.path);
        csound.SetChannel(CS_OFFSET + queueTrack.trackNumber, offsetMs);
        csound.SetChannel(CS_LOAD + queueTrack.trackNumber, 1); // request load
        songLoaded = blockUntilSongLoaded();
    }

    public void crossfadeToQueuedTrack(int offsetMs_nu) {
        if (playState == PlayState.STOPPED) {
            startFromStopped();//offsetMs);
        } else {
            swapTracks();
            // fade out old track
            csound.SetChannel(CS_END + queueTrack.trackNumber, 1);
            csound.SetChannel(CS_START + currentTrack.trackNumber, 1); // request start
            if (isPaused()) {
                csound.SetChannel(CS_PLAY, 1); // set track playing
                playState = PlayState.PLAYING;
            }
        }
    }

    private boolean blockUntilSongLoaded() {
        int cnt = 0;
        int i = 0;
        boolean loaded = false;
        while (i++ < 1000) {
            if (csound.GetChannel(CS_LOADED + queueTrack.trackNumber) == 1) {
                loaded = true;
                break;
            }
            SystemClock.sleep(10);
        }
        if (LOG) {
            if (loaded)
                clog("en_queue: Song loaded in track " + queueTrack.trackNumber);
            else
                clog("en_queue: unable to load Song");
        }
        return loaded;
    }


    @Override
    public void pause() {
        csound.SetChannel(CS_PLAY, 2); // pause it
        playState = PlayState.PAUSED;
    }

    @Override
    public void play() {
        if (isStopped()) {
            startFromStopped();
        } else if (isPaused()) {
            startFromPause();
        } else
            clog("unvalid command play");
    }

    @Override
    public boolean isPaused() {
        return (playState == PlayState.PAUSED);
    }

    @Override
    public boolean isStopped() {
        return (playState == PlayState.STOPPED);
    }

    @Override
    public boolean isPlaying() {
        return (playState == PlayState.PLAYING);
    }


    @Override
    public void stop() {
        csound.SetChannel(CS_END + currentTrack.trackNumber, 1);
        playState = PlayState.STOPPED;
    }

    @Override
    public void setCompletionListener(final IAudioFinishedCallback listener) {
        this.onCompletionListener = listener;
    }

    public float getAndUpdateCurrentTrackPlaybackTempo() {
        if ((playbackTempoMod == -1)
                && currentTrack.isSongUsable()) {
          playbackTempoMod = csound.GetChannel(CS_TEMPO1 + currentTrack.trackNumber);
            if (currentTrack.song.bpm_ != -1)
                playbackBPM = playbackTempoMod * currentTrack.song.bpm_;
            else
                playbackBPM = -1;
            onCompletionListener.updateTempo(playbackTempoMod,playbackBPM);//have changed so have been re_read from csound
        }
        return playbackTempoMod;
    }



    public void setAndPrepareUpdateCurrentTrackPlaybackTempo(float tempo) {
        clog(String.format("setPlaybackTempo: tempo %.4f", tempo));
        if (tempo <= 0)
            throw new IllegalArgumentException("tempo must be a positive float, was: " + tempo);
        csound.SetChannel("tempo1."+currentTrack.trackNumber, tempo);
        // FIXME: what about correctly setting tempo of other track if it is still playing
        clog(String.format("setChannel: tempo1.%d %.4f", currentTrack.trackNumber, tempo));
        // update the playbackTempoMod the next time that getPlaybackTempo is called
        playbackTempoMod = -1;
        playbackBPM = -1;
    }


    @Override
    public boolean startPlayback(final Song song, final float tempo, final int startPosMs, final long startTimeMs_nu, boolean compensateAudioLatency_nu) {
        enqueue(song, startPosMs);
        if (songLoaded) {
            crossfadeToQueuedTrack(startPosMs);
            setAndPrepareUpdateCurrentTrackPlaybackTempo(tempo);
             }
        return songLoaded;
    }


    @Override
    public void setTempo(final Float f) {
        setAndPrepareUpdateCurrentTrackPlaybackTempo(f);
    }

    private void startFromStopped() { //FP180305 the bug
        swapTracks();
        csound.SetChannel(CS_START + currentTrack.trackNumber, 1);
        playState = PlayState.PLAYING;
    }

    private void startFromPause() {
        csound.SetChannel(CS_PLAY, 1); // set track playing
        playState = PlayState.PLAYING;
    }

    private void alog(String s) {
        client.log(s);
    }

}
