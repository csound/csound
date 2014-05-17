'''
Copyright 2005, 22013, 2014 by Michael Gogins.

A concise geometric approach to common operations
in pragmatic music theory,
for use in score generating algorithms.

When run as a standalone program,
displays a model of the voice-leading space
for trichords and can orbit a chord through
the space, playing the results using Csound.

Voice-leading space is an orbifold of chords
with one dimension per voice, voices ordered by pitch,
pitch measured in tones per octave,
and a modulus equal to the range of the voices.
I.e., it is a complete ChordSpace3.
Root progressions are motions more or less
up and down the 'columns' of identically
structured chords. The closest voice-leadings are
between the closest chords in the space.
The 'best' voice-leadings are closest first
by 'smoothness,' and then  by 'parsimony.'
See Dmitri Tymoczko,
_The Geometry of Musical Chords_, 2005
(Princeton University).

This script also demonstrates the triadic
neo-Riemannian transformations
of leading-tone exchange (press l),
parallel (press p),
relative (press r),
and dominant (press d) progression.
See Alissa S. Crans, Thomas M. Fiore, and Raymon Satyendra,
_Musical Actions of Dihedral Groups_, 2008
(arXiv:0711.1873v2).

You can do plain old transpositions
by pressing 1, 2, 3, 4, 5, or 6.

You can move each voice independently with the arrow keys:
up arrow to move voice 1 up 1 semitone (shift for down),
right arrow to move voice 2 in the same way,
down arrow to move voice 3.

'''
print __doc__
import gc
import operator
import sys
import traceback
import time
import random
import sets
import threading
import copy
import collections
from visual import *
#from numpy import *
#import Image
#import ImageGrab
#import ImageOps

