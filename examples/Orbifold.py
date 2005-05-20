'''
V O I C E - L E A D I N G   O R B I F O L D
F O R   T R I C H O R D S   I N   1 2 T E T 
Copyright 2005 by Michael Gogins.
This software is licensed under the GNU Lesser General Public License,
in other words, it is open source software.

This program uses Python and VPython to display an interactive 3-dimensional
model of the complete trichord Tonnetz, i.e. the voice-leading orbifold
for trichords. Selected trichords are played in real time by CsoundVST.

See Dmitri Tymoczko, http://music.princeton.edu/~dmitri/voiceleading.pdf,
on voice-leading orbifolds. Many thanks to Professor Tymoczko for this article
and for his helpful comments and suggestions.

The following software is used in this program:
Python 2.3 from http://www.python.org
VPython from http://www.vpython.org
CsoundVST from http://csound.sourceforge.net

N O T E
Wait for Csound to finish compiling before selecting any trichords.
You may need to change the dac number (sound card) in the code below.
And you need to have an ASIO driver for your sound card (or, ASIO4ALL).
'''
print __doc__
import gc
import sys
import time
# Change enableCsound to True if you have installed CsoundVST.
# Importing CsoundVST automatically creates a csound object.
enableCsound = False
if enableCsound:
    import CsoundVST
from visual import *
import random
scene.title = "VOICE-LEADING ORBIFOLD: click ball=play, drag right button=spin, drag middle button=zoom, close window to stop"
scene.fullscreen = 0
scene.width = 800
scene.height = 600
scene.autoscale = 1
scene.exit = 0
def unorder(orderedTrichord):
    list = [orderedTrichord[0], orderedTrichord[1], orderedTrichord[2]]
    list.sort()
    return (list[0], list[1], list[2])
def modulusOf(trichord):
    newTrichord = ((trichord[0] - trichord[0] + 144) % 12,
               (trichord[1] - trichord[0] + 144) % 12,
               (trichord[2] - trichord[0] + 144) % 12)
    return newTrichord    
def unorderedType(trichord):
    trichord = modulusOf(trichord)
    trichord = unorder(trichord)
    return trichord
        
trichords = {}
balls = {}
ballsForChordTypes = {}
def setColor(ball):
    inversion1 = unorderedType(ball.trichord)
    inversion2 = (inversion1[2] - 8, inversion1[0] + 4, inversion1[1] + 4)
    inversion3 = (inversion2[2] - 8, inversion2[0] + 4, inversion2[1] + 4)
    inversion1 = unorder(modulusOf(inversion1))
    inversion2 = unorder(modulusOf(inversion2))
    inversion3 = unorder(modulusOf(inversion3))
    inversion1Key = str(inversion1)
    inversion2Key = str(inversion2)
    inversion3Key = str(inversion3)
    if   inversion1Key in ballsForChordTypes:
        ball.color = ballsForChordTypes[inversion1Key].color
    elif inversion2Key in ballsForChordTypes:
        ball.color = ballsForChordTypes[inversion2Key].color
    elif inversion3Key in ballsForChordTypes:
        ball.color = ballsForChordTypes[inversion3Key].color
    else:
        # Color major triads red.
        if   inversion1 == (0, 4, 7) or inversion2 == (0, 4, 7) or inversion3 == (0, 4, 7):
            ball.color = (1.0,0.0,0.0)
        # Color augmented triads white.
        elif inversion1 == (0, 4, 8) or inversion2 == (0, 4, 8) or inversion3 == (0, 4, 8):
            ball.color = (1.0,1.0,1.0)
        # Color minor triads blue.
        elif inversion1 == (0, 3, 7) or inversion2 == (0, 3, 7) or inversion3 == (0, 3, 7):
            ball.color = (0.0,0.0,1.0)
        else:
            hue = (inversion1[0] + inversion1[1] * 2.0 + inversion1[2]) / 44.0
            saturation = 1.0
            value = 1.0
            ball.color = color.hsv_to_rgb((hue, saturation, value))
