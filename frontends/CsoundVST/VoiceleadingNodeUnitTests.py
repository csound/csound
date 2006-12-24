'''
/*
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
This script tests the following Score methods:

    virtual void PT(double time, double P, double T);
    virtual void PTV(double time, double P, double T, double V);
    virtual void PTL(double time, double P, double T, bool avoidParallels = true);
    virtual void C(double time, double C_);
    virtual void C(double time, std::string C_);
    virtual void CV(double time, double C, double V);
    virtual void CV(double time, std::string C, double V);
    virtual void CL(double time, double C, bool avoidParallels = true);
    virtual void CL(double time, std::string C, bool avoidParallels = true);
    virtual void V(double time, double V_);
    virtual void L(double time, bool avoidParallels = true);

Most of these methods are tested, in the order listed above,
in separate segments of notes. Most methods have more than one test segment.

Run the script, then examine the generated VoiceleadingNodeUnitTests.py.mid MIDI
sequence file in a notation program and verify the correctness of each section.
'''

import CsoundVST
import random
filename = 'VoiceleadingNodeUnitTests.py'
model = CsoundVST.MusicModel()
model.setCppSound(csound)
score = model.getScore()
CsoundVST.System_setMessageLevel(1+2+4+8)

def addVoiceleadingTest(sequence, voiceleadingNode, duration):
	random = CsoundVST.Random()
	random.thisown=0
	random.createDistribution("uniform_01")
	random.eventCount = 200
	random.setElement(CsoundVST.Event.INSTRUMENT, 	11, 1)
	random.setElement(CsoundVST.Event.TIME, 	11, 1)
	random.setElement(CsoundVST.Event.DURATION, 	11, 1)
	random.setElement(CsoundVST.Event.KEY, 		11, 1)
	random.setElement(CsoundVST.Event.VELOCITY, 	11, 1)
	random.setElement(CsoundVST.Event.PAN, 		11, 1)
	rescale = CsoundVST.Rescale()
	rescale.setRescale(CsoundVST.Event.INSTRUMENT, 1, 1,   1.,       4.)
	rescale.setRescale(CsoundVST.Event.TIME,       1, 1,   1.,       duration)
	rescale.setRescale(CsoundVST.Event.DURATION,   1, 1,   0.25,     1.)
	rescale.setRescale(CsoundVST.Event.STATUS,     1, 1, 144.,       0.)
	rescale.setRescale(CsoundVST.Event.KEY,        1, 1,  36.,      60.)
	rescale.setRescale(CsoundVST.Event.VELOCITY,   1, 1,  60.,       9.)
	rescale.setRescale(CsoundVST.Event.PAN,        1, 1,  -0.25,     1.5)
	rescale.addChild(random)
	rescale.thisown=0
	voiceleading.normalizeTimes = True
	voiceleading.thisown=0
	voiceleading.addChild(rescale)
	sequence.addChild(voiceleading)

sequenceDuration = 20.0

sequence = CsoundVST.Sequence()
model.addChild(sequence)

#    virtual void PT(double time, double P, double T);

# 1
voiceleading = CsoundVST.VoiceleadingNode()
# Note: shouldn't actually be CM7 - but must be some M7 chord.
voiceleading.PT(0.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12)), 0.0)
voiceleading.PT(1.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12)), 5.0)
voiceleading.PT(2.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12)), 0.0)
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

# 2
voiceleading = CsoundVST.VoiceleadingNode()
# Note: shouldn't actually be FM7 - but must be some M7 chord.
voiceleading.PT(0.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("FM7"), 12)), 0.0)
voiceleading.PT(1.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("FM7"), 12)), 5.0)
voiceleading.PT(2.0, CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("FM7"), 12)), 0.0)
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void PTV(double time, double P, double T, double V);

# 3
voiceleading = CsoundVST.VoiceleadingNode()
# Note - shouldn't actually start with CM7, but should be revoicing of some M7 chord.
for i in xrange(11):
	voiceleading.PTV(float(i), CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12)), 0.0, float(i))
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#~ #    virtual void PTL(double time, double P, double T, bool avoidParallels = true);

