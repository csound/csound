import CsoundVST
import math
import random
import string
import sys
sys.path.append('d:/utah/home/mkg/projects/icmc2006-mkg')
import tonnetz

print 'UNIT TESTS FOR CsoundVST.Voicelead and CsoundVST.Score'
print

lowest = 48
range = 36
highest = lowest + range

for i in xrange(36, 96, 1):
    p = float(i)
    print '%8.3f = Voicelead_pc(%8.3f)' % (CsoundVST.Voicelead_pc(p), p)
print
a = [0, 4, 7, 11]
b = [1, 5, 8,  0]
print '%s = Voicelead_voiceleading(%s, %s)' % (CsoundVST.Voicelead_voiceleading(a, b), a, b)
print
c = [5, 5, 5, 5]
print '%s = Voicelead_areParallel(%s, %s)' % (CsoundVST.Voicelead_areParallel(a, b), a, b)
print '%s = Voicelead_areParallel(%s, %s)' % (CsoundVST.Voicelead_areParallel(a, c), a, c)
print
print '%s = Voicelead_rotate(%s)' % (CsoundVST.Voicelead_rotate(a), a)
print
rotations = CsoundVST.Voicelead_rotations(a)
print '%s = CsoundVST.Voicelead_rotations(%s)' % (a, rotations)
for chord in rotations:
    print chord
print
pitches = [65., 69., 72., 76.]
print '%s = Voicelead_pcs(%s)' % (CsoundVST.Voicelead_pcs(pitches), pitches)
print '%8.3f = Voiclead_mFromPitchClassSet(%s)' % (CsoundVST.Voicelead_mFromPitchClassSet(pitches), pitches)
print '%8.3f = Voiclead_mFromPitchClassSet(%s)' % (CsoundVST.Voicelead_mFromPitchClassSet(a), a)
print
print '%s = Voicelead_pitchClassSetFromM(%8.3f)' % (CsoundVST.Voicelead_pitchClassSetFromM(2193), 2193)
print
voicings = CsoundVST.Voicelead_voicings(a, lowest, range, 12)
print 'Voicelead_voicings(%s, %s, %s, %d):' % (a, lowest, range, 12)
for chord in voicings:
    print chord
print
CM7 = [60., 64., 67., 71.]
print 'CM7: %s' % (CM7)
FM7 = [65., 69., 72., 76.]
print 'FM7: %s' % (FM7)
G7 =  [67., 71., 74., 77.]
print 'G7:  %s' % (G7)
t1 = CsoundVST.Voicelead_voicelead(CM7, FM7, lowest, range, False)
print '%s = CsoundVST.Voicelead_voicelead(%s, %s, lowest, range, True)' % (t1, CM7, FM7)
t2 = CsoundVST.Voicelead_voicelead(t1, G7, lowest, range, False)
print '%s = CsoundVST.Voicelead_voicelead(%s, %s, lowest, range, True)' % (t2, t1, G7)
print
t1 = CsoundVST.Voicelead_recursiveVoicelead(CM7, FM7, lowest, range, False)
print '%s = CsoundVST.Voicelead_recursiveVoicelead(%s, %s, lowest, range, True)' % (t1, CM7, FM7)
t2 = CsoundVST.Voicelead_recursiveVoicelead(t1, G7, lowest, range, False)
print '%s = CsoundVST.Voicelead_recursiveVoicelead(%s, %s, lowest, range, True)' % (t2, t1, G7)
print
print 'Inversions of', a
vinversions = CsoundVST.Voicelead_inversions(a)
tonnetz = tonnetz.Tonnetz(4)
tinversions = tonnetz.rotations(a)
for i in xrange(len(vinversions)):
	vinversion = vinversions[i]
	tinversion = tinversions[i]
	print 'inversion of %s:           V: %s  T: %s' % (a, vinversion, tinversion)
	print '  origin chord of inversion:          V: %s  T: %s' % (CsoundVST.Voicelead_toOrigin(vinversion), tonnetz.zeroForm(tinversion))
	print '  normal chord of inversion:          V: %s  T: %s' % (CsoundVST.Voicelead_normalChord(vinversion), tonnetz.firstInversion(tinversion))
	print '  zero chord of inversion:            V: %s  T: %s' % (CsoundVST.Voicelead_primeChord(vinversion), tonnetz.zeroForm(tonnetz.firstInversion(tinversion)))
	print ' '
print
for i in xrange(lowest, range, 1):
	print '%8.3f = Voiclead_closestPitch(Voicelead_pc(%d), %s)' %(CsoundVST.Voicelead_closestPitch(i, CM7), i, CM7)
print
for i in xrange(lowest, range, 1):
	print '%8.3f = Voiclead_conformToPitchClassSet(%d, %s)' %(CsoundVST.Voicelead_conformToPitchClassSet(i, a), i, a)
print
score = CsoundVST.Score()
for i in xrange(2000):
	time = i * 0.125
	duration = 0.5
	key = random.randint(lowest, highest)
	velocity = 80.0
	score.append(time, duration, 144.0, 1.0, key, velocity)
score.save('CsoundVstUnitTest.py.1.mid')
score.setPitchClassSet(0, len(score), a)
score.save('CsoundVstUnitTest.py.2.mid')
score = CsoundVST.Score()
for i in xrange(2000):
	time = i * 0.125
	duration = 0.5
	key = random.randint(lowest, highest)
	velocity = 80.0
	score.append(time, duration, 144.0, 1.0, key, velocity)
C = CsoundVST.Voicelead_mToC(CsoundVST.Voicelead_mFromPitchClassSet([0.,4.,7.,11.,14.]), 12)
for i in xrange(0, len(score), 20):
	M = CsoundVST.Voicelead_cToM(C)
	pcs = CsoundVST.Voicelead_pitchClassSetFromM(M)
	print 'M: %8.3f  C: %8.3f  %s' % (M, C, pcs)
	score.setPitchClassSet(i, i + 20, pcs)
	result = CsoundVST.Voicelead_uniquePcs(score.getPitches(i, i + 20))
	print 'Result:',result
	print ' '
	C = 2 * C + 1
	C = C % 4094
score.save('CsoundVstUnitTest.py.3.mid')
for i in xrange(0, len(score) - 40, 20):
	prepitches = score.getPitches(i, i + 20)
	score.voicelead(i, i + 20, i + 20, i + 40, lowest, highest - lowest, True)
	postpitches = score.getPitches(i + 20, i + 40)
	print i, prepitches, postpitches
	print i, CsoundVST.Voicelead_uniquePcs(prepitches), CsoundVST.Voicelead_uniquePcs(postpitches)
	print ' '
score.save('CsoundVstUnitTest.py.4.mid')

for n in string.split('C C# D D# E F G G# A A# B'):
    name = n + "M7"
    chord = CsoundVST.Conversions_nameToPitches(n + "M7")
    prime = CsoundVST.Voicelead_primeChord(chord)
    print name, chord, prime
    p = 0.0
    t = 0.0
    result = CsoundVST.Voicelead_primeAndTranspositionFromPitchClassSet(chord, int(12))
    print result
    

	




	
	







































































