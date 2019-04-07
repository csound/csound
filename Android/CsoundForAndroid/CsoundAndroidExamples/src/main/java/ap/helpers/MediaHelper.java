package ap.helpers;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.Log;
import java.util.ArrayList;
import java.util.List;
import ap.api.IAudioEngine;
import ap.api.IAudioFinishedCallback;
import ap.data.Song;


/**
 * @author fpfister
 */
public class MediaHelper {
    private static final String TAG = "bhtest5";
    private static final boolean LOG = true;


    private Context context;
    private IAudioEngine audioEngine;

    private void clog(String m) {
        if (LOG)
            Log.d(TAG, m);
    }


    private int[] toIntArray(List<Integer> list) {
        int[] ret = new int[list.size()];
        int i = 0;
        for (Integer e : list)
            ret[i++] = e.intValue();
        return ret;
    }

    static MediaHelper instance;


    private MediaHelper() {

    }

    public static MediaHelper getInstance(Context c) {
        if (instance == null)
            instance = new MediaHelper();
        instance.context = c;
        instance.audioEngine = ((IAudioFinishedCallback) c).getAudioEngine();
        return instance;
    }

    private Song songAtCursor(Cursor cursor, boolean readBeatsEnabled) {
        final String path = cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA));

        int dummybpm = 100;
        int[] dummybeats={1,2,3,4,5,6,7,8,9};

        Song song = new Song(cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.ARTIST)),
                cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.TITLE)),
                dummybpm,// afile.bpm,
                cursor.getLong(cursor.getColumnIndex(MediaStore.Audio.Media.DURATION)),
                path,
                dummybeats,
                cursor.getInt(cursor.getColumnIndex(MediaStore.Audio.Media._ID)));
        clog(song.getFullString());
        return song;
    }


    public Song loadSong(String artist, String title) {
        Song result = null;
        ContentResolver contentResolver = context.getContentResolver();
        Uri uri = android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        String[] projection = {MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.ARTIST, MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.DATA};
        String selection = MediaStore.Audio.Media.IS_MUSIC + " != 0";
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, projection, selection, null, null);
        } catch (Exception e) {

        }
        if (cursor == null) {
            clog("song not exists: " + artist + " " + title);
        } else if (!cursor.moveToFirst()) {
            clog("no song available");
        } else {
            do {
                result = songAtCursor(cursor, false);
                if (result != null && result.title.equals(title) && result.artist.equals(artist))
                    break;
            } while (cursor.moveToNext());
            clog("selected " + result.getFullString());
        }
        if (cursor != null)
            cursor.close();
        return result;
    }


    public List<Song> readAllSongs() {
        List<Song> results = new ArrayList<>();
        Uri uri = android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        String[] projection = {MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.ARTIST, MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.DATA};
        String selection = MediaStore.Audio.Media.IS_MUSIC + " != 0";
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, projection, selection, null, null);
        } catch (Exception e) {
            clog("cursor song error");
        }
        if (cursor == null) {
            clog("no song available");
        } else if (!cursor.moveToFirst()) {
            clog("no song available");
        } else {
            do {
                Song s_ = songAtCursor(cursor, false);
                if (s_ != null) {
                    clog(s_.getFullString());
                    results.add(s_);
                } else
                    clog("no song available" + cursor.getString(cursor.getColumnIndex(MediaStore.Audio.Media.DATA)));
            } while (cursor.moveToNext());
        }
        if (cursor != null)
            cursor.close();
        return results;
    }

    public  boolean startPlaybackWithTempo(Song song, float tempo,long timeNowMs) {
        if (song != null) {
            if (!startSongImmediatelyWithTempo(song, tempo, false,timeNowMs)) {
                log("unable to start song " + song.title);
                setStatus("unable to start song " + song.title);
            } else
                return true;
        } else {
            log("no song available");
            setStatus("no song available");
        }
        return false;
    }

    private void setStatus(String s) {
        ((IAudioFinishedCallback) context).setStatus(s);
    }

    private void setTitle(String t) {
        ((IAudioFinishedCallback) context).setTitle(t);
    }

    private void log(String s) {
        ((IAudioFinishedCallback) context).log(s);
    }

    private boolean startSongImmediately(Song song, float bpm, boolean startAtFirstBeat,long timeNowMs) {
        clog(String.format(
                "startSongImmediately: bpm=%.1f, startFirstBeat=%b, song=%s",
                bpm, startAtFirstBeat, song));
        // we want to start next song so that it's first beat is in phase with
        // the next predicted beat of the current song
        //long timeNowMs = getAccurateTimeMs();
        // set start position if requested and enough info available
        int startPosMs = startAtFirstBeat ? song.getFirstBeatMs() : 0;
        float newTempoMod = 1.f;
        // set tempo mod if enough info available
        /* //FP190407
        if ((bpm > 0) && (song.bpm_ > 0)) {
            newTempoMod = bpm / song.bpm_;
        }*/
        return startPlayback(song, newTempoMod, startPosMs,
                timeNowMs + 60, false);
    }


    public boolean startSongImmediatelyWithTempo(Song song, float tempo, boolean startAtFirstBeat,long timeNowMs) {
        // we want to start next song so that it's first beat is in phase with
        // the next predicted beat of the current song
       // long timeNowMs = getAccurateTimeMs();
        // set start position if requested and enough info available
        int startPosMs = startAtFirstBeat ? song.getFirstBeatMs() : 0;
        return startPlayback(song, tempo, startPosMs,
                timeNowMs + 60, false);
    }


    private boolean startPlayback(Song newSong, float tempo, int startPosMs, long startTimeMs, boolean compensateAudioLatency) {
        Song song = loadSong(newSong.artist, newSong.title);
        if (song != null) {
            //long timeNowMs = initCurrentTimeMs + SystemClock.elapsedRealtime();
            if (!audioEngine.startPlayback(song, tempo, startPosMs, startTimeMs, compensateAudioLatency)) {//14108);
                log("load error 3");
                return false;
            } else {
                setTitle(song.title);
                // titlev.setTextSize(25);
                return true;
            }
        } else
            log("song not available");
        return false;
    }

    public Song startRandomSong(float tempo) {
       // sdf
        Song song = loadRandomSong();
        if (song != null) {
            if (!audioEngine.startPlayback(song, tempo, 0, 0L, false)) {
                log("load error");
                setTitle("load error 2");
                ;
            } else {
                setTitle(song.title);
                return song;
            }
        } else {
            log("song not available");
            setTitle("song not available");
        }
        return null;
    }

    public Song loadNextSong(Song song) {
       // dfg  MediaHelper.getInstance(this).
        Song result = null;
        Uri uri = android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        String[] projection = {MediaStore.Audio.Media._ID,  MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.ARTIST, MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.DATA};
        String selection = MediaStore.Audio.Media.IS_MUSIC + " != 0";
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, projection, selection, null, null);
        } catch (Exception e) {
            clog("cursor song error");
        }
        if (cursor == null) {
            clog("no song available");
        } else if (!cursor.moveToFirst()) {
            clog("no song available");
        } else {
            Song prior = null, current = null;
            do {
                current = songAtCursor(cursor, false);
                if (current != null) {
                    if (song == null || (prior != null && prior.path.equals(song.path))) {
                        result = current;
                        break;
                    }
                    prior = current;
                }
            } while (cursor.moveToNext());
        }

        if (cursor != null) {
            if (result != null)
               result = songAtCursor(cursor, true); //re-read the metadata for the found song
            cursor.close();
        }
        return result;
    }


    public Song loadRandomSong() {
        Song result = null;
        ContentResolver contentResolver = context.getContentResolver();
        Uri uri = android.provider.MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        String[] projection = {MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
            MediaStore.Audio.Media.ARTIST, MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.DATA};
        String selection = MediaStore.Audio.Media.IS_MUSIC + " != 0";
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, projection, selection, null, null);
        } catch (Exception e) {

        }
        if (cursor == null) {
            clog("no song available");
        } else if (!cursor.moveToFirst()) {
            clog("no song available");
        } else {
            int ndx = (int) (cursor.getCount() * Math.random());
            cursor.move(ndx);
            result = songAtCursor(cursor, false);
            if (result != null)
                clog("selected " + result.getFullString());
        }
        if (cursor != null) {
            if (result != null)
                result = songAtCursor(cursor, true); //re-read the metadata for the found song
            cursor.close();
        }
        return result;
    }




}
