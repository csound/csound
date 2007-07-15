'''
Unit tests for CsoundVST.Voicelead
'''
import CsoundVST
import copy

print __doc__

def Test(test):
	print '=========================================================================='
	print
	print test
	print
	result = eval(test)
	print 'Result:',result
	print

def addOctave(lowestVoicing, newVoicing, maximumPitch, divisionsPerOctave):
	for i in xrange(len(lowestVoicing)):
		newPitch = newVoicing[i] + divisionsPerOctave
		if newPitch >= maximumPitch:
			newVoicing[i] = lowestVoicing[i]
		else:
			newVoicing[i] = newPitch
			return True
	return False
  
C = [11, 12, 16, 19]
voicing = copy.copy(C)
i = 1
print i, voicing
while addOctave(C, voicing, 60, 12):
	i = i + 1
	print i, voicing
print
	


Test('CsoundVST.Voicelead_chordToPTV([0, 4, 7], 0, 24, 12)')
Test('CsoundVST.Voicelead_chordToPTV([1, 5, 8], 0, 24, 12)')
Test('CsoundVST.Voicelead_ptvToChord(69, 1, 0, 0, 24, 12)')
Test('CsoundVST.Voicelead_chordToPTV([12, 16, 19], 0, 24, 12)')
Test('CsoundVST.Voicelead_ptvToChord(69, 0, 7, 0, 24, 12)')
Test('CsoundVST.Voicelead_allVoicings([12, 16, 19], 0, 24, 12)')
Test('CsoundVST.Voicelead_allVoicings([0, 0, 0], 0, 24, 12)')
Test('CsoundVST.Voicelead_wrap([0, 4, 7], 0, 24)')
Test('CsoundVST.Voicelead_wrap([12, 16, 19], 0, 24)')
Test('CsoundVST.Voicelead_wrap([24, 16, 19], 0, 24)')
Test('CsoundVST.Voicelead_chordToPTV([12, 4, 7], 0, 24, 12)')
Test('CsoundVST.Voicelead_chordToPTV([0, 4, 7, 11], 0, 48, 12)')
Test('CsoundVST.Voicelead_normalChord([0, 4, 7, 11])')
Test('CsoundVST.Voicelead_wrap([11, 12, 16, 19], 0, 12)')
Test('CsoundVST.Voicelead_allVoicings([1,2,3,4], 0, 24, 12)')


quit()

