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

/**
 * Data class for beat annotation files
 * @author econway
 * Created: 13 Nov 2014 09:40:28
 *
 */
public class AnnotationFile {
	
	public final float bpm;
	public final float activation;
	public final int[] beats;
	
	/**
	 * @param bpm
	 * @param activation
	 * @param beats
	 */
	public AnnotationFile(float bpm, float activation, int[] beats) {
		super();
		this.bpm = bpm;
		this.activation = activation;
		this.beats = beats;
	}
		
}