import csnd6
import CsoundAC
'''
Represents operations on chords in a voice-leading orbifold.
Chords can actually have more dimensions than voices,
but voice-leading operations affect only the specified number of voices,
which can be a lower subspace of the orbifold.
This is to enable using the lower subspace of chords to represent pitches,
and the higher subspace to represent other properties of music;
e.g. [0:4] can be a tetrachord, [4:8] durations, [8:12] loudnesses, and so on.
'''
class ChordSpace3(object):
    def __init__(self, voiceCount=3, cubeOctaveCount=2,octaveCount=3, tonesPerOctave=12, isCube=False, isPrism=False, isNormalPrism = False, debug=False):
        self.N = voiceCount
        self.octaveCount = octaveCount
        self.tonesPerOctave = tonesPerOctave
        self.isCube = isCube
        self.isPrism = isPrism
        self.isNormalPrism = isNormalPrism
        self.debug = debug
        self.R = self.tonesPerOctave * self.octaveCount
        self.NR = self.N * self.R
        self.cubeOctaveCount = cubeOctaveCount
        self.cubeTessitura = self.tonesPerOctave * self.cubeOctaveCount
        self.cubeRadius = 0.07
        self.prismRadius = self.cubeRadius * 2.0
        self.normalPrismRadius = self.prismRadius #* 2.0
    def getTessitura(self):
        if self.isCube:
            return self.cubeTessitura
        else:
            return self.R
    def sort(self, chord):
        c = array(chord, 'd').copy()
        d = c[0:self.N]
        d.sort()
        c[0:self.N] = d
        return c
    '''
    Move 1 voice.
    '''
    def move(self, chord_, voice, interval):
        chord = list(chord_)
        print 'Move %d by %f.' % (voice, interval)
        chord[voice] = chord[voice] + interval
        chord = tuple(self.bounceInside(chord))
        return chord
    '''
    Do a root progression by tranposition.
    '''
    def pT(self, chord, interval):
        chord = self.firstInversion(chord)
        print 'Transpose by %f.' % interval
        for i in xrange(3):
            chord[i] = chord[i] + interval
        chord = tuple(self.bounceInside(chord))
        return chord
    '''
    Perform the leading tone exchange neo-Riemannian transformation.
    '''
    def nrL(self, chord):
        print 'Leading-tone exchange transformation.'
        chord = self.firstInversion(chord)
        z1 = self.zeroFormFirstInversion(chord)
        if   z1[1] == 4.0:
            chord[0] = chord[0] - 1
        elif z1[1] == 3.0:
            chord[2] = chord[2] + 1
        chord = tuple(self.keepInside(chord))
        return chord
    '''
    Perform the parallel neo-Riemannian transformation.
    '''
    def nrP(self, chord):
        print 'Parallel transformation.'
        chord = self.firstInversion(chord)
        z1 = self.zeroFormFirstInversion(chord)
        if   z1[1] == 4.0:
            chord[1] = chord[1] - 1
        elif z1[1] == 3.0:
            chord[1] = chord[1] + 1
        chord = tuple(self.keepInside(chord))
        return chord
    '''
    Perform the relative neo-Riemannian transformation.
    '''
    def nrR(self, chord):
        print 'Relative transformation.'
        chord = self.firstInversion(chord)
        z1 = self.zeroFormFirstInversion(chord)
        if   z1[1] == 4.0:
            chord[2] = chord[2] + 2
        elif z1[1] == 3.0:
            chord[0] = chord[0] - 2
        chord = tuple(self.keepInside(chord))
        return chord
    '''
    Perform the dominant neo-Riemannian transformation.
    '''
    def nrD(self, chord):
        print 'Dominant transformation.'
        chord = self.firstInversion(chord)
        chord[0] = chord[0] - 7
        chord[1] = chord[1] - 7
        chord[2] = chord[2] - 7
        chord = tuple(self.keepInside(chord))
        return chord
    def tones(self, chord):
        c = array(chord, 'd').copy()
        for i in xrange(self.N):
           c[i] = c[i] % self.tonesPerOctave
        return self.sort(c)
    def zeroFormModulus(self, chord):
        c = array(chord, 'd').copy()
        for i in xrange(self.N):
           c[i] = c[i] % self.tonesPerOctave
        m = min(c)
        for i in xrange(self.N):
            c[i] = c[i] - m
        return c
    def zeroForm(self, chord):
        c = array(chord).copy()
        m = min(c[:self.N])
        for i in xrange(self.N):
            c[i] = c[i] - m
        return c
    def range(self, chord):
        c = chord[0:self.N]
        return max(c) - min(c)
        #c = self.sort(chord).copy()
        #return c[self.N-1] - c[0]
    def firstInversion(self, chord):
        inversions = self.rotations(chord)
        inversionDistances = {}
        origin = []
        for i in xrange(self.N):
            origin.append(0.)
        for inversion in inversions:
            zi = self.zeroForm(inversion)
            #z = float(sum(zi)) / float(self.N)
            d = self.euclidean(zi, origin)
            if self.debug:
                print 'distance %f zeroform %s inversion %s' % (d, zi, inversion)
            inversionDistances[d] = inversion
        return inversionDistances[min(inversionDistances.keys())]
    def zeroFormFirstInversion(self, chord):
        return self.zeroForm(self.firstInversion(chord))
    def equalTones(self, a, b):
        a = self.tones(a)
        b = self.tones(b)
        if a == b:
            return True
        else:
            return False
    def inversions_(self, tones, iterating_chord, voice, inversions):
        if voice >= self.N:
            return
        if self.isPrism:
            beginning = -self.getTessitura() * 2
            end = self.getTessitura() * 2
        elif self.isCube:
            beginning = -self.getTessitura()
            end = self.getTessitura()
        p = beginning
        increment = 1.0
        while p < end:
            if self.pitchclass(p) == tones[voice]:
                iterating_chord[voice] = p
                increment = self.tonesPerOctave
                si = self.sort(iterating_chord)
                if self.isInside(si, self.getTessitura()):
                    ic = tuple(si.tolist())
                    inversions.add(ic)
                self.inversions_(tones, iterating_chord, voice + 1, inversions)
            p = p + increment
    def inversions(self, chord):
        inversions = sets.Set()
        tones = self.tones(chord)
        iterating_chords = self.rotations(tones)
        for iterating_chord in iterating_chords:
            voice = 0
            self.inversions_(tones, iterating_chord, voice, inversions)
        l = list(inversions)
        for i in xrange(len(l)):
            l[i] = array(l[i])
        return l
    def euclidean(self, a, b):
        ss = 0.0
        for i in xrange(self.N):
            ss += ((a[i] - b[i]) ** 2.0)
        return math.sqrt(ss)
    def voiceleading(self, a, b):
        v = []
        for i in xrange(self.N):
            v.append(b[i] - a[i])
        return v
    def areParallel(self, a, b):
        return CsoundAC.areParallel(a,b)