# 4
voiceleading = CsoundVST.VoiceleadingNode()
# Note - shouldn't actually start with CM7, but should be stepwise progression of M7 chords.
for i in xrange(5):
	voiceleading.PTL(float(i), CsoundVST.Voicelead_cToP(CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12)), float(i))
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void C(double time, double C_);

# 5
voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.C(0.0, CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12))
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

# 6
voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.C(0.0, CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("FM7"), 12))
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void C(double time, std::string C_);

# 7
voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.C(0.0, "FM7")
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

# 8
voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.C(0.0, "FM7")
voiceleading.C(1.0, "Bbm7")
voiceleading.C(2.0, "E7")
voiceleading.C(3.0, "Abm7")
voiceleading.C(4.0, "FM7")
voiceleading.C(5.0, "FM7")
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void CV(double time, double C, double V);

# If SV for strings works and S for numbers works, then SV for numbers works; no test.

#    virtual void CV(double time, std::string C, double V);

# 9
voiceleading = CsoundVST.VoiceleadingNode()
for i in xrange(11):
	voiceleading.CV(float(i), CsoundVST.Voicelead_mToC(CsoundVST.Conversions_nameToM("CM7"), 12), float(i))
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void CL(double time, double C, bool avoidParallels = true);

# If CL for strings works and C for numbers works, then CV for numbers works; no test.

#    virtual void CL(double time, std::string C, bool avoidParallels = true);

# 10
voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.CL(0.0,"FM7", True)
voiceleading.CL(1.0,"Bbm7", True)
voiceleading.CL(2.0,"E7", True)
voiceleading.CL(3.0,"Abm7", True)
voiceleading.CL(4.0,"FM7", True)
voiceleading.CL(5.0,"FM7", True)
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

#    virtual void V(double time, double V_);

# If V works in combination, it should work by itself; no test.

#    virtual void L(double time, bool avoidParallels = true);

# if L works in combination, it should work by itself; no test.

model.generate()
score = model.getScore()
for i in xrange(12):
	score.arrange(i, 56, 1.0)
csound.load('d:\\utah\\home\\mkg\\projects\\music\\library\\CsoundVST.csd')
csound.setCommand("csound -m3 -RWfo " + filename + ".wav " + filename + ".orc " + filename + ".sco")
csound.setCommand("csound -m3 -r 88200 -k 882 -RWZfo " + filename + ".wav " + filename + ".orc " + filename + ".sco")
csound.setFilename(filename)
print "Events in generated score:", len(score)
duration = model.getScore().getDuration() + 8.0
model.createCsoundScore('''
; EFFECTS MATRIX

; Chorus to Reverb
i 1 0 0 200 210 0.0
; Leslie to Reverb
; i 1 0 0 201 210 0.5
; Chorus to Output
i 1 0 0 200 220 0.5
; Reverb to Output
i 1 0 0 210 220 0.5

; SOUNDFONTS OUTPUT

; Insno Start   Dur     Key 	Amplitude
i 190 	0       %f      0	84.

; PIANOTEQ OUTPUT

; Insno Start   Dur     Key 	Amplitude
i 191 	0       %f      0	64.

; MASTER EFFECT CONTROLS

; Chorus.
; Insno	Start	Dur	Delay	Divisor of Delay
i 200   0       %f      10      30

; Reverb.
; Insno	Start	Dur	Delay	Pitch mod	Cutoff
i 210   0       %f      0.87    0.015  		16000

; Master output.
; Insno	Start	Dur	Fadein	Fadeout
i 220   0       %f      0.1     0.1

''' % (duration, duration, duration, duration, duration))
csound.setCommand('csound -m3 -R -W -Z -f -r 44100 -k 441 -o dac8 temp.orc temp.sco')
csound.exportForPerformance()
for key,value in csound.getInstrumentNames().items():
	print 'Instrument %3d: %s' % (key, value)
index = 1
for note in score:
	print '%4d: %s' % (index, note.toString())
	index = index + 1
model.getScore().save(filename + ".mid")
csound.perform()











































































































