# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition 
# based on translating the orbit of a chaotic attractor to a score.

import CsoundVST

model = CsoundVST.MusicModel()
model.setCppSound(csound)
strangeAttractor = CsoundVST.StrangeAttractor()
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

random = CsoundVST.Random()
random.createDistribution("uniform_01")
random.setElement(6, 11, 1)
random.setElement(8, 11, 1)

rescale = CsoundVST.Rescale()
rescale.setRescale( 0, 1, 1,  0,     300)
rescale.setRescale( 1, 1, 0,  2,       4)
rescale.setRescale( 3, 1, 1,  0,      24)
rescale.setRescale( 4, 1, 1, 33,      50)
rescale.setRescale( 5, 1, 1, 60,      16)
rescale.setRescale( 7, 1, 1, -0.5,     1)
rescale.setRescale(10, 1, 1,  0,    4095)
random.addChild(strangeAttractor)
rescale.addChild(random)

# Add these nodes to the builtin MusicModel instance.
model.addChild(rescale)
model.setTonesPerOctave(12.0)

csound.load("c:/projects/csound5/examples/CsoundVST.csd")
csound.setCommand("csound --opcode-lib=c:/projects/csound5/fluid.dll -RWdfo ./StrangeAttractor.wav ./StrangeAttractor.orc ./StrangeAttractor.sco")
model.render()















































































































































