##        if self.debug:
##            v = self.voiceleading(a, b)
##        for i in xrange(self.N):
##            if v.count(v[i]) > 1:
##                for j in xrange(self.N):
##                    if i != j:
##                        if (math.fabs(a[i] - a[j]) == 7) and (math.fabs(b[i] - b[j]) == 7):
##                            if self.debug:
##                                print a, b, v, 'parallel fifth'
##                            return True
##        return false
    def smoothness(self, a, b):
        L1 = 0.0
        for i in xrange(self.N):
            L1 += math.fabs(b[i] - a[i])
        return L1
    def smoother(self, source, destination1, destination2, avoidParallels=False):
        s1 = self.smoothness(source, destination1)
        s2 = self.smoothness(source, destination2)
        if avoidParallels:
            if self.areParallel(source, destination1):
                return destination2
            if self.areParallel(source, destination2):
                return destination1
        if s1 <= s2:
            return destination1
        else:
            return destination2
    def simpler(self, source, destination1, destination2, avoidParallels=False):
        v1 = self.voiceleading(source, destination1)
        v1 = sort(v1)
        v2 = self.voiceleading(source, destination2)
        v2 = sort(v2)
        for i in xrange(self.N - 1, -1, -1):
            if v1[i] < v2[i]:
                return destination1
            if v2[i] < v1[i]:
                return destination2
        return destination1
    def closer(self, source, destination1, destination2, avoidParallels=False):
        if avoidParallels:
            if self.areParallel(source, destination1):
                return destination2
            if self.areParallel(source, destination2):
                return destination1
        s1 = self.smoothness(source, destination1)
        s2 = self.smoothness(source, destination2)
        if s1 < s2:
            return destination1
        if s1 > s2:
            return destination2
        return self.simpler(source, destination1, destination2, avoidParallels)
    def closest(self, source, destinations, avoidParallels=False):
        d = destinations[0]
        for i in xrange(1, len(destinations)):
            d = self.closer(source, d, destinations[i], avoidParallels)
        return d
    def isFirstInversion(self, chord):
        return tuple(self.zeroForm(chord)) == tuple(self.zeroFormFirstInversion(chord))
    def rotate(self, a, n=1):
        l = a.tolist()
        for i in xrange(n):
            tail = l.pop(self.N - 1)
            l.insert(0, tail)
        return array(l, 'd')
    def invert(self, chord):
        chord = array(chord)
        c = chord[1:self.N].tolist()
        c.append(chord[0] + self.tonesPerOctave)
        d = chord.copy()
        d[0:self.N] = c
        return d
    def rotations(self, chord):
        chord = self.tones(chord)
        rotations = [chord]
        for i in xrange(1, self.N):
            #chord = self.rotate(chord, i)
            chord = self.invert(chord)
            rotations.append(chord)
        return rotations
    def isInside(self, chord, range):
        if self.isPrism:
            return self.isInFundamentalDomain(chord)
            #return self.isInsidePrism(chord, range)
        else:
            return self.isInsideCube(chord, range)
    def isInsideCube(self, chord, range):
        for i in xrange(self.N):
            if chord[i] < -range/2.0:
                return False
            if chord[i] >  range/2.0:
                return False
        return True
    def isInsideNormalPrism(self, chord, range):
        if not self.isInsidePrism(chord, range):
            return False
        if self.isFirstInversion(chord):
            return True
        return False
    def layer(self, chord):
        return sum(chord[0:self.N])
    def isInsidePrism(self, chord, range):
        if chord[0] < -range:
            return False
        elif chord[0] > range:
            return False
        for i in xrange(1, self.N):
            if chord[i] > chord[0] + range:
                return False
            elif chord[i] < chord[0]:
                return False
        s = sum(chord[0:self.N])
        if 0 <= s and s <= range:
            return True
        else:
            return False
    def isInFundamentalDomain(self, chord):
        if self.isInLayer(chord) and self.isInOrder(chord):
            if self.debug:
                print 'Chord',chord,'in F'
            return True
        else:
            if self.debug:
                print 'Chord',chord,'not in F'
            return False
    def isInLayer(self, chord):
        L = self.layer(chord)
        if not (0 <= L and L <= self.R):
            return False
        return True
    def isInOrder(self, chord):
        for i in xrange(self.N - 1):
            if not chord[i] <= chord[i + 1]:
                return False
        if not chord[self.N - 1] <= (chord[0] + self.R):
            return False
        return True
    def O(self, c):
        if self.debug:
            print "O: ",c,
        r = []
        for i in xrange(1, self.N):
            r.append(c[i] - (self.R / self.N))
        r.append(c[0] + (self.R - (self.R / self.N)))
        c[0:self.N] = r
        if self.debug:
            print c
        return c
    def bounceInside(self, chord):
        inversions = self.inversions(chord)
        if self.debug:
            print inversions
        for inversion in inversions:
            if tuple(inversion) in self.trichords:
                return inversion
            return None
    def keepInside(self, chord):
        if self.isInFundamentalDomain(chord):
            return chord
        else:
            inversions = self.inversions(chord)
            if self.debug:
                print inversions
            for inversion in inversions:
                if self.isInOrder(inversion):
                    c = list(inversion)
                    for i in xrange(self.N):
                        if self.isInLayer(c):
                            return array(c)
                        c = self.O(c)
            return None
    def stayInside(self, chord):
        if self.isInside(chord, self.getTessitura()):
            return chord
        chord = self.sort(chord)
        if self.isPrism:
            inversions = self.inversions(chord)
            if self.debug:
                print 'inversions',inversions
            distances = {}
            for inversion in inversions:
                distances[self.euclidean(chord, inversion)] = inversion
            c = distances[max(distances.keys())]
            if self.debug:
                print 'keepInside:', 't =',self.getTessitura(), 'original =',chord, 'inside =',c
            return c
        else:
            c = array(chord)
            for i in xrange(self.N):
                while c[i] <  -self.getTessitura()/2:
                    c[i] += self.getTessitura()
                while c[i] >= self.getTessitura()/2:
                    c[i] -= self.getTessitura()
            c = self.sort(c)
            if self.debug:
                print chord,'keeps inside as',c
            return c
    def pitchclasses(self, chord):
        c = array(chord, 'd').copy()
        for i in xrange(self.N):
            c[i] = self.pitchclass(chord[i])
        return c
    def pitchclass(self, pitch):
        return pitch % self.tonesPerOctave
    '''
    Returns the best bijective voice-leading,
    first by smoothness then by parsimony,
    optionally avoiding parallel fifths,
    from a given source chord of pitches
    to a new chord of pitches
    that belong to the pitch-class set of a target chord,
    and lie within a specified range.
    The algorithm makes an exhaustive search
    of potential target chords in the space.
    '''
    def voicelead(self, a, b, avoidParallels):
        if self.debug:
            print '   From:', a
            print '     To:', b
            print 'Through:'
        invs = self.inversions(b)
        if self.debug:
            for inv in invs:
                print '        ',inv
            c = self.closest(a, invs, avoidParallels)
        if self.debug:
            print '(%d inversions) is:' % len(invs)
            print '        ', c
            print 'Leading:', self.voiceleading(a,c)
        return c
    def label(self, chord):
        c = array(chord[0:self.N])
        return 'C   %s\nT   %s\n0   %s\n1   %s\n0-1 %s\nSum %f' % (c, self.tones(c), self.zeroForm(c), self.firstInversion(c), self.zeroFormFirstInversion(chord), sum(chord[0:self.N]))

