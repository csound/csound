# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.
# This should be run from the csound/examples/python directory.
# If you have bwfmetaedit, sox, and lame installed, change the last line to
# model.performAll().
import gc
import os
import sys
import math
import os.path
import CsoundAC
gc.disable()
model = CsoundAC.MusicModel()
model.setFilename('Lindenmayer.py')
model.setArtist('Michael_Gogins')
model.setTitle('Lindenmayer')
model.setCopyright('C_2010_Michael_Gogins')
model.setLicense('Creative_Commons_by_nc_nd')
model.setAlbum('Csound_Examples')
csound = model.getCppSound()
print 'csound:', csound
lindenmayer = CsoundAC.Lindenmayer()
print 'lindenmayer:', lindenmayer
lindenmayer.setAxiom("b")
lindenmayer.setAngle(2.0 * math.pi / 3.0)
lindenmayer.addRule("b", " b [ Ti-1 a N b ] Tt+1 Tk-3.1 a N b Tt+3 N Tt+1.3 Tk+2 b [ Ti+1 a b ] N")
lindenmayer.addRule("a", " N Tt+1.1 Tk+1 N [ Tk+2 b ] Tk+3 N Tk-3 Tt-1 [ Tt+1 Tk-4 a ] N ")
lindenmayer.setIterationCount(6)
lindenmayer.generate()
print 'generated...'
random = CsoundAC.Random()
print 'random:', random

random.createDistribution("uniform_real")
random.setElement(7, 11, 1)
rescale = CsoundAC.Rescale()
rescale.setRescale( 0, 1, 1,  0,      60)
rescale.setRescale( 1, 1, 1,  2,       4)
rescale.setRescale( 3, 1, 1,  2,       8)
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
csound.setCommand("csound -m195 -RWdfo" + filename + '.wav')
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
score.setDuration(180.0)
duration = score.getDuration()
print 'Duration: %9.4f' % (duration)
print 'Command:  ', csound.getCommand()
print 'Score:'
model.createCsoundScore()
scoreString = csound.getScore()
print scoreString
print 'Performing...'
# model.performAll()
model.perform()
