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
	exec test
	print
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

Test('''print CsoundVST.Voicelead_wrap([0, 4, 7], 0, 24, 12)''')	
Test('''print CsoundVST.Voicelead_wrap([4, 8, 11], 0, 24, 12)''')	
Test('''print CsoundVST.Voicelead_wrap([12, 16, 19], 0, 24, 12)''')	
Test('''print CsoundVST.Voicelead_chordToPTV([0, 4, 7], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([0, 4, 7], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_chordToPTV([1, 5, 8], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([7, 11, 14], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_ptvToChord(69, 1, 0, 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_chordToPTV([12, 16, 19], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_ptvToChord(69, 0, 7, 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([0, 0, 0], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_wrap([0, 4, 7], 0, 24)''')
Test('''print CsoundVST.Voicelead_wrap([12, 16, 19], 0, 24)''')
Test('''print CsoundVST.Voicelead_wrap([24, 16, 19], 0, 24)''')
Test('''print CsoundVST.Voicelead_wrap([0, 4, 7], 12, 24)''')
Test('''print CsoundVST.Voicelead_wrap([12, 16, 19], 13, 36)''')
Test('''print CsoundVST.Voicelead_wrap([24, 16, 19], 12, 24)''')
Test('''print CsoundVST.Voicelead_wrap([25, 16, 19], 12, 24)''')
Test('''print CsoundVST.Voicelead_chordToPTV([12, 4, 7], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_normalChord([0, 4, 7, 11])''')
Test('''print CsoundVST.Voicelead_wrap([11, 12, 16, 19], 0, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([0, 4, 7, 11], 0, 24, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([0, 4, 7, 11], 24, 60, 12)''')
Test('''print CsoundVST.Voicelead_allVoicings([2, 6, 9, 13], 24, 60, 12)''')
Test('''print CsoundVST.Voicelead_chordToPTV([36, 40, 43, 59], 0, 60, 12)''')
Test('''print CsoundVST.Voicelead_ptvToChord(128, 11, 562, 0, 60, 12)''')
Test('''print CsoundVST.Voicelead_chordToPTV([12, 16, 19, 23], 0, 60, 12)''')
Test('''print CsoundVST.Voicelead_chordToPTV([13, 17, 20, 24], 0, 60, 12)''')


quit()

