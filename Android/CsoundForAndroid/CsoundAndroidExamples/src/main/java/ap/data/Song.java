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

import java.util.Locale;

/**
 * Song data class
 * Contains everything BeatHealth needs to know about an individual song
 * 
 * @author econwayM
 * Created: May 27, 2014 3:25:38 PM
 */
public class Song implements Comparable<Song> {
	public int id;
	public String artist, title, path;
	public float bpm_;
	public  int[] beatsMs;
    public long durationMs;
    public  long usableMs;

    public Song() {

    }


    public int getId() {
		return id;
	}

	public String getArtistz() {
		return artist;
	}

	public String getTitlez() {
		return title;
	}

	public String getPathz() {
		return path;
	}

	public String getBpmz() {
		return Float.toString(bpm_);
	}

	public String getBeatsMsz() {
		return beatsMs.toString();
	}

	public String getDurationMsz() {
		return Long.toString(durationMs);
	}




	public  int getFirstBeatMs() {  //used in this test
		try {
			if ( (beatsMs != null) && (beatsMs.length > 0))
				return beatsMs[0];
			else {
				return 1;
			}
		} catch (Exception e) {
			return 1; //FP180705
		}
	}

	public String toCsv(){
        String result = "";
        result+=id+";";
        result+=artist+";";
        result+=title+";";
        result+=path+";";
        result+= (beatsMs ==null?"0;": beatsMs.length+";") ;
		result+=durationMs+";";
		result+=usableMs+";";
		return result;
    }

	public void usableFromCsv(String line){
		String[] ws=line.split(";");
		if (ws.length==7) {
            id = Integer.parseInt(ws[0]);
            usableMs = Long.parseLong(ws[6]);
        }
	}





	/**
	 * Create a new Song object
	 * @param artist
	 * @param title
	 * @param bpm
	 * @param path
	 */
	public Song(String artist, String title, float bpm, long duration, String path, int[] beatAnnotations, int id) {
		super();
		this.artist = artist;
		this.title = title;
		this.bpm_ = bpm;
		this.durationMs = duration;
		this.path = path;
		this.beatsMs = beatAnnotations;
		this.id = id;
		this.usableMs = durationMs;
	}

    public Song(Song other) {
        super();
        this.artist = other.artist;
        this.title = other.title;
        this.bpm_ = other.bpm_;
        this.durationMs = other.durationMs;
        this.path = other.path;
        this.beatsMs = new int[other.beatsMs.length];
        for (int i = 0; i<other.beatsMs.length; i++)
            this.beatsMs[i]=other.beatsMs[i];
        this.id = other.id;
        this.usableMs = durationMs;
    }


	@Override
	public String toString() {
		return getFullString();
	}
	
	
	public String getMediumString() {
        int nBeats = (beatsMs != null) ? beatsMs.length : -1;
        
        return String.format(Locale.US,
                "Song {id=%d, bpm=%.2f, nBeats=%d, durMs=%d, title=%s}", 
                id, bpm_, nBeats, durationMs, title);
	}
	
	public String getFullString() {
	    int nBeats = (beatsMs != null) ? beatsMs.length : -1;
	    
        return String.format(Locale.US,
                "Song {id=%d, bpm=%.2f, nBeats=%d, durMs=%d, title=%s, artist=%s, path=%s}",
                id, bpm_, nBeats, durationMs, title, artist, path);
	}
	
	

	/* (non-Javadoc)
     * @see java.lang.Object#equals(java.lang.Object)
     */
    @Override
    public boolean equals(Object o) {
        if (o == null)
            return false;
        Song other = (Song) o;
        return id == other.id && path.equals(other.path); 
    }

    /**
	 * @see java.lang.Comparable#compareTo(java.lang.Object)
	 */
	@Override
	public int compareTo(Song another) {
		return title.compareTo(another.title);
		/*
		if (bpm_ == another.bpm_)
			return 0;
		if (bpm_ > another.bpm_)
			return 1;
		if (bpm_ < another.bpm_)
			return -1;
		return 0;*/
	}
}
