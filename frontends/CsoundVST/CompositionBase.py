import CsoundVST
import csnd

import atexit
import math
import psyco
from scipy import *
import signal
import sys
import threading
import traceback
'''
C O M P O S I T I O N   B A S E
Copyright (c) 2006 by Michael Gogins.
All rights reserved.
This software is licensed under the GNU Lesser General Public License.

CompositionBase is designed to be a general-purpose base class
for algorithmic compositions made with Silence and Csound.

CompositionBase encapsulates and automates most of the housekeeping
for algorithmic composition, and enables a general-purpose Python
development environment to be used for composing. Normally,
only the assembleModel and assembleOrchestra methods need to be
overridden.

Compositions may be developed with or without graphical user interfaces.

1. Assemble a MusicModel.
2. Generate the music graph for the MusicModel.
3. Assemble a Csound orchestra.
4. Export the Csound score and orchestra for performance,
   and incidentally also always export a MIDI file of the score.
5. Assemble an appropriate Csound command line.
6. Render the composition -- as real-time audio for preview,
   as a CD quality soundfile, or as a high-resolution soundfile.
7. The rendering can be stopped at any time from the development environment.
'''

global composition
composition = None

class CompositionBase(object):
    def __init__(self):
        self.csound = csound
        self.model = CsoundVST.MusicModel()
        self.model.setCppSound(self.csound)
        self.keepPerforming = False
        self.csoundThread = None
        composition = self
    '''
    All filenames are based off the composition script.
    '''
    def getFilename(self):
        return __name__
    def getMidiFilename(self):
        return self.getFilename() + '.mid'
    def getOutputSoundfileName(self):
        return self.getFilename() + '.wav'
    def getOrcFilename(self):
        return self.getFilename() + '.orc'
    def getScoFilename(self):
        return self.getFilename() + '.sco'
    '''
    This method is desgned to be overridden in compositions.
    The default implementation assembles a Lindenmayer system score.
    CsoundVST objects created here should have thisown = 0.
    '''
    def assembleModel(self):
        print 'BEGAN CompositionBase.assembleModel...'
        lindenmayer = CsoundVST.Lindenmayer()
        lindenmayer.thisown = 0
        lindenmayer.setAxiom("b")
        lindenmayer.setAngle(2.0 * math.pi / 9.0)
        lindenmayer.addRule("b", " b [  Ti-1 a b ] Tt+1 Tk-3.1 a N b Tt+3 N Tt+1.3 Tk+2 b [ Ti+1 a b ] N")
        lindenmayer.addRule("a", " N Tt+1.1 Tk+1 N [ Tk+2 b ] Tk+3 N Tk-3 Tt-1 [ Tt+1 Tk-4 a ] N ")
        lindenmayer.setIterationCount(6)
        lindenmayer.generate()
        random = CsoundVST.Random()
        random.thisown = 0
        random.createDistribution("uniform_real")
        random.setElement(7, 11, 1)
        rescale = CsoundVST.Rescale()
        rescale.thisown = 0
        rescale.setRescale( 0, 1, 1,  0,     240)
        rescale.setRescale( 1, 1, 1,  6,       4)
        rescale.setRescale( 3, 1, 1,  2,       6)
        rescale.setRescale( 4, 1, 1, 36,      60)
        rescale.setRescale( 5, 1, 1, 60,      15)
        rescale.setRescale( 7, 1, 1, -0.75,    1.5)
        scale = 'E major'
        scalenumber = CsoundVST.Conversions.nameToM(scale)
        print '"%s" = %s' % (scale, scalenumber)
        rescale.setRescale(10, 1, 1,  scalenumber,    0)
        random.addChild(lindenmayer)
        rescale.addChild(random)
        self.model.addChild(rescale)
        print 'ENDED CompositionBase.assembleModel.'
    '''
    This method is designed to be overridden in compositions.
    The default implementation loads the Silence orchestra.
    '''
    def assembleOrchestra(self):
        print 'BEGAN CompositionBase.assembleOrchestra...'
        csound.load(r'D:/utah/home/mkg/projects/music/library/CsoundVST.csd')
        print 'ENDED CompositionBase.assembleOrchestra.'
    def generateScore(self):
        print 'BEGAN CompositionBase.generateScore...'
        self.model.generate()
        score = self.model.getScore()
        score.thisown = 0
        duration = score.getDuration() + 8.0
        score.arrange(0,56, 1.0)
        score.arrange(1,15, 1.0)
        score.arrange(2,19, 1.0)
        score.arrange(3, 8, 1.0)
        score.arrange(4,36, 1.0)
        score.arrange(5,56, 1.0)
        score.arrange(6,56, 1.0)
        score.arrange(7,56, 1.0)
        print 'Duration: %9.4f' % (duration)
        self.model.createCsoundScore('''
        ; EFFECTS MATRIX

        ; Chorus to Reverb
        i 1 0 0 200 210 0.0
        ; Leslie to Reverb
        ; i 1 0 0 201 210 0.5
        ; Chorus to Output
        i 1 0 0 200 220 0.5
        ; Reverb to Output
        i 1 0 0 210 220 0.5

        ; SOUNDFONTS OUTPUT

        ; Insno Start   Dur     Key 	Amplitude
        i 190 	0       -1      0	84.

        ; PIANOTEQ OUTPUT

        ; Insno Start   Dur     Key 	Amplitude
        i 191 	0       -1      0	64.

        ; MASTER EFFECT CONTROLS

        ; Chorus.
        ; Insno	Start	Dur	Delay	Divisor of Delay
        i 200   0       -1      10      30

        ; Reverb.
        ; Insno	Start	Dur	Delay	Pitch mod	Cutoff
        i 210   0       -1      0.87    0.015  		16000

        ; Master output.
        ; Insno	Start	Dur	Fadein	Fadeout
        i 220   0       -1      0.1     0.1

        ''')
        print 'Generated score:'
        print self.csound.getScore()
        self.csound.setCommand(self.command)
        self.csound.exportForPerformance()
        self.model.getScore().save(self.getMidiFilename())
        print 'ENDED CompositionBase.exportForPerformance.'
    def getAudioCommand(self, dac):
        command = 'csound -m1 -h -d -r 44100 -k 441 -o %s %s %s' % (dac, self.getOrcFilename(), self.getScoFilename())
        print 'Command: %s' % (command)
        return command
    def getCdSoundfileCommand(self):
        command = 'csound -m3 -R -W -d -r 44100 -k 100 -o %s %s %s' % (self.getOutputSoundfileName(), self.getOrcFilename(), self.getScoFilename())
        print 'Command: %s' % (command)
        return command
    def getMasterSoundfileCommand(self):
        command = 'csound -m3 -R -W -Z -d -r 88200 -k 1 -o %s %s %s' % (self.getOutputSoundfileName(), self.getOrcFilename(), self.getScoFilename())
        print 'Command: %s' % (command)
        return command
    def perform(self):
        print 'BEGAN CompositionBase.perform...'
        self.keepPerforming = True
        if self.keepPerforming:
            self.model.clear()
        if self.keepPerforming:
            self.assembleModel()
        if self.keepPerforming:
            self.assembleOrchestra()
        if self.keepPerforming:
            self.generateScore()
        if self.keepPerforming:
            self.csound.perform()
        print 'ENDED CompositionBase.perform.'
    '''
    High-level function to generate a score and orchestra,
    export them, and render them as audio in real time
    on the indicated audio interface.
    '''
    def performAudio(self, dac='dac8'):
        print 'BEGAN CompositionBase.peformAudio(%s)...' % (dac)
        try:
            self.command = self.getAudioCommand(dac)
            self.perform()
        except:
            traceback.print_exc()
        print 'ENDED CompositionBase.performAudio.'
    '''
    High-level function to generate a score and orchestra,
    export them, and render them as a CD-quality soundfile.
    '''
    def performCdSoundfile(self):
        print 'BEGAN CompositionBase.performCdSoundfile...'
        try:
            self.command = self.getCdSoundfileCommand()
            self.keepPerforming = True
            self.perform()
        except:
            traceback.print_exc()
        print 'ENDED CompositionBase.performCdSoundfile.'
    '''
    High-level function to generate a score and orchestra,
    export them, and render them as a high-resolution,
    master-quality soundfile.
    '''
    def performMasterSoundfile(self):
        print 'BEGAN CompositionBase.performMasterSoundfile...'
        try:
            self.command = self.getMasterSoundfileCommand()
            self.perform()
        except:
            traceback.print_exc()
        print 'ENDED CompositionBase.performMasterSoundfile.'
    '''
    Stops performance at any stage.
    '''
    def stop(self):
        print 'BEGAN CompositionBase.stop...'
        self.keepPerforming = False
        self.csound.stop()
        print 'ENDED CompositionBase.stop.'

def signalHandler(signal, frame):
    print 'signalHandler called with signal: %d' % (signal)
    global composition
    if composition:
        composition.stop()
        composition = None
    
# Set up signal handling so Csound can be killed
# when performing in the development environment.

signal.signal(signal.SIGABRT,   signalHandler) 
signal.signal(signal.SIGBREAK,  signalHandler) 
signal.signal(signal.SIGFPE,    signalHandler) 
signal.signal(signal.SIGILL,    signalHandler) 
signal.signal(signal.SIGINT,    signalHandler) 
signal.signal(signal.SIGSEGV,   signalHandler) 

'''
Unit test rendering.
'''
if __name__ == '__main__':
    print 'Create composition...'
    composition = CompositionBase()
    print 'Began performance...'
    composition.performAudio()
    print 'Ended performance.'


        
