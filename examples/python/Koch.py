#!/usr/bin/python

# Copyright (C) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial showing how to generate a score using a Koch curve.

import os
import os.path

# Import the Csound 5 API.

import csnd6

# Create an instance of Csound (actually, CppSound).

csound = csnd6.CppSound()

# Enable Csound to print to the Python console.

#csound.setPythonMessageCallback()

# A recursive function for generating a score
# in the form of a Koch curve.
# Each note in a generating motive
# will get a smaller copy of the motive nested atop it,
# and so on.

gainsForLevels = {}
gainsForLevels[0] = 0.0
gainsForLevels[1] = 1.5
gainsForLevels[2] = 1.0
gainsForLevels[3] = 1.0
gainsForLevels[4] = 1.0
gainsForLevels[5] = 1.0
gainsForLevels[6] = 1.0
gainsForLevels[7] = 1.0

def Koch(generator, t, d, c, k, v, level, score):
        t1 = t
        d1 = d
        c1 = c
        k1 = k
        v1 = v
        pan = 0
        if level > 0:
                for i in range(0, generator.__len__(), 4):
                        k1 = k + generator[i]
                        v1 = generator[i + 1]
                        d1 = d  * generator[i + 2]
                        d2 = d1 * generator[i + 3]
                        score.addNote(level, t1, d2, k1, (70.0 + v1) * gainsForLevels[level], 0, pan)
                        Koch(generator, t1, d1, c1, k1 + 12, v1, level - 1, score)
                        t1 = t1 + d1

# Normalizes the times in a generator.

def normalizeGeneratorTimes(g):
        sum = 0.0
        for i in range(0, len(g), 4):
                sum = (sum + g[i + 2])
        for i in range(0, len(g), 4):
                g[i + 2] = g[i + 2] / sum

# Define two generators for the Koch function.
# Each consists of a sequence of tuples:
# {{add to MIDI key, add to MIDI velocity, relative time, normalized duration},...}

g = [  7,  0,  8,  1,
      -4,  0,  8,  .875,
       7,  1,  6,  1,
       9,  1,  6,  1,
      -4, -5,  8,  1,
      15,  4,  3,  .875,
      -9,  1,  6,  1
    ]

normalizeGeneratorTimes(g)

# The only differences between this generator and the first are the
# relative durations and dynamics of some sections.
# The relative pitches and numbers of events are identical.
# The different times will move the canon offset
# back and forth during performance.

h = [  7,  0,  8,  1,
      -4,  0,  6,  .875,
       7,  0,  3,  1,
       9,  1,  6,  1,
      -4,  1,  6,  1,
      15,  4,  6,  .875,
      -7, -5,  8,  1
#     -13,  0,  8,  1
    ]

normalizeGeneratorTimes(h)

# Generate events for 3 layers of each generator.

Koch(g, 0, 240, 0, 24, 32, 3, csound)

# The second generator makes a canon relative to the first.

Koch(h, 0.5, 240, 0, 30, 32, 3, csound);

# Set up an FluidSynth-based orchestra in Csound.

csound.setOrchestra('''
sr                      =                       44100
ksmps                   =                       100
nchnls                  =                       2
0dbfs                   =                       0.1

giFluidsynth            fluidEngine
giFluidGM               fluidLoad               "../../samples/sf_GMbank.sf2", giFluidsynth, 1
                        fluidProgramSelect      giFluidsynth, 0, giFluidGM,      0,   4
                        fluidProgramSelect      giFluidsynth, 1, giFluidGM,      0,   8
                        fluidProgramSelect      giFluidsynth, 2, giFluidGM,      0,   6
                        fluidProgramSelect      giFluidsynth, 3, giFluidGM,      0,   0
                        fluidProgramSelect      giFluidsynth, 4, giFluidGM,      0,   1
                        fluidProgramSelect      giFluidsynth, 5, giFluidGM,      0,  15
                        fluidProgramSelect      giFluidsynth, 6, giFluidGM,      0,  15

instr 1,2,3,4,5,6; FluidSynth
; INITIALIZATION
                        mididefault             60, p3
                        midinoteonkey           p4, p5
; Use channel assigned in fluidload.
ichannel                =                       p1 - 1
ikey                    =                       p4
ivelocity               =                       p5
ijunk                   =                       p6
ijunk                   =                       p7
                        fluidNote               giFluidsynth, ichannel, ikey, ivelocity
endin

instr 100 ; Fluidsynth output
; AUDIO
iamplitude              =                       0.75
aleft, aright           fluidOut                giFluidsynth
                        outs                    aleft * iamplitude, aright * iamplitude
endin
''')
# Set the Csound command line.

#csound.setCommand("csound -b100 -B100 -odac2 /tempk.orc /tempk.sco")
#csound.setCommand("csound -RWdfo koch.wav c:/tempk.orc c:/tempkk.sco")
csound.setCommand("csound -RWdfo koch.wav")
csound.addScoreLine("i 100 0 -1")

# Export the orchestra and generated score for performance.

#csound.exportForPerformance()

# Perform the generated score with the embedded orchestra.

csound.perform()




