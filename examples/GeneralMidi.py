#!/usr/bin/python

# Copyright (c) 2003 by Michael Gogins
# Shows how to render an ordinary MIDI sequence
# using a SoundFont

import CsoundVST

# Load the MIDI sequence into the score.
model = CsoundVST.MusicModel()
model.setCppSound(csound)
score = model.getScore()
score.load("c:/WINDOWS/Media/town.mid")
print("Score length = ", len(score))
csound.setOrchestra('''
sr = 44100
kr = 441
ksmps = 100
nchnls = 2
0dbfs = 1

gSfont                  =                       "c:/MusicProgrammingBook/examples/63.3mg The Sound Site Album Bank V1.0.SF2"

giFluid                 fluidEngine
giSFont                 fluidLoad               gSfont, giFluid, 1

ichn                    =                       1
lp01:
ibank                   =                       (ichn != 10 ? 0 : 128)
ipreset                 =                       (ichn != 10 ? (ichn - 1) * 8 : 0)
                        fluidProgramSelect      giFluid, ichn - 1, giSFont, ibank, ipreset
                        loop_le                 ichn, 1, 16, lp01

                        event_i                 "i", 90, 0, -1

instr 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 ; FluidSynth General MIDI
; INITIALIZATION
; Channel, bank, and program determine the preset, that is, the actual sound.
ichannel                =                       p1
ikey                    =                       p4
ivelocity               =                       p5
ijunk11                 =                       p11
; AUDIO
                        print                   ichannel, ikey, ivelocity
                        fluidNote               giFluid, ichannel - 1, ikey, ivelocity
endin

instr 90
aL, aR                  fluidOut                giFluid
                        outs                    aL, aR
endin

''')

csound.setScore(score.getCsoundScore())
csound.setCommand("csound -RWdfo ./town.wav ./temp.orc ./temp.sco")
csound.exportForPerformance()
model.perform()

