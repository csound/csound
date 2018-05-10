/*
    Bank.cpp:

    Copyright (C) 2006 Steven Yi

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "Bank.hpp"

static char* gm[] = {
  (char *)"Acoustic Grand",
  (char *)"Bright Acoustic",
  (char *)"Electric Grand",
  (char *)"Honky-Tonk",
  (char *)"Electric Piano 1",
  (char *)"Electric Piano 2",
  (char *)"Harpsichord",
  (char *)"Clavinet",

  (char *)"Celesta",
  (char *)"Glockenspiel",
  (char *)"Music Box",
  (char *)"Vibraphone",
  (char *)"Marimba",
  (char *)"Xylophone",
  (char *)"Tubular Bells",
  (char *)"Dulcimer",

  (char *)"Drawbar Organ",
  (char *)"Percussive Organ",
  (char *)"Rock Organ",
  (char *)"Church Organ",
  (char *)"Reed Organ",
  (char *)"Accoridan",
  (char *)"Harmonica",
  (char *)"Tango Accordian",

  (char *)"Nylon String Guitar",
  (char *)"Steel String Guitar",
  (char *)"Electric Jazz Guitar",
  (char *)"Electric Clean Guitar",
  (char *)"Electric Muted Guitar",
  (char *)"Overdriven Guitar",
  (char *)"Distortion Guitar",
  (char *)"Guitar Harmonics",

  (char *)"Acoustic Bass",
  (char *)"Electric Bass(finger)",
  (char *)"Electric Bass(pick)",
  (char *)"Fretless Bass",
  (char *)"Slap Bass 1",
  (char *)"Slap Bass 2",
  (char *)"Synth Bass 1",
  (char *)"Synth Bass 2",

  (char *)"Violin",
  (char *)"Viola",
  (char *)"Cello",
  (char *)"Contrabass",
  (char *)"Tremolo Strings",
  (char *)"Pizzicato Strings",
  (char *)"Orchestral Strings",
  (char *)"Timpani",

  (char *)"String Ensemble 1",
  (char *)"String Ensemble 2",
  (char *)"SynthStrings 1",
  (char *)"SynthStrings 2",
  (char *)"Choir Aahs",
  (char *)"Voice Oohs",
  (char *)"Synth Voice",
  (char *)"Orchestra Hit",

  (char *)"Trumpet",
  (char *)"Trombone",
  (char *)"Tuba",
  (char *)"Muted Trumpet",
  (char *)"French Horn",
  (char *)"Brass Section",
  (char *)"SynthBrass 1",
  (char *)"SynthBrass 2",

  (char *)"Soprano Sax",
  (char *)"Alto Sax",
  (char *)"Tenor Sax",
  (char *)"Baritone Sax",
  (char *)"Oboe",
  (char *)"English Horn",
  (char *)"Bassoon",
  (char *)"Clarinet",

  (char *)"Piccolo",
  (char *)"Flute",
  (char *)"Recorder",
  (char *)"Pan Flute",
  (char *)"Blown Bottle",
  (char *)"Skakuhachi",
  (char *)"Whistle",
  (char *)"Ocarina",

  (char *)"Lead 1 (square)",
  (char *)"Lead 2 (sawtooth)",
  (char *)"Lead 3 (calliope)",
  (char *)"Lead 4 (chiff)",
  (char *)"Lead 5 (charang)",
  (char *)"Lead 6 (voice)",
  (char *)"Lead 7 (fifths)",
  (char *)"Lead 8 (bass+lead)",

  (char *)"Pad 1 (new age)",
  (char *)"Pad 2 (warm)",
  (char *)"Pad 3 (polysynth)",
  (char *)"Pad 4 (choir)",
  (char *)"Pad 5 (bowed)",
  (char *)"Pad 6 (metallic)",
  (char *)"Pad 7 (halo)",
  (char *)"Pad 8 (sweep)",

  (char *)"FX 1 (rain)",
  (char *)"FX 2 (soundtrack)",
  (char *)"FX 3 (crystal)",
  (char *)"FX 4 (atmosphere)",
  (char *)"FX 5 (brightness)",
  (char *)"FX 6 (goblins)",
  (char *)"FX 7 (echoes)",
  (char *)"FX 8 (sci-fi)",

  (char *)"Sitar",
  (char *)"Banjo",
  (char *)"Shamisen",
  (char *)"Koto",
  (char *)"Kalimba",
  (char *)"Bagpipe",
  (char *)"Fiddle",
  (char *)"Shanai",

  (char *)"Tinkle Bell",
  (char *)"Agogo",
  (char *)"Steel Drums",
  (char *)"Woodblock",
  (char *)"Taiko Drum",
  (char *)"Melodic Tom",
  (char *)"Synth Drum",
  (char *)"Reverse Cymbal",

  (char *)"Guitar Fret Noise",
  (char *)"Breath Noise",
  (char *)"Seashore",
  (char *)"Bird Tweet",
  (char *)"Telephone Ring",
  (char *)"Helicopter",
  (char *)"Applause",
  (char *)"Gunshot"
};

Bank::Bank(CSOUND *csound, char* bankName) : bankNum(0) {
        this->name = bankName;
        IGN(csound);
        currentProgram = 0;
        previousProgram = -1;
}

Bank::~Bank() {
        while( !programs.empty() ) {
        programs.erase( programs.begin() );
        }
}

void Bank::initializeGM() {
        for(int i = 0; i < 128; i++) {
                Program temp(i, gm[i]);
                this->programs.push_back(temp);
        }
}
