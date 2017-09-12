import CsoundAC
from   scipy import *
import scipy.signal.waveforms
import time
import math
from   numpy import *
import os
import random
import sys
# Use both numpy and psyco to get all possible speed
# out of Python.
import psyco
psyco.full()

print 'CREATING FILENAMES...'
print

scriptFilename = sys.argv[0]
print 'Full Python script:  %s' % scriptFilename
title = os.path.basename(scriptFilename)
print 'Base Python script:  %s' % title
directory = os.path.dirname(scriptFilename)
if len(directory):
    print 'Working directory:   %s' % directory
    os.chdir(directory)
print 'Working directory:   %s' % directory
soundfileName = title + '.wav'
print 'Soundfile name:      %s' % soundfileName
soundfilePlayer = r'D:\utah\opt\Audacity\audacity.exe'
print 'Soundfile player:    %s' % soundfilePlayer
print

print 'CREATING BLANK SOUNDFILE...'
print
if os.path.exists(soundfileName):
    os.remove(soundfileName)
began = time.time()
print 'Began', time.ctime(began)
soundfile = CsoundAC.Soundfile()
soundfile.create(soundfileName, 88200, 1)
# Create a blank output soundfile
# with additional time padded at beginning and end.
soundfile.blank(90)
print 'Blank', time.ctime()
print
print 'CREATING 1 440 Hz 10 second grain centered at 10 seconds'
print
soundfile.cosineGrain(10, 10.0, 440.0, 0.1, 0.0, 0.0)

print 'CREATING 2 440 Hz 10 second grains overlapped at 30 seconds'
print
soundfile.cosineGrain(30, 10.0, 440.0, 0.1, 0.0, 0.0)
soundfile.cosineGrain(35, 10.0, 440.0, 0.1, 0.0, 0.0)

print 'CREATING 1 880 Hz 0.01 second grain centered at 45 seconds'
print
soundfile.cosineGrain(45, 0.01, 880.0, 0.1, 0.0, 0.0)

print 'CREATING 2 440 Hz 1 second grains of different phase at 50 seconds.'
print 'Check sonogram -- verify that frequencies are identical.'
print
soundfile.cosineGrain(50.0, 1.0, 440.0, 0.1, 0.0, 0.0)
soundfile.cosineGrain(52.0, 1.0, 440.0, 0.1, math.pi, 0.0, False)

print 'CREATING 2 440 Hz 1 second grains out of phase by pi at 53 seconds.'
print 'Check sonogram -- verify that amplitude cancels to 0.'
print
soundfile.cosineGrain(53.0, 1.0, 440.0, 0.1, 0.0, 0.0)
soundfile.cosineGrain(53.0, 1.0, 440.0, 0.1, math.pi, 0.0, False)

print 'CREATING 2 440 Hz 1 second grains out of phase by pi / 2 at 55 seconds.'
print 'Check sonogram -- verify that amplitude reinforces x 1.5.'
print
soundfile.cosineGrain(55.0, 1.0, 440.0, 0.1, 0.0, 0.0)
soundfile.cosineGrain(55.0, 1.0, 440.0, 0.1, math.pi / 2.0, 0.0, False)

print 'CREATING 500 880 Hz 0.01 second grains overlapped at 60 seconds'
print
d = 0.01
t = 60.0
f = 880.0
A = 0.1
phase = 0.0
for i in xrange(500):
	print 'grain:  t: %9.4f  d: %9.4f  f: %9.4f  A: %9.4f  phi: %9.4f' % (d,t,f,A,phase)
	soundfile.cosineGrain(t, d, f, A, phase, 0.0, False)
	t = t + d / 2.0
	
print 'CREATING 500 880 Hz 0.01 second grains overlapped at 75 seconds, with synchronous phase.'
print 'Check sonogram -- this sound should have no interference tones, as compared with the previous sound.'
print 'Use this approach to create phase-synchronous collages of grains.'
d = 0.01
t = 75.0
f = 880.0
A = 0.1
w = 1.0 / f
phase = True
for i in xrange(500):
##	# Convert fractional wavelengths to radians.
##	wavelengths = t / w
##	fraction, wholecycles = math.modf(wavelengths)	
##	phase = fraction * 2.0 * math.pi
	print 'grain:  t: %9.4f  d: %9.4f  f: %9.4f  A: %9.4f  phi: %9.4f' % (d,t,f,A,phase)
	soundfile.cosineGrain(t, d, f, A, phase, 0.0)
	t = t + d / 2.0
	
soundfile.close()
ended = time.time()
print 'Ended', time.ctime(ended)
elapsed = ended - began
#print 'Elapsed time     %s' % (time.strftime('%H:%M:%S', time.gmtime(elapsed))
os.spawnl(os.P_NOWAIT, soundfilePlayer, soundfilePlayer, r'"%s"' % (os.getcwd() + os.sep + soundfileName))