voices = 3
modulus = 12
octaves = 2
layers = (modulus - 1) * voices * octaves
trichordCount = 0
for layer in xrange(0, layers + 1):
    for x in xrange(layer / voices - (2 * modulus / voices), (layer / voices + (modulus  / voices) + 1) * octaves):
        for y in xrange(x, (layer / voices + (2 * modulus  / voices) + 1) * octaves):
            for z in xrange(y, (layer / voices + (2 * modulus / voices) + 1) * octaves):
                sum = x + y + z
                if sum == layer and z <= (x + modulus):
                    trichord = unorder((x, y, z))
                    if trichord not in trichords:
                        trichordCount = trichordCount + 1
                        trichords[trichord] = trichord
                        ball = sphere(pos = trichord, radius = 0.125)
                        # Make a label for each ball, and hide it till it's needed.
                        ball.trichord = unorder(((x + 144) % modulus, (y + 144) % modulus, (z + 144) % modulus))
                        # print ball.trichord
                        balls[str(ball.trichord)] = ball;
                        if ball.trichord == (1,5,10):
                            scene.center = ball.pos
                        ball.layer = layer
                        ball.sum = ball.trichord[0] + ball.trichord[1] + ball.trichord[2]
                        setColor(ball)
                        ball.name = "%2d (%2d,%2d,%2d)\n  (%2d,%2d,%2d)" % (layer, ball.trichord[0], ball.trichord[1], ball.trichord[2], ball.pos[0],ball.pos[1],ball.pos[2])
                        ball.label = label(pos = trichord, text = ball.name, height = 11, box = 1, opacity = 0.3, visible = 0, line = 1, xoffset=40,yoffset=40 )
    print
# Enlarge C, its closest F, and its closest G.
#balls['(0, 4, 7)'].radius = .25
#balls['(0, 5, 9)'].radius = .25
#balls['(2, 7, 11)'].radius = .25
def connect(origin, neighbor):
    if neighbor in trichords:
        curve(pos = [origin, neighbor], color = (0.67, 0.67, 0.67))
##for trichord in trichords.values():
##    connect(trichord, unorder((trichord[0] + 1.0, trichord[1], trichord[2])))
##    connect(trichord, unorder((trichord[0], trichord[1] + 1.0, trichord[2])))
##    connect(trichord, unorder((trichord[0], trichord[1], trichord[2] + 1.0)))
##    connect(trichord, unorder((trichord[0] - 1.0, trichord[1], trichord[2])))
##    connect(trichord, unorder((trichord[0], trichord[1] - 1.0, trichord[2])))
##    connect(trichord, unorder((trichord[0], trichord[1], trichord[2] - 1.0)))
if enableCsound:
    def csoundThreadRoutine():
        csound.load('c:/utah/home/mkg/projects/csound5/examples/CsoundVST.csd')
        csound.setCommand('csound -d -m0 -b600 -B600 -odac2 temp.orc temp.sco')
        csound.exportForPerformance()
        csound.compile()
        gc.disable()
        while True:
            csound.performKsmps()
    csoundThread = threading.Thread(None, csoundThreadRoutine)
    csoundThread.start()
pickedBall = None
oldBall = None
while scene.visible:
    if scene.mouse.clicked:
        try:
            m = scene.mouse.getclick()
            if oldBall:
                oldBall.label.visible = 0
            if pickedBall:
                pickedBall.label.visible = 0
            if str(m.pick.trichord) in balls:
                oldBall = pickedBall
                pickedBall = m.pick
                if pickedBall:
                    pickedBall.label.visible = 1
                    note1 = "i  8 0 4 %d 70 0 -.75" % (60 + pickedBall.pos[0])
                    note2 = "i  8 0 4 %d 70 0  .0"  % (60 + pickedBall.pos[1])
                    note3 = "i  8 0 4 %d 70 0  .75" % (60 + pickedBall.pos[2])
                    if enableCsound:
                        csound.inputMessage(note1)
                        csound.inputMessage(note2)
                        csound.inputMessage(note3)
        except:
            pass
        scene.mouse.events = 0
if enableCsound:
    csound.cleanup()
