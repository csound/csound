# Copyright (C) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial showing how to generate a score using a Koch curve.

import os
import os.path
import CsoundVST

# A recursive function for generating a score
# in the form of a Koch curve.
# Each note in a generating motive 
# will get a smaller copy of the motive nested atop it,
# and so on.

programsForInstruments = {6:80, 5:47, 4:49, 3:1, 2:15, 1:13}

def Koch(generator, t, d, c, k, v, level, score):
	t1 = t
	d1 = d
	c1 = c
	k1 = k
	v1 = v
	pan = 0
	if level > 0:
		for i in range(0, generator.__len__(), 4):
			k1 = 2 + k1 + generator[i]
			v1 = generator[i + 1]
			d1 = d  * generator[i + 2]
			d2 = d1 * generator[i + 3]
			if level == 0:
				pan = 0
			elif level == 1:
				pan = -.5
			elif level == 2:
				pan = .5
			else:
				pan == 0
			score.addNote(level, t1, d2, k1 - 12, v1, programsForInstruments[level], pan)
			Koch(generator, t1, d1, c1, k1, v1, level - 1, score)
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
      -4, -5,  6,  1,
      15,  4,  3,  .875,
      -7,  1,  6,  1
#    -13,  0,  8,  1 
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
      -4,  1,  6,  1,
      15,  4,  6,  .875,
      -7, -5,  8,  1
#     -13,  0,  8,  1 
    ]

normalizeGeneratorTimes(h)

# Generate events for 3 layers of each generator.

Koch(g, 0, 240, 0, 30, 32, 3, csound)

# The second generator makes a canon relative to the first.

Koch(h, 0.5, 240, 0, 37, 32, 3, csound);

csound.setOrchestra('''
sr = 44100
kr = 441
ksmps = 100   
nchnls = 2
0dbfs = .1

instr 1,2,3,4,5 ; FluidSynth General MIDI
; INITIALIZATION
; Channel, bank, and program determine the preset, that is, the actual sound.
ichannel		=			p1
iprogram		=			p6
ikey	 		= 			p4
ivelocity 		= 			p5 + 12
ijunk6 			= 			p6
ijunk7			=			p7
; AUDIO
istatus			=			144
;			print			iprogram, istatus, ichannel, ikey, ivelocity
aleft, aright		fluid			"c:/projects/csound5/samples/VintageDreamsWaves-v2.sf2", iprogram, istatus, ichannel, ikey, ivelocity, 1
			outs 			aleft, aright
endin

''')

csound.setCommand("csound --opcode-lib=c:/projects/csound5/fluid.dll -RWdfo ./koch.wav ./temp.orc ./temp.sco")

csound.exportForPerformance()
csound.perform()















































































































































































