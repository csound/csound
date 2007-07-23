import sets
import math
import string
import copy
import psyco
import sys
from numpy import *
import CsoundAC

chords = {}
numbersForChords = {}
primechords = {}
numbersForPrimeChords = {}

'''
apply tones to score segment
apply score segment to tones
apply voicing to score segment
apply score segment to voicing
work with indexed segments of scores
index unordered chords: i * 2 + 1 transposes a chord one semitone
index prime chords: i * 2 + 1 mutates a chord to the next least compact form
'''

def chordnumber(chord):
    pcs = sets.Set()
    for v in chord:
        pcs.add(float(v) % 12.0)
    cn = 0.0
    for pc in pcs:
        cn = cn + pow(2.0, pc)
    return cn

def numberchord(number, tonesPerOctave = 12):
    chord = []
    powerOf2 = int(1)
    for i in xrange(tonesPerOctave):
        if (powerOf2 & int(number)) == powerOf2:
            chord.append(int(i))
        powerOf2 = powerOf2 + powerOf2
    return array(chord)

def pitchClass(p, tonesPerOctave = 12):
    pitch = int(p + 0.5)
    pc = pitch % tonesPerOctave
    return pc

def ascendingDistance(p1, p2, tonesPerOctave = 12):
    pc1 = pitchClass(p1)
    pc2 = pitchClass(p2)
    d = pc2 - pc1
    if d < 0:
        d = d + tonesPerOctave
    return d
            
class SortByAscendingDistanceComparator(object):
    def __init__(self, chord):
        self.root = chord[0]
    def __call__(self, p1, p2):
        d1 = ascendingDistance(self.root, p1)
        d2 = ascendingDistance(self.root, p2)
        if d1 < d2:
            return -1
        elif d1 > d2:
            return 1
        else:
            return 0

def euclidean(a, b):
    ss = 0.0
    for i in xrange(a.shape[0]):
        ss += ((a[i] - b[i]) ** 2.0)
    return math.sqrt(ss)

def distanceFromOrigin(chord):
    origin = zeros((12), 'f')
    c = zeros((12), 'f')
    c[0:chord.shape[0]] = chord
    return euclidean(origin, c)
        
def sortByAscendingDistance(chord):
    c = list(copy.copy(chord))
    c.append(c[0])
    c.sort(SortByAscendingDistanceComparator(c))
    return c

def displacementMultiset(c1, d2):
    dm = zeros(len(c1), Float64)
    for i in xrange(len(c1)):
        dm[i] = fabs(c2 - c1)
    return dm
            
def voiceLeadingSize(c1, c2):
    size = 0
    for i in xrange(len(c1)):
        size = size + fabs(c2[i] - c2[i])
    return size
        
for i in xrange(60):
    print 'p %2d  pc %2d' % (i, pitchClass(i))

for i in xrange(12):
    p1 = 6
    p2 = i
    ad = ascendingDistance(p1, p2)
    print 'p1 %2d  p2 %2d  ascending distance %2d' % (p1, p2, ad)
    
Cmajor = array([0, 2, 4, 5, 7, 9, 11])
cn = chordnumber([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11])
print cn
print numberchord(cn)
print chordnumber(Cmajor)
C = [0, 4, 7]
print chordnumber(C)
D = []
for v in C:
    D.append(v + 2)
print D 
print chordnumber(D)
for i in xrange(12):
    print 'pc %3d = %4d' % (i, chordnumber([i]))

C = 0.0
for i in xrange(1, int(pow(2, 12))):
    M = float(i)
    C = M - 1.0
    C1 = CsoundAC.Voicelead_mToC(M, 12)
    if C != C1:
        print 'Error: mToC is wrong: M: %s  C: %s %s' % (M, C, C1)
    c = numberchord(M)
    chord = tuple(CsoundAC.Voicelead_mToPitchClassSet(M, 12))
    if tuple(c) != chord:
        print 'Error: mToPitchClassSet is wrong.'
    name = CsoundAC.Conversions_mToName(M)
    if name == 'Not found.':
        name = ''
    normal = CsoundAC.Voicelead_normalChord(c)
    zero = CsoundAC.Voicelead_toOrigin(normal)
    prime = CsoundAC.Voicelead_primeChord(c)
    if tuple(zero) == tuple(normal):
        if tuple(prime) != tuple(zero):
	    print 'Error: primeChord is wrong.'
    P = CsoundAC.Voicelead_cToP(C, 12)
    distance = distanceFromOrigin(c)
    print 'M: %4d  %-63s  C: %4d  %-68s  P: %4d  %-63s  distance: %6.3f  %s' % (int(M), chord, int(C), normal, int(P), prime, distance, name)
print
    
print CsoundAC.Voicelead_pitchClassSetToM(tuple(sort([0,1,10,7,5])))
print (pow(2, 12) -1) % (pow(2,12)-1)

print 'Unordered voicings of CM in 3 octaves:'
voicings = CsoundAC.Voicelead_voicings([0, 4, 7], 0.0, 37.0, 12)
for voicing in voicings:
    print voicing
print len(voicings)

#print 'Unordered voicings of 12 TET in 4 octaves:'
#voicings = CsoundAC.Voicelead_voicings([0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11.], 0.0, 49.0, 12)
#for voicing in voicings:
#    print voicing
#print len(voicings)









