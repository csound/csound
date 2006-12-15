import CsoundVST
import random
filename = 'test.py'
model = CsoundVST.MusicModel()
model.setCppSound(csound)
score = model.getScore()


def addVoiceleadingTest(sequence, voiceleadingNode, duration):
	random = CsoundVST.Random()
	random.thisown=0
	random.createDistribution("uniform_01")
	random.eventCount = 200
	random.setElement(CsoundVST.Event.INSTRUMENT, 11, 1)
	random.setElement(CsoundVST.Event.TIME, 11, 1)
	random.setElement(CsoundVST.Event.DURATION, 11, 1)
	random.setElement(CsoundVST.Event.KEY, 11, 1)
	random.setElement(CsoundVST.Event.VELOCITY, 11, 1)
	random.setElement(CsoundVST.Event.PAN, 11, 1)
	rescale = CsoundVST.Rescale()
	rescale.setRescale(CsoundVST.Event.INSTRUMENT, 1, 1,  1.,       4.)
	rescale.setRescale(CsoundVST.Event.TIME,       1, 1, 1.,       duration)
	rescale.setRescale(CsoundVST.Event.DURATION,   1, 1, 0.25,       2.)
	rescale.setRescale(CsoundVST.Event.KEY,        1, 1, 36.,       60.)
	rescale.setRescale(CsoundVST.Event.VELOCITY,   1, 1, 60.,       9.)
	rescale.setRescale(CsoundVST.Event.PAN,        1, 1, -0.25,    1.5)
	rescale.addChild(random)
	rescale.thisown=0
	voiceleading.normalizeTimes = True
	voiceleading.thisown=0
	voiceleading.addChild(rescale)
	sequence.addChild(voiceleading)

sequenceDuration = 20.0

sequence = CsoundVST.Sequence()
model.addChild(sequence)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.S(0.0, CsoundVST.Conversions_nameToPitchClassSet("CM7"))
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.S(0.0, CsoundVST.Conversions_nameToPitchClassSet("FM7"))
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.S(0.0,"FM7")
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.PT(0.0, CsoundVST.Conversions_nameToPitchClassSet("FM7"), 2)
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.PTV(0.0, CsoundVST.Conversions_nameToPitchClassSet("FM7"), 4, 5)
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.PTV(0.0, CsoundVST.Conversions_nameToPitchClassSet("FM7"), 4, 6)
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.S(0.0,"FM7")
voiceleading.S(1.0,"Bbm7")
voiceleading.S(2.0,"E7")
voiceleading.S(3.0,"Abm7")
voiceleading.S(4.0,"FM7")
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.SV(0.0,"FM7", 10)
voiceleading.SV(1.0,"Bbm7", 11)
voiceleading.SV(2.0,"E7", 12)
voiceleading.SV(3.0,"Abm7", 13)
voiceleading.SV(4.0,"FM7", 14)
#addVoiceleadingTest(sequence, voiceleading, sequenceDuration)

voiceleading = CsoundVST.VoiceleadingNode()
voiceleading.SL(0.0,"FM7", True)
voiceleading.SL(1.0,"Bbm7", True)
voiceleading.SL(2.0,"E7", True)
voiceleading.SL(3.0,"Abm7", True)
voiceleading.SL(4.0,"FM7", True)
addVoiceleadingTest(sequence, voiceleading, sequenceDuration)



model.generate()
score = model.getScore()
for e in score:
	print e.toString()
score.arrange(0,56, 1.0)
score.arrange(1,56, 1.0)
score.arrange(2,56, 1.0)
score.arrange(3,56, 1.0)
score.arrange(4,56, 1.0)
score.arrange(5,56, 1.0)
score.arrange(6,56, 1.0)
score.arrange(7,56, 1.0)
score.save(filename + '.mid')
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
score.save(filename + '.mid')
csound.exportForPerformance()
for key,value in csound.getInstrumentNames().items():
	print 'Instrument %3d: %s' % (key, value)
for note in score:
	print note.toString()
csound.perform()








































































