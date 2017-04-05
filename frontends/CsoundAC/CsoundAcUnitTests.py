import CsoundAC
import math
import random
import string
import sys
sys.path.append('d:/utah/home/mkg/projects/icmc2006-mkg')
import tonnetz

print 'UNIT TESTS FOR CsoundAC.Voicelead and CsoundAC.Score'
print

lowest = 48
range = 36
highest = lowest + range

for i in xrange(36, 96, 1):
    p = float(i)
    print '%8.3f = Voicelead_pc(%8.3f)' % (CsoundAC.Voicelead_pc(p), p)
print
a = [0, 4, 7, 11]
b = [1, 5, 8,  0]
print '%s = Voicelead_voiceleading(%s, %s)' % (CsoundAC.Voicelead_voiceleading(a, b), a, b)
print
c = [5, 5, 5, 5]
print '%s = Voicelead_areParallel(%s, %s)' % (CsoundAC.Voicelead_areParallel(a, b), a, b)
print '%s = Voicelead_areParallel(%s, %s)' % (CsoundAC.Voicelead_areParallel(a, c), a, c)
print
print '%s = Voicelead_rotate(%s)' % (CsoundAC.Voicelead_rotate(a), a)
print
rotations = CsoundAC.Voicelead_rotations(a)
print '%s = CsoundAC.Voicelead_rotations(%s)' % (a, rotations)
for chord in rotations:
    print chord
print
pitches = [65., 69., 72., 76.]
print '%s = Voicelead_pcs(%s)' % (CsoundAC.Voicelead_pcs(pitches), pitches)
print '%8.3f = Voiclead_mFromPitchClassSet(%s)' % (CsoundAC.Voicelead_pitchClassSetToM(pitches), pitches)
print '%8.3f = Voiclead_mFromPitchClassSet(%s)' % (CsoundAC.Voicelead_pitchClassSetToM(a), a)
print
print '%s = Voicelead_pitchClassSetFromM(%8.3f)' % (CsoundAC.Voicelead_mToPitchClassSet(2193), 2193)
print
voicings = CsoundAC.Voicelead_voicings(a, lowest, range, 12)
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
t1 = CsoundAC.Voicelead_voicelead(CM7, FM7, lowest, range, False)
print '%s = CsoundAC.Voicelead_voicelead(%s, %s, lowest, range, True)' % (t1, CM7, FM7)
t2 = CsoundAC.Voicelead_voicelead(t1, G7, lowest, range, False)
print '%s = CsoundAC.Voicelead_voicelead(%s, %s, lowest, range, True)' % (t2, t1, G7)
print
t1 = CsoundAC.Voicelead_recursiveVoicelead(CM7, FM7, lowest, range, False)
print '%s = CsoundAC.Voicelead_recursiveVoicelead(%s, %s, lowest, range, True)' % (t1, CM7, FM7)
t2 = CsoundAC.Voicelead_recursiveVoicelead(t1, G7, lowest, range, False)
print '%s = CsoundAC.Voicelead_recursiveVoicelead(%s, %s, lowest, range, True)' % (t2, t1, G7)
print
print 'Inversions of', a
vinversions = CsoundAC.Voicelead_inversions(a)
tonnetz = tonnetz.Tonnetz(4)
tinversions = tonnetz.rotations(a)
for i in xrange(len(vinversions)):
	vinversion = vinversions[i]
	tinversion = tinversions[i]
	print 'inversion of %s:           V: %s  T: %s' % (a, vinversion, tinversion)
	print '  origin chord of inversion:          V: %s  T: %s' % (CsoundAC.Voicelead_toOrigin(vinversion), tonnetz.zeroForm(tinversion))
	print '  normal chord of inversion:          V: %s  T: %s' % (CsoundAC.Voicelead_normalChord(vinversion), tonnetz.firstInversion(tinversion))
	print '  zero chord of inversion:            V: %s  T: %s' % (CsoundAC.Voicelead_primeChord(vinversion), tonnetz.zeroForm(tonnetz.firstInversion(tinversion)))
	print ' '
print
for i in xrange(lowest, range, 1):
	print '%8.3f = Voiclead_closestPitch(Voicelead_pc(%d), %s)' %(CsoundAC.Voicelead_closestPitch(i, CM7), i, CM7)
print
for i in xrange(lowest, range, 1):
	print '%8.3f = Voiclead_conformToPitchClassSet(%d, %s)' %(CsoundAC.Voicelead_conformToPitchClassSet(i, a), i, a)
print
score = CsoundAC.Score()
for i in xrange(2000):
	time = i * 0.125
	duration = 0.5
	key = random.randint(lowest, highest)
	velocity = 80.0
	score.append(time, duration, 144.0, 1.0, key, velocity)
score.save('CsoundACUnitTest.py.1.mid')
score.setPitchClassSet(0, len(score), a)
score.save('CsoundACUnitTest.py.2.mid')
score = CsoundAC.Score()
for i in xrange(2000):
	time = i * 0.125
	duration = 0.5
	key = random.randint(lowest, highest)
	velocity = 80.0
	score.append(time, duration, 144.0, 1.0, key, velocity)
C = CsoundAC.Voicelead_mToC(CsoundAC.Voicelead_pitchClassSetToM([0.,4.,7.,11.,14.]), 12)
for i in xrange(0, len(score), 20):
	M = CsoundAC.Voicelead_cToM(C)
	pcs = CsoundAC.Voicelead_mToPitchClassSet(M)
	print 'M: %8.3f  C: %8.3f  %s' % (M, C, pcs)
	score.setPitchClassSet(i, i + 20, pcs)
	result = CsoundAC.Voicelead_uniquePcs(score.getPitches(i, i + 20))
	print 'Result:',result
	print ' '
	C = 2 * C + 1
	C = C % 4094
score.save('CsoundACUnitTest.py.3.mid')
for i in xrange(0, len(score) - 40, 20):
	prepitches = score.getPitches(i, i + 20)
	score.voicelead(i, i + 20, i + 20, i + 40, lowest, highest - lowest, True)
	postpitches = score.getPitches(i + 20, i + 40)
	print i, prepitches, postpitches
	print i, CsoundAC.Voicelead_uniquePcs(prepitches), CsoundAC.Voicelead_uniquePcs(postpitches)
	print ' '
score.save('CsoundACUnitTest.py.4.mid')

for n in string.split('C C# D D# E F G G# A A# B'):
    name = n + "M7"
    chord = CsoundAC.Conversions_nameToPitches(n + "M7")
    prime = CsoundAC.Voicelead_primeChord(chord)
    print name, chord, prime
    p = 0.0
    t = 0.0
    result = CsoundAC.Voicelead_primeAndTranspositionFromPitchClassSet(chord, int(12))
    print result
    

	




	
	







































