class ChordSpace3Model(ChordSpace3):
    def __init__(self, octaveCount=1, tonesPerOctave=12, isCube=False, isPrism=True, isNormalPrism=False, doCycle=False, showFirstInversion=False, doConnect=False, enableCsound=False, debug=False, showUnordered=False):
        ChordSpace3.__init__(self, 3, octaveCount=octaveCount, tonesPerOctave=tonesPerOctave, isCube=isCube, isPrism=isPrism, isNormalPrism=isNormalPrism, debug=debug)
        self.trichords = {}
        self.balls = {}
        self.ballsForChordTypes = {}
        self.doConnect = doConnect
        self.doCycle = doCycle
        self.showUnordered = showUnordered
        self.showFirstInversion = showFirstInversion
        self.firstInversions = []
        self.enableCsound = enableCsound
        if self.enableCsound:
            self.csound = csnd6.CppSound()
        if self.isCube:
            for x in xrange(-self.cubeTessitura/2, self.cubeTessitura/2):
                for y in xrange(-self.cubeTessitura/2, self.cubeTessitura/2):
                    for z in xrange(-self.cubeTessitura/2, self.cubeTessitura/2):
                        trichord = (x,y,z)
                        radius = 0.125
                        if trichord not in self.trichords:
                            self.trichords[trichord] = trichord
                            ball = sphere(pos = trichord, radius = self.cubeRadius)
                            ball.trichord = trichord
                            self.balls[ball.trichord] = ball
                            self.setColor(ball)
                            ball.name = self.label(trichord)
                            ball.label = label(pos = trichord, text = ball.name, height = 11, box = 2, opacity = 0.3, linecolor=(0.9,0.5,0.9), visible = 0, line = 2, xoffset = 20, yoffset = 20)
        if self.isPrism or self.isNormalPrism:
            for x in xrange(-self.R, self.R+1):
                for y in xrange(x, x + self.R+1):
                    for z in xrange(x, x + self.R+1):
                        trichord = array((x,y,z), 'd')
                        trichord = tuple(self.sort(trichord))
                        if self.isPrism and self.isInsidePrism(trichord, self.R):
                            if trichord not in self.trichords:
                                self.trichords[trichord] = trichord
                                tones = tuple(self.tones(trichord))
                                ball = sphere(pos = trichord, radius = self.prismRadius)
                                ball.trichord = trichord
                                self.balls[ball.trichord] = ball
                                self.setColor(ball)
                                ball.name = self.label(trichord)
                                ball.label = label(pos = trichord, text = ball.name, height = 11, box = 2, opacity = 0.3, linecolor=(0.9,0.5,0.9), visible = 0, line = 2, xoffset = 20, yoffset = 20)
                            else:
                                self.balls[trichord].radius = self.prismRadius
                        if self.isNormalPrism and self.isInsideNormalPrism(trichord, self.R):
                            if trichord not in self.trichords:
                                self.trichords[trichord] = trichord
                                tones = tuple(self.tones(trichord))
                                ball = sphere(pos = trichord, radius = self.normalPrismRadius)
                                ball.trichord = trichord
                                self.balls[ball.trichord] = ball
                                self.setColor(ball)
                                ball.name = self.label(trichord)
                                ball.label = label(pos = trichord,  text = ball.name, height = 11, box = 2, opacity = 0.3, linecolor=(0.5,0.5,0.5), visible = 0, line = 2, xoffset = -20, yoffset = 20)

                            else:
                                self.balls[trichord].radius = self.normalPrismRadius
        if self.doConnect:
            for trichord in self.trichords.values():
                self.connect(trichord, self.sort((trichord[0] + 1.0, trichord[1], trichord[2])))
                self.connect(trichord, self.sort((trichord[0], trichord[1] + 1.0, trichord[2])))
                self.connect(trichord, self.sort((trichord[0], trichord[1], trichord[2] + 1.0)))
                self.connect(trichord, self.sort((trichord[0] - 1.0, trichord[1], trichord[2])))
                self.connect(trichord, self.sort((trichord[0], trichord[1] - 1.0, trichord[2])))
                self.connect(trichord, self.sort((trichord[0], trichord[1], trichord[2] - 1.0)))
    def setColor(self, ball):
        z = tuple(self.zeroFormFirstInversion(ball.trichord))
        if z in self.ballsForChordTypes:
            ball.color = self.ballsForChordTypes[z].color
        else:
            # Color major triads red.
            if   z == (0, 4, 7):
                ball.color = (1.0,0.0,0.0)
            # Color augmented triads white.
            elif z == (0, 4, 8):
                ball.color = (1.0,1.0,1.0)
            # Color minor triads blue.
            elif z == (0, 3, 7):
                ball.color = (0.67,0.67,1.0)
            else:
                hue = (z[0] + z[1] * 2.0 + z[2]) / 44.0
                saturation = 1.0
                value = 1.0
                ball.color = color.hsv_to_rgb((hue, saturation, value))
    def showAsFirstInversion(self, trichord):
        if not self.showFirstInversion:
            return False
        elif self.isFirstInversion(trichord):
            return True
        else:
            return False
    def connect(self, origin, neighbor):
        o = tuple(origin)
        n = tuple(neighbor)
        if n in self.trichords:
            curve(pos = [o, n], color = (0.65, 0.65, 0.65), radius = 0.020)
    def runGrab(self, filename, bbox=None):
        while scene.visible:
            if scene.mouse.clicked:
                print 'CURRENT POINT:'
                print 'center =',scene.center
                print 'forward =',scene.forward
                print 'up =',scene.up
                print 'scale =',scene.scale
                print 'fov = ',scene.fov
                print
                try:
                    if bbox:
                        pass #image = ImageGrab.grab(bbox)
                    else:
                        pass #image = ImageGrab.grab()
                    #image = ImageOps.grayscale(image)
                    #image.save(filename)
                    #print 'Captured screen shot in "%s".' % (filename)
                except:
                    traceback.print_exc()
                scene.mouse.events = 0
    def playBall(self, pickedBall):
        pickedBall.label.visible = 1
        print pickedBall.name
        note1 = "i 1 0 4 %d 70 0 -.75" % (60 + pickedBall.pos[0])
        note2 = "i 2 0 4 %d 70 0  .0"  % (60 + pickedBall.pos[1])
        note3 = "i 3 0 4 %d 70 0  .75" % (60 + pickedBall.pos[2])
        print '%s\n%s\n%s' % (note1, note2, note3)
        if self.enableCsound:
            self.csound.inputMessage(note1)
            self.csound.inputMessage(note2)
            self.csound.inputMessage(note3)
        print

    def run(self):
        pickedBall = None
        oldBall = None
        movingChord = ( 0, 4, 7)
        translation = (1,1,1)
        while scene.visible:
            rate(100)
            movingChord = tuple(self.sort(movingChord))
            if scene.kb.keys:
                k = scene.kb.getkey()
                print 'key: %s' % k
                if   k == 'up':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 0,  1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'right':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 1,  1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'down':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 2,  1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'shift+up':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 0, -1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'shift+right':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 1, -1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'shift+down':
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.move(movingChord, 2, -1.0)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                if k in ('p', 'P'):
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.nrP(movingChord)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k in ('l', 'L'):
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.nrL(movingChord)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k in ('r', 'R'):
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.nrR(movingChord)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k in ('d', 'D'):
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    movingChord = self.nrD(movingChord)
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k in ('1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b'):
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 0
                    if k == 'a':
                        k = 10
                    if k == 'b':
                        k = 11
                    movingChord = self.pT(movingChord, float(k))
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible = 1
                    self.playBall(movingBall)
                    oldBall = movingBall
                elif k == 'g':
                    self.runGrab("orbifold.png")
                elif k in ('x', 'X', 'q', 'Q'):
                    sys.exit()
            if scene.mouse.clicked:
                try:
                    m = scene.mouse.getclick()
                    if oldBall:
                        oldBall.label.visible = 0
                    if pickedBall:
                        pickedBall.label.visible = 0
                    oldBall = pickedBall
                    pickedBall = m.pick
                    if pickedBall:
                        movingBall = pickedBall
                        movingChord = tuple(movingBall.pos)
                        self.playBall(pickedBall)
                except:
                    traceback.print_exc()
                    print self.label(movingChord)
                scene.mouse.events = 0
            elif self.doCycle:
                try:
                    movingBall = self.balls[movingChord]
                    movingBall.label.visible=1
                    print movingBall.name
                    time.sleep(2)
                    movingBall.label.visible=0
                    a = (movingChord[0], movingChord[1], movingChord[2])
                    print 'Old chord',a
                    movingChord = (movingChord[0] + translation[0], movingChord[1] + translation[1], movingChord[2] + translation[2])
                    print 'New chord',movingChord
                    #movingChord = self.voiceLead(a, b, True)
                    movingChord = tuple(self.keepInside(movingChord))
                    self.playBall(movingChord)
                except:
                    traceback.print_exc()
                    print self.label(movingChord)
                    return
        print "Finished."

