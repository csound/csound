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

instr 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 ; FluidSynth General MIDI
; INITIALIZATION
; Channel, bank, and program determine the preset, that is, the actual sound.
ichannel                =                       p1
                        if                      ichannel != 10 igoto nonpercussion
iprogram                =                       128
                        igoto                   resume
                        nonpercussion:
iprogram                =                       ichannel * 8
                        resume:
ikey                    =                       p4
ivelocity               =                       p5
ijunk6                  =                       p6
ijunk7                  =                       p7
; AUDIO
istatus                 =                       144
                        print                   iprogram, istatus, ichannel, ikey, ivelocity
aleft, aright           fluid                   "c:/MusicProgrammingBook/examples/63.3mg The Sound Site Album Bank V1.0.SF2", iprogram, istatus, ichannel, ikey, ivelocity, 1
                        outs                    aleft, aright
endin

''')

csound.setCommand("csound --opcode-lib=c:/cvs/CVSROOT/csound/bin/Soundfonts.dll -RWdfo ./town.wav ./temp.orc ./temp.sco")
model.perform()

