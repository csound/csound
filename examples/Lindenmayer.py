# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.
import gc
import os
import sys
import math
import os.path
import CsoundVST
gc.disable()
model = CsoundVST.MusicModel()
model.setCppSound(csound)
lindenmayer = CsoundVST.Lindenmayer()
lindenmayer.setAxiom("b")
lindenmayer.setAngle(2.0 * math.pi / 9.0)
lindenmayer.addRule("b", " b [  Ti-1 a b ] Tt+1 Tk-2 a N b Tt+3 N Tt+1 Tk+3 b [ Ti+1 b a ] N")
lindenmayer.addRule("a", " N Tt+1 Tk+4 N [ Tk+14 b ] Tk+12 N Tk-9 Tt-1 [ Tt+1 Tk-12 a ] N ")
lindenmayer.setIterationCount(6)
lindenmayer.generate()
print 'Generated events: ', + len(lindenmayer.getScore())
random = CsoundVST.Random()
random.createDistribution("uniform_real")
random.setElement(7, 11, 1)
rescale = CsoundVST.Rescale()
rescale.setRescale( 0, 1, 1,  0,     240)
rescale.setRescale( 1, 1, 1,  4,       4)
rescale.setRescale( 3, 1, 1,  2,       6)
rescale.setRescale( 4, 1, 1, 30,      66)
rescale.setRescale( 5, 1, 1, 70,      15)
rescale.setRescale( 7, 1, 1, -0.75,    1.5)
rescale.setRescale(10, 1, 1,  1392,    0)
random.addChild(lindenmayer)
rescale.addChild(random)
model.addChild(rescale)
model.generate()
filename = os.path.abspath('Lindenmayer-2006-06-10-a.py')
print 'Filename:', filename
instruments = { 2: 12, 3:  7, 4:13, 5:9, 6:13, 7:13, 8:13, 9:13 }
levels =      { 2:  0, 3:  0, 4:  0, 5: 0, 6: 0, 7: 0, 8: 0, 9: 0 }
score = model.getScore()
for i in xrange(len(score)):
    event = score[i]
    event.thisown = 0
    #print event.toString()
    insno = math.floor(event.getInstrument())
    velocity = event.getVelocity()
    event.setInstrument(instruments[insno])
    event.setVelocity(levels[insno] + velocity)
    score[i] = event
print 'Filename:', filename
model.setConformPitches(True)
csound.load('d:/utah/home/mkg/projects/csoundd/examples/CsoundVST.csd')
#csound.load('d:/utah/home/mkg/projects/music/library/silence.csd')
csound.setCommand("csound -m7 -RWdfo" + filename + ".wav " + filename + ".orc " + filename + ".sco")
csound.setFilename(filename)
print "Events in generated score:", len(score)
duration = score.getDuration()
print 'Duration: %9.4f' % (duration)
model.createCsoundScore('''
; EFFECTS MATRIX

; Chorus to Reverb
i 1 0 0 200 210 0.0
; Chorus to Output
i 1 0 0 200 220 0.05
; Reverb to Output
i 1 0 0 210 220 2.0

; SOUNDFONTS OUTPUT

; Insno Start   Dur     Key 	Amplitude
i 190 	0       %f      0	64.

; MASTER EFFECT CONTROLS

; Chorus.
; Insno	Start	Dur	Delay	Divisor of Delay
i 200   0       %f      10      30

; Reverb.
; Insno	Start	Dur	Level	Feedback	Cutoff
i 210   0       %f      0.81    0.0  		16000

; Master output.
; Insno	Start	Dur	Fadein	Fadeout
i 220   0       %f      0.1     0.1

''' % (duration, duration, duration, duration))
#print csound.getScore()
print csound.getCommand()
csound.perform()





































