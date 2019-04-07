
package ap.api;

import ap.data.Song;

public interface IAudioFinishedCallback {
	void audioFinished();
	void audioProgress(int playPos, int trackRemainingPos, int beatsRemainingPos, float playbackTempo);
	void log(String mesg);
    void selectSong(Song song);
	void logCsound(String mesg);
	void setStatus(String s);
	IAudioEngine getAudioEngine();
	void setTitle(String t);
	void updateTempo(float newplaybackTempo, float newPlaybackBPM);
}
