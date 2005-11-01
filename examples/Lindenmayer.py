# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.

import os
import os.path
import math
import CsoundVST

model = CsoundVST.MusicModel()
model.setCppSound(csound)

# Create and generate a Lindenmayer system node.
lindenmayer = CsoundVST.Lindenmayer()
lindenmayer.setAxiom("[ Tt+2.5 Tk+7 a ] Ti+1 b")
lindenmayer.addRule("a", "a Tk+6 g Tt+1.125 St*1.25 b a St*0.8 N g Tx-1 Tv+2 b b Tv-2 Tx+1 Tt+2.5 a N Ti+1 a Ti-1 a Tk-7.0025 Tt+1 N a")
lindenmayer.addRule("b", "b Tk+3.0025 Tt+1.25 N  Ti+1 b a [ Sk*2 a Tt+1 Tk+2 N ] Ti-1 N Tx-1 Tv+2 a Tv-2 Tx+1 Tt+2.0 N b N St*0.8 b St*1.25  b Tk-3      Tt+1.375 b N")
# Do a little branching and rotating, to create some polyphony.
lindenmayer.addRule("g", "St*1.01 [ Ti+1 Tk-2 N ]")

lindenmayer.setIterationCount(4)
lindenmayer.generate()
print 'Generated events: ', + len(lindenmayer.getScore())

# Place the Lindenmayer node inside a Random node to randomize velocity and pan,
# place the Random node inside a Rescale node,
# and place the Rescale node inside the MusicModel.

random = CsoundVST.Random()
random.createDistribution("uniform_real")
random.setElement(6, 11, 1)
random.setElement(8, 11, 1)
rescale = CsoundVST.Rescale()
rescale.setRescale(0, 1, 1,  0,     300)
rescale.setRescale(1, 1, 1,  2.75,    4)
rescale.setRescale(3, 1, 1,  2,      4)
rescale.setRescale(4, 1, 1, 33,      60)
rescale.setRescale(5, 1, 1, 50,      16)
rescale.setRescale(7, 1, 1, -0.5,     1)
random.addChild(lindenmayer)
rescale.addChild(random)

# Add these nodes to the builtin MusicModel instance.
model.addChild(rescale)
model.generate()
filename = csound.getFilename();
print 'Filename:', filename
csound.load("c:/utah/home/mkg/projects/csound5/examples/CsoundVST.csd")
csound.setFilename(filename);
csound.setCommand("csound -RWdfo " + filename + ".wav " + filename + ".orc " + filename + ".sco")
print csound.getCommand()
instruments = { 2: 3, 3: 2, 4: 4, 5:11, 6:11}
levels =      { 2: 0, 3: 0, 4: 0, 5: 0, 6: 0}

for event in model.getScore():
        print '  ', event.toString()
        insno = math.floor(event.getInstrument())
        velocity = event.getVelocity()
        event.setInstrument(instruments[insno])
        event.setVelocity(levels[insno] + velocity)
        print '=>', event.toString()

filename = csound.getFilename();
print 'Filename:', filename
print "Command", csound.getCommand()
print 'Creating Csound arrangement...'
#model.setConformPitches(True)
csound.load('c:/utah/home/mkg/projects/music/library/orchestra.csd')
csound.setCommand("csound -m7 -RWdfo " + filename + ".wav " + filename + ".orc " + filename + ".sco")
csound.setFilename(filename)
print "Events in generated score:", len(model.getScore())
model.createCsoundScore('''
; SoundFonts
; to Chorus
i 1 0 0 100 200 0
; to Reverb
i 1 0 0 100 210 2
; to Output
i 1 0 0 100 220 1

; Kelley Harpsichord
; to Chorus
i 1 0 0 4 200 0.5
; to Reverb
i 1 0 0 4 210 1
; to Output
i 1 0 0 4 220 1

; Kung Xanadu 1
; to Chorus
i 1 0 0 5 200 0
; to Reverb
i 1 0 0 5 210 0
; to Output
i 1 0 0 5 220 1

; Kung Xanadu 2
; to Chorus
i 1 0 0 6 200 0
; to Reverb
i 1 0 0 6 210 0
; to Output
i 1 0 0 6 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 7 200 0
; to Reverb
i 1 0 0 7 210 0
; to Output
i 1 0 0 7 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 8 200 0
; to Reverb
i 1 0 0 8 210 0
; to Output
i 1 0 0 8 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 9 200 0
; to Reverb
i 1 0 0 9 210 0
; to Output
i 1 0 0 9 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 10 200 0
; to Reverb
i 1 0 0 10 210 0
; to Output
i 1 0 0 10 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 11 200 0
; to Reverb
i 1 0 0 11 210 1
; to Output
i 1 0 0 11 220 1

; Kung Xanadu 3
; to Chorus
i 1 0 0 12 200 .2
; to Reverb
i 1 0 0 12 210 .1
; to Output
i 1 0 0 12 220 1

; Chorus to Reverb
i 1 0 0 200 210 1.0
; Chorus to Output
i 1 0 0 200 220 0.05
; Reverb to Output
i 1 0 0 210 220 0.1

i 100 0 [550.0+15]  95     95        0
i 200 0  550.0      10     30
i 210 0 [550.0+15]  0.7   0.8  20000
i 220 0  550.0      16     10
''')
csound.perform()