def runModel(model):
    began = time.clock()
    scene.background = (1,1,1)
    scene.background = (0,0,0)
    scene.autocenter = 1
    sort(model.firstInversions)
    if model.enableCsound:
        model.csound.setOrchestra('''
sr                              =                       44100
ksmps                           =                       100
nchnls                          =                       2

                                connect                 "FenderRhodesModel",    "out", "Reverberation", "in"
                                connect                 "FMWaterBell",          "out", "Reverberation", "in"
                                connect                 "Xing",                 "out", "Reverberation", "in"
                                connect                 "Reverberation",        "out", "MasterOutput", "in"

                                alwayson                "Reverberation"
                                alwayson                "MasterOutput"

gkFenderRhodesModelGain         init                    0
gkFenderRhodesModelGainCorrection init                  60
                                instr                   FenderRhodesModel
                                //////////////////////////////////////////////////////
                                // Original by Perry Cook.
                                // Adapted by Michael Gogins.
                                //////////////////////////////////////////////////////
i_instrument                    =                       p1
i_time                          =                       p2
i_duration                      =                       p3
i_midikey                       =                       p4
i_midivelocity                  =                       p5
i_phase                         =                       p6
i_pan                           =                       p7
i_depth                         =                       p8
i_height                        =                       p9
i_pitchclassset                 =                       p10
i_homogeneity                   =                       p11
iattack                         =                       0.01
isustain                        =                       p3
irelease                        =                       0.125
p3                              =                       isustain + iattack + irelease
adeclick                        linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iindex                          =                       4
icrossfade                      =                       3
ivibedepth                      =                       0.2
iviberate                       =                       6
isine                           ftgenonce               0, 0, 65536,    10,     1
icosine                         ftgenonce               0, 0, 65536,    11,     1
icookblank                      ftgenonce               0, 0, 65536,     10,     0 ; Blank wavetable for some Cook FM opcodes.
ifn1                            =                       isine
ifn2                            =                       icosine
ifn3                            =                       isine
ifn4                            =                       icookblank
ivibefn                         =                       isine
ifrequency                      =                       cpsmidinn(i_midikey)
asignal                         fmrhode                 1, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
kdbgain                         =                       gkFenderRhodesModelGainCorrection + (gkFenderRhodesModelGain - 0.5) * 100.0
kiamplitude                     =                       ampdb(i_midivelocity) * pow(10, kdbgain / 20)
aoutleft, aoutright		        pan2			        asignal * adeclick * kiamplitude, i_pan
aout[]                          init                    2
aout[0]                         =                       aoutleft
aout[1]                         =                       aoutright
                                outletv                 "out",  aout
                                endin

gkFMWaterBellGain               init                    0.0
gkFMWaterBellGainCorrection     init                    50.0
                                instr                   FMWaterBell
                                //////////////////////////////////////////////
                                // Original by Steven Yi.
                                // Adapted by Michael Gogins.
                                //////////////////////////////////////////////
i_instrument                    =                       p1
i_time                          =                       p2
i_duration                      =                       p3
i_midikey                       =                       p4
i_midivelocity                  =                       p5
i_phase                         =                       p6
i_pan                           =                       p7
i_depth                         =                       p8
i_height                        =                       p9
i_pitchclassset                 =                       p10
i_homogeneity                   =                       p11
ipch                            =                       cpsmidinn(i_midikey)
iamplitude                      =                       ampdb(i_midivelocity)
ipch2                           =                       ipch
kpchline 	                    line                    ipch, i_duration, ipch2
iamp 	                        =                       2
ienvType	                    =                       2
kenv 	                        init 	                0
                                if ienvType == 0 kgoto env0  ; adsr
                                if ienvType == 1 kgoto env1  ; pyramid
                                if ienvType == 2 kgoto env2  ; ramp
env0:
kenv	                        adsr	                .3, .2, .9, .5
                                kgoto                   endEnvelope
env1:
kenv 	                        linseg	                0, i_duration * .5, 1, i_duration * .5, 0
                                kgoto                   endEnvelope
env2:
kenv	                        linseg 	                0, i_duration - .1, 1, .1, 0
kgoto                           endEnvelope
endEnvelope:
kc1                             =                       5
kc2                             =                       5
kvdepth                         =                       0.005
kvrate                          =                       6
icosine                  	    ftgenonce               0, 0, 65536, 11, 1
ifn1                            =                       icosine
ifn2                            =                       icosine
ifn3                            =                       icosine
ifn4                            =                       icosine
ivfn                            =                       icosine
asignal                         fmbell	                iamp, kpchline, kc1, kc2, kvdepth, kvrate, ifn1, ifn2, ifn3, ifn4, ivfn
iattack                         =                       0.003
isustain                        =                       p3
irelease                        =                       0.06
p3                              =                       isustain + iattack + irelease
adeclick                        linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
kdbgain                         =                       gkFMWaterBellGainCorrection + (gkFMWaterBellGain - 0.5) * 100.0
kiamplitude                     =                       ampdb(i_midivelocity) * pow(10, kdbgain / 20)
aoutleft, aoutright		        pan2			        asignal * adeclick * kiamplitude, i_pan
aout[] init 2
aout[0] = aoutleft
aout[1] = aoutright
                                outletv                 "out",  aout
                                endin

gkXingGain                      init                    0.0
gkXingGainCorrection            init                    60.0
                                instr                   Xing
                                //////////////////////////////////////////////
                                // Original by Andrew Horner.
                                // Adapted by Michael Gogins.
                                // p4 pitch in octave.pch
                                // original pitch        = A6
                                // range                 = C6 - C7
                                // extended range        = F4 - C7
                                //////////////////////////////////////////////
insno           		        =                       p1
itime           		        =                       p2
iduration       		        =                       p3
ikey            		        =                       p4
ivelocity                       =                       p5
iphase                          =                       p6
ipan                            =                       p7
idepth                          =                       p8
iheight                         =                       p9
ipcs                            =                       p10
ihomogeneity                    =                       p11
kgain			    	        =                       1.25
iHz                             =                       cpsmidinn(ikey)
kHz                             =                       k(iHz)
iattack                         =                       (440.0 / iHz) * 0.01
                                print                   iHz, iattack
isustain                        =                       p3
irelease                        =                       .3
p3                              =                       iattack + isustain + irelease
iduration                       =                       p3
iamplitude                      =                       ampdb(ivelocity).
isine                           ftgenonce               0, 0, 65536,    10,     1
kfreq                           =                       cpsmidinn(ikey)
iamp                            =                       1
inorm                           =                       32310
aamp1                           linseg                  0,.001,5200,.001,800,.001,3000,.0025,1100,.002,2800,.0015,1500,.001,2100,.011,1600,.03,1400,.95,700,1,320,1,180,1,90,1,40,1,20,1,12,1,6,1,3,1,0,1,0
adevamp1                        linseg                  0, .05, .3, iduration - .05, 0
adev1                           poscil                  adevamp1, 6.7, isine, .8
amp1                            =                       aamp1 * (1 + adev1)
aamp2                           linseg                  0,.0009,22000,.0005,7300,.0009,11000,.0004,5500,.0006,15000,.0004,5500,.0008,2200,.055,7300,.02,8500,.38,5000,.5,300,.5,73,.5,5.,5,0,1,1
adevamp2                        linseg                  0,.12,.5,iduration-.12,0
adev2                           poscil                  adevamp2, 10.5, isine, 0
amp2                            =                       aamp2 * (1 + adev2)
aamp3                           linseg                  0,.001,3000,.001,1000,.0017,12000,.0013,3700,.001,12500,.0018,3000,.0012,1200,.001,1400,.0017,6000,.0023,200,.001,3000,.001,1200,.0015,8000,.001,1800,.0015,6000,.08,1200,.2,200,.2,40,.2,10,.4,0,1,0
adevamp3                        linseg                  0, .02, .8, iduration - .02, 0
adev3                           poscil                  adevamp3, 70, isine ,0
amp3                            =                       aamp3 * (1 + adev3)
awt1                            poscil                  amp1, kfreq, isine
awt2                            poscil                  amp2, 2.7 * kfreq, isine
awt3                            poscil                  amp3, 4.95 * kfreq, isine
asig                            =                       awt1 + awt2 + awt3
arel                            linenr                  1,0, iduration, .06
; asignal                         =                       asig * arel * (iamp / inorm) * iamplitude * kgain
asignal                         =                       asig * (iamp / inorm) * kgain
adeclick                        linsegr                 0, iattack, 1, isustain, 1, irelease, 0
kdbgain                         =                       gkXingGainCorrection + (gkXingGain - 0.5) * 100.0
kiamplitude                     =                       ampdb(ivelocity) * pow(10, kdbgain / 20)
aoutleft, aoutright		        pan2			        asignal * adeclick * kiamplitude, ipan
aout[] init 2
aout[0] = aoutleft
aout[1] = aoutright
                                outletv                 "out",  aout
                                endin


gkReverberationEnabled          init                    1
gkReverberationDelay            init                    0.4
gkReverberationWet          	init                    0.25
                                instr                   Reverberation
                                //////////////////////////////////////////////
                                // By Michael Gogins.
                                //////////////////////////////////////////////
ain[]                           init 2
ain                             inletv                  "in"
ainleft                         =                       ain[0]
ainright                        =                       ain[1]
if (gkReverberationEnabled == 1) then
aoutleft                        =                       ainleft
aoutright                       =                       ainright
kdry				            =			            1.0 - gkReverberationWet
else
awetleft, awetright             reverbsc                ainleft, ainright, gkReverberationDelay, 18000.0
aoutleft			            =			            ainleft *  kdry + awetleft  * gkReverberationWet
aoutright			            =			            ainright * kdry + awetright * gkReverberationWet
endif
aout[]                          init                    2
aout[0]                         =                       aoutleft
aout[1]                         =                       aoutright
                                outletv                 "out", aout
                                endin

gkMasterLevel                   init                    1.0
                                instr                   MasterOutput
                                //////////////////////////////////////////////
                                // By Michael Gogins.
                                //////////////////////////////////////////////
ain[]                           init 2
ain                             inletv                  "in"
ain[0]                          =                       gkMasterLevel * ain[0]
ain[1]                          =                       gkMasterLevel * ain[1]
                                out                     ain
                                endin

            ''')
        model.csound.setScore('''
            f1 0 8192 10 1
            f0 6000
            e
            ''')
        model.csound.setCommand('csound -h -d -r 48000 -k 1000 -m227 -b1000 -B1000 -odac')
        #gc.disable()
        model.csound.compile()
        performanceThread = csnd6.CsoundPerformanceThread(model.csound)
        performanceThread.Play()
    fg = (1,1,1)
    arrowcolor = (0.7,0.7,0.7)
    size = model.getTessitura() * 1.125
    shaftwidth = model.cubeRadius * 1.0
    arrow(pos = (0,0,0), axis=(size/3,0,0), fixedwidth=1, shaftwidth=shaftwidth, color = arrowcolor)
    label(pos = (size/3,0,0), text = 'Voice 1', color=fg, height = 20, box = 0, linecolor=(0.5,0.5,0.5), opacity = 0.1, visible = 1, line = 0, xoffset =  5, yoffset =  5, zoffset = 5)
    arrow(pos = (0,0,0), axis=(0,size/1.5,0), fixedwidth=1, shaftwidth=shaftwidth, color = arrowcolor)
    label(pos = (0,size/1.5,0), text = 'Voice 2', color=fg, height = 20, box = 0, linecolor=(0.5,0.5,0.5), opacity = 0.1, visible = 1, line = 0, xoffset =   5, yoffset = 5, zoffset = 5)
    arrow(pos = (0,0,0), axis=(0,0,size), fixedwidth=1, shaftwidth=shaftwidth, color = arrowcolor)
    label(pos = (0,0,size), text = 'Voice 3', color=fg, height = 20, box = 0, linecolor=(0.5,0.5,0.5), opacity = 0.1, visible = 1, line = 10, xoffset = 5, yoffset = 5, zoffset = 5)
    arrow(pos = (0,0,0), axis=(size/2.5,size/2.5,size/2.5), fixedwidth=1, shaftwidth=shaftwidth, color=arrowcolor)
    label(pos = (size/2.5,size/2.5,size/2.5), text = 'Orthogonal axis', color=fg, height = 20, box = 0, linecolor=(0.5,0.5,0.5), opacity = 0.1, visible = 1, line = 0, xoffset = 5, yoffset = 5, zoffset = 5)
    ended = time.clock()
    elapsed = ended - began
    print 'elapsed: %f' % (elapsed)
    model.run()
    print 'Visual finished.'
    if model.enableCsound:
        performanceThread.Stop()
        print 'Csound finished.'


if __name__ == '__main__':
    #scene.fullscreen = False
    #scene.width = 300 * 7
    #scene.height = 300 * 5
    # ChordSpace3 for trichords
    model = ChordSpace3Model(octaveCount=1, doCycle=False, doConnect=True, isPrism=True, enableCsound=True)
    # Ranged chord space
    #model = ChordSpace3Model(octaveCount=2, doCycle=False, doConnect=False, isCube=True, isPrism=False)
    # ChordSpace3 in ranged chord space
    #model = ChordSpace3Model(octaveCount=1, doCycle=True, doConnect=False, isPrism=True, isCube=True)
    # Voice-leading space
    #model = ChordSpace3Model(octaveCount=3, doCycle=True, doConnect=False, isPrism=True, isNormalPrism=False)
    # Normal chord space
    #model = ChordSpace3Model(octaveCount=3, doCycle=False, doConnect=False, isPrism=False, isNormalPrism=True)
    # Normal chord space in voice-leading space
    #model = ChordSpace3Model(octaveCount=3, doCycle=False, doConnect=False, isPrism=True, isNormalPrism=True)
    runModel(model)
    print 'Program finished.'

