# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.
import gc
import os
import sys
import math
import os.path
import CsoundAC
import psyco
gc.disable()
model = CsoundAC.MusicModel()
csound = model.getCppSound()
lindenmayer = CsoundAC.Lindenmayer()
lindenmayer.setAxiom("b")
lindenmayer.setAngle(2.0 * math.pi / 9.0)
lindenmayer.addRule("b", " b [  Ti-1 a b ] Tt+1 Tk-3.1 a N b Tt+3 N Tt+1.3 Tk+2 b [ Ti+1 a b ] N")
lindenmayer.addRule("a", " N Tt+1.1 Tk+1 N [ Tk+2 b ] Tk+3 N Tk-3 Tt-1 [ Tt+1 Tk-4 a ] N ")
lindenmayer.setIterationCount(5)
lindenmayer.generate()
random = CsoundAC.Random()
random.createDistribution("uniform_real")
random.setElement(7, 11, 1)
rescale = CsoundAC.Rescale()
rescale.setRescale( 0, 1, 1,  0,     240)
rescale.setRescale( 1, 1, 1,  2,       4)
rescale.setRescale( 3, 1, 1,  2,       6)
rescale.setRescale( 4, 1, 1, 36,      60)
rescale.setRescale( 5, 1, 1, 20,      15)
rescale.setRescale( 7, 1, 1, -0.75,    1.5)
scale = 'E major'
scalenumber = CsoundAC.Conversions_nameToM(scale)
print '"%s" = %s' % (scale, scalenumber)
rescale.setRescale(10, 1, 1,  scalenumber,    0)
random.addChild(lindenmayer)
rescale.addChild(random)
model.addChild(rescale)
model.generate()
filename = os.path.abspath('Lindenmayer.py')
print 'Filename:', filename
model.setConformPitches(True)
csound.load('../CsoundAC.csd')
csound.setCommand("csound -+id_artist=Michael_Gogins -+id_copyright=Copyright_2007_by_Michael_Gogins -+id_title=Lindenmayer -m3 -RWZdfo" + filename + ".wav " + filename + ".orc " + filename + ".sco")
csound.setFilename(filename)
score = model.getScore()
print 'Events in generated score:', len(score)
duration = score.getDuration()
print 'Duration: %9.4f' % (duration)
score.arrange(0, 7)
score.arrange(1, 5)
score.arrange(2, 13)
score.arrange(3, 10)
score.arrange(4, 14)
score.arrange(5, 7)
score.arrange(6, 5)
score.arrange(7, 9)
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
model.performAll()