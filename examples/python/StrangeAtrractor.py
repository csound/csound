# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition
# based on translating the orbit of a chaotic attractor to a score.

import CsoundAC
import time

CsoundAC.Random_seed(int(time.time()))

model = CsoundAC.MusicModel()
model.setCppSound(csound)
strangeAttractor = CsoundAC.StrangeAttractor()
strangeAttractor.reset()
strangeAttractor.setDimensionCount(4)
strangeAttractor.setAttractorType(3)
strangeAttractor.setIterationCount(2000)
while strangeAttractor.searchForAttractor():
        pass
print "ScoreType = ", strangeAttractor.getScoreType()
#strangeAttractor.setCode('OMPDPTWXBMXDRRWRJCLXRVRTPWRYBMFPJBFFVGGNAKAGUIJFAKLAIAIEYNTHKFCUGRVDCYILYPTDNUITMFVGESBHSRKYWEGWTNVGNQKUMUJULEDMKRBGAFNIXVDMCLJIITUVQNNBXYLMCJYARPSACEYKFGHBVYXIWLCYJEBJIUCNFLKKNUXRHGWUONWFPMOALWLODJQTKLGFBYGXQYMKPPTYANECOPBJSDLLYJYERXEOBOWFKKYNXGHNCCYWTINDFIJXAUEEQXBEQLORVORIGISBC')
#strangeAttractor.setCode('OMPDPTWXBMXDRRWRJCLXRVRTPWRYBMFPJBFFVGGNAKAGUIJFAKLAIAIEYNTHKFCUGRVDCYILYPTDNUITMFVGESBHSRKYWEGWTNVGNQKUMUJULEDMKRBGAFNIXVDMCLJIITUVQNNBXYLMCJYARPSACEYKFGHBVYXIWLCYJEBJIUCNFLKKNUXRHGWUONWFPMOALWLODJQTKLGFBYGXQYMKPPTYANECOPBJSDLLYJYERXEOBOWFKKYNXGHNCCYWTINDFIJXAUEEQXBEQLORVORIGISBC')
#strangeAttractor.setCode('SJEOSGBEDTSYIUJLGICPVYJPNDRYXLUDFHBSMFMNEBWKWJPXLOWIARKQBEXJDBSYVQHHACDSVATCJELUKHHBUEJUXNKEMNARCJBMBVSBKS')
#strangeAttractor.setCode('WVBEKJFDCWESFBQVBQAISKEKDYLCVBXTSWATPPGCNCLVTXHJWEETRGIHRRMFVVPPBYNEHKXBSYKYVTYINHCNVFCNOGOYCMQANLBLBCJQJCUWRQMIIWSSIEQCIIOHEXQNMDAFGSWQRKCGOSYTTHYAONPBUCDVAPMAYUCBOWJYSUHUTVUKEISHDEIENUXJVHNLQNRNKGIHOKLXFSMGMTSNJMSYTSECNJHLBDWEJJCETRSDOKVFDIQDSMWWMYVIANAIANOXDDPKLOJFOUMWBQFQADWBE')
#strangeAttractor.setCode('TPKRWTCCXRDACSDWAAXJATYTBGEWBYGLUCGENDPQMCTRVJLNNFKWNEJOQVVUCWXDPUQFDFMJGMLRVDESPKXXSIXVNWJOAIDHFUPVBQJIMCBTAHJRKHDPFCHOKSEBOKMQKSWQXSXFVAJNTXFLTGGVVUEOADTMTAPOIRWUMAHPJ')
print "Code = ", strangeAttractor.getCode()
strangeAttractor.generate()
print "Generated events = ", len(strangeAttractor.getScore())

# Place the Lindenmayer node inside a Random node to randomize velocity and pan,
# place the Random node inside a Rescale node,
# and place the Rescale node inside the MusicModel.

random = CsoundAC.Random()
random.createDistribution("uniform_01")
random.setElement(6, 11, 1)
random.setElement(8, 11, 1)

rescale = CsoundAC.Rescale()
rescale.setRescale( 0, 1, 1,  0,     300)
rescale.setRescale( 1, 1, 0,  2,       4)
rescale.setRescale( 3, 1, 1, 10,       4)
rescale.setRescale( 4, 1, 1, 36,      60)
rescale.setRescale( 5, 1, 1, 35,      12)
rescale.setRescale( 7, 1, 1, -0.5,     1)
rescale.setRescale(10, 1, 1,  0,    4095)
random.addChild(strangeAttractor)
rescale.addChild(random)

# Add these nodes to the builtin MusicModel instance.
model.addChild(rescale)
model.setTonesPerOctave(12.0)
model.generate()

csound.load("./CsoundAC.csd")
csound.setCommand("csound -RWdfo StrangeAttractor.wav StrangeAttractor.orc StrangeAttractor.sco")
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
csound.perform()








