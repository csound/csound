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
package ap.data;

import android.net.Uri;
import android.provider.MediaStore;

/**
 * @author rudi
 *
 */
public class Constants {
    
    /** 
     * Phase value is unknown or cannot be calculated
     */
    public final static float PHASE_UNKNOWN = -999999;
    public final static long TIME_UNKNOWN = -999999;
    public final static int INTERVAL_UNKNOWN = -999999; 
    public final static int POS_UNKNOWN__ = -999999;


    public static final float UNKNOWN_BPM_ = 0.0f;
    /** 
     * Tempo modifier value is unknown or cannot be calculated
     */
    public final static float TEMPO_MOD_UNKNOWN_ = -1;
    
    /**
     * Tempo modifier indicates no tempo modification over base tempo
     */
    public final static float TEMPO_MOD_BASE = 1.f;
    
    /**
     * Rate (SPM, BPM, Freq, etc.) unknown or cannot be calculated
     */
    public final static float RATE_UNKNOWN_ = -1;
    
    /**
     * Song id when song could not be identified (or no song was present)
     */
    public final static int SONG_UNKNOWN = -1;


    public static final int AUDIO_LATENCY = 50; //FP190227
    public  static final int CSOUND_LATENCY = 60; // this just depends on csound and is time to load mp3 file
    // int audioLoadDelayMs = 60; // this just depends on csound and is time to load mp3 file
    public  static final  int beatAnnotationMp3DelayMs = 25; //FP190227


    public final static String TRACK_ID = MediaStore.Audio.Media._ID;
    public final static String TRACK_NAME = MediaStore.Audio.Media.TITLE;
    public final static String ARTIST = MediaStore.Audio.Media.ARTIST;
    public final static String DURATION = MediaStore.Audio.Media.DURATION;
    public final static String PATH = MediaStore.Audio.Media.DATA;
    public final static Uri TRACK_URI = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;




}
