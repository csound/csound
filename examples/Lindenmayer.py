# Copyright (c) 2002, 2003 by Michael Gogins. All rights reserved.
# Tutorial demonstrating a MusicModel composition based on a Lindenmayer system.

import os
import os.path
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
random.createDistribution("uniform_01")
random.setElement(6, 11, 1)
random.setElement(8, 11, 1)
rescale = CsoundVST.Rescale()
rescale.setRescale(0, 1, 1,  0,     300)
rescale.setRescale(1, 1, 1,  2.75,    4)
rescale.setRescale(3, 1, 1,  4,      12)
rescale.setRescale(4, 1, 1, 33,      60)
rescale.setRescale(5, 1, 1, 65,      16)
rescale.setRescale(7, 1, 1, -0.5,     1)
random.addChild(lindenmayer)
rescale.addChild(random)

# Add these nodes to the builtin MusicModel instance.
model.addChild(rescale)
filename = csound.getFilename();
print 'Filename:', filename
csound.load("c:/projects/csound5/examples/CsoundVST.csd")
csound.setFilename(filename);
csound.setCommand("csound -RWdfo " + filename + ".wav " + filename + ".orc " + filename + ".sco")
model.render()






























































































































































