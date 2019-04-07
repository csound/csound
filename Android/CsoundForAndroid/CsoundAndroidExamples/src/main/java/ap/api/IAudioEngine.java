
package ap.api;


import com.csounds.CsoundObj;

import ap.data.Song;

public interface IAudioEngine<T> {
	//void start();
    boolean isPlaying();
    boolean isStopped();
    boolean isPaused();
	void stop();
	void pause();
    void play();
	void setCompletionListener(T listener);
    void enqueue(Song song, int startPosMs);
    boolean startPlayback(Song song, float tempo,int startPosMs, long startTimeMs,boolean compensateAudioLatency);
      void setCSoundObj(CsoundObj csoundObj);
    void csoundStart();
    void setTempo(final Float f);
    void csoundStop();
    void update();
}
