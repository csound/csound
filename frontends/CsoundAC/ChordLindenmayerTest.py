#!/usr/bin/python
'''
A   S I L E N C E   C O M P O S I T I O N
Copyright (C) 2009 by Michael Gogins.
All rights reserved.

This file serves as a template for Csound compositions,
and is intended to speed up and simplify the music production 
workflow as much as possible. 

A studio quality, general-purpose Csound orchestra with a multi-buss
mixer and mastering effects is embedded in this file, 
and can of course be extended and/or customized.

This file should contain and archive all materials required
to realize a composition, with the exception of binaries 
(Csound, external plugins) and external source material
(audio samples, SoundFonts, external MIDI or MusicXML files).

In general, new compositions will be created by using the 
previous composition as a template, replacing the bodies 
of the CsoundComposition.createMusicModel() and 
CsoundComposition.createCsoundArrangement() functions,
and modifying the CsoundComposition.createCsoundOrchestra
function.

The execution of this file is controlled by the 
CsoundComposition.renderingMode field:

  'score':   There is no waveform audio output. The generated score, 
             which is always saved as a MIDI file, is loaded into 
             a notation program or MIDI file player for rapid preview.
             This is the quickest way to get a preview of the piece.
  'audio':   The score will be rendered to the real-time audio output (dac),
             44,100 Hz stereo float samples, at 100 ksmps.
  'preview:' The master output soundfile will be CD audio quality,
             44,100 Hz stereo float samples, at 100 ksmps (this is the default).
  'cd':      The master output soundfile will be CD audio quality,
             44,100 Hz stereo float samples, at 1 ksmps.
  'master':  The master output soundfile will be high-resolution audio,
             88,200 Hz stereo float samples, at 1 ksmps.
  'virtual': The orchestra only will render to the real-time audio output,
             using a virtual MIDI keyboard for interactive instrument testing.
  'midi':    The orchestra only will render to the real-time audio output,
             using an actual MIDI keyboard for interactive instrument testing.

For the 'preview', 'cd,' and 'master' modes, the following additional
processing is automatically performed:

  (1) The initial output soundfile is normalized to -3 dBFS.
  (2) The normalized output soundfile is converted to a CD audio track 
      (44,100 Hz, stereo, 16 bit samples), with ID tags.
  (3) The normalized output soundfile is converted to an MP3 soundfile,
      with ID tags.
  (4) If in addition the CsoundComposition.playback field is True, 
      the normalized output soundfile is opened in an external editor for audition
      and/or additional processing.

'''
print __doc__
print 'IMPORTING REQUIRED MODULES...'
print
import csnd6
import CsoundAC
import datetime
import math
import numpy
import os
try:
    import psyco
    psyco.full()
    print 'Using psyco.'
    print
except:
    print 'Psyco not available.'
    print
import random
import signal
import string
import subprocess
import sys
import time
import traceback

class CsoundComposition(object):
    def __init__(self):
        # Set "renderingMode" to:  "cd", "preview" (default), "master", "test" (virtual keyboard orc test) or "audio".
        # Set "playback" to:       True (default) or False.
        self.renderingMode = 'preview'
        self.playback = True
        if os.name == 'posix':
            self.dacName = 'dac'
        else:
            self.dacName = 'dac'
    def createGlobalObjects(self):
        print 'CREATING GLOBAL OBJECTS...'
        print
        self.model = CsoundAC.MusicModel()
        self.csound = self.model.getCppSound()
        self.csound.setPythonMessageCallback()  
        self.score = self.model.getScore()
    def createFilenames(self):
        print 'CREATING FILENAMES...'
        print
        self.composer = 'Michael_Gogins'
        print 'Composer:                %s' % self.composer
        self.scriptPathname = os.path.realpath(sys.argv[0])
        print 'Full script pathname:    %s' % self.scriptPathname
        self.title, self.ext = os.path.splitext(os.path.basename(self.scriptPathname))
        print 'Title:                   %s' % self.title
        self.copyright = 'Copr._%d_%s' % (self.timestamp.year, self.composer)
        print 'Copyright:               %s' % self.copyright
        self.directory = os.path.dirname(self.scriptPathname)
        if len(self.directory):
            os.chdir(self.directory)
        print 'Working directory:       %s' % self.directory
        self.orcFilename = self.scriptPathname + '.orc'
        print 'Csound orchestra:        %s' % self.orcFilename
        self.scoFilename =  self.scriptPathname + '.sco'
        print 'Csound score:            %s' % self.scoFilename
        self.midiFilename =  self.scriptPathname + '.mid'
        print 'MIDI file:               %s' % self.midiFilename
        self.xmlFilename =  self.scriptPathname + '.xml'
        print 'MusicXML2 file:          %s' % self.xmlFilename
        self.csoundOutputSoundfile = self.scriptPathname + '.output.wav'
        print 'Output soundfile:        %s' % self.csoundOutputSoundfile
        self.rescaledSoundfile =  self.scriptPathname + '.wav'
        print 'Rescaled soundfile:      %s' % self.rescaledSoundfile
        self.cdTrackSoundfile =  self.scriptPathname + '.cd.wav'
        print 'CD track soundfile:      %s' % self.cdTrackSoundfile
        self.mp3Soundfile =  self.scriptPathname + '.mp3'
        print 'MP3 soundfile:           %s' % self.mp3Soundfile
        print 'Audio output name:       %s' % self.dacName
        self.createCsoundCommands()
        if os.name == 'posix':
            self.soundfileScaler = '/home/mkg/csound5/scale'
            self.formatConverter = '/usr/bin/sndfile-convert'
            self.mp3encoder = '/usr/bin/lame'
            self.soundfilePlayer = '/usr/bin/sweep'
            self.midifilePlayer = '/usr/bin/timidity'
        elif os.name == 'nt':
            self.soundfileScaler = r'scale'
            self.formatConverter = r'D:/utah/opt/Mega-Nerd/libsndfile/sndfile-convert'
            self.mp3encoder = r'D:/utah/opt/lame/lame'
            self.soundfilePlayer = r'D:/utah/opt/audacity/audacity'
            self.midifilePlayer = r'C:/utah/opt/Pianoteq-3.5/Pianoteq35.exe'
        self.rescalerCommand = r'%s -P 75 -o %s %s' % (self.soundfileScaler, self.rescaledSoundfile, self.csoundOutputSoundfile)
        print 'Rescaler command:        %s' % self.rescalerCommand
        self.cdTrackCommand = r'%s -pcm16 %s %s' % (self.formatConverter, self.rescaledSoundfile, self.cdTrackSoundfile)
        print 'CD track command:        %s' % self.cdTrackCommand
        self.encoderCommand = r'%s -b 192 --tt %s --ta %s --tc %s %s %s' % (self.mp3encoder, self.title, self.composer, self.copyright, self.cdTrackSoundfile, self.mp3Soundfile)
        print 'Encoder command:         %s' % self.encoderCommand
        self.playerCommand = r'%s %s' % (self.soundfilePlayer, self.rescaledSoundfile)
        print 'Player command:          %s' % self.playerCommand
        self.midifilePlayerCommand = r'%s %s' % (self.midifilePlayer, self.midiFilename)
        print 'MIDI player command:     %s' % self.midifilePlayerCommand
        print
    def createCsoundCommands(self):
        self.commandsForRendering = {
            'preview':  r'csound -g -m163 -W -f -R -K -r 44100 -k 441   --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s -o %s %s %s' % (self.composer, self.composer, self.title, self.csoundOutputSoundfile, self.orcFilename, self.scoFilename),
            'cd':       r'csound -g -m163 -W -f -R -K -r 48000 -k 375 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s -o %s %s %s' % (self.composer, self.composer, self.title, self.csoundOutputSoundfile, self.orcFilename, self.scoFilename),
            'master':   r'csound -g -m163 -W -f -R -K -r 88200 -k 88200 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s -o %s %s %s' % (self.composer, self.composer, self.title, self.csoundOutputSoundfile, self.orcFilename, self.scoFilename),
            'audio':    r'csound -g -m163 -h -r 48000 -k 375 --midi-key=4 --midi-velocity=5 -o %s %s %s' % (self.dacName, self.orcFilename, self.scoFilename),           'score':    r'',
            'virtual':  r'csound -g -m163 -h -r 44100 -k 441 --midi-key=4 --midi-velocity=5 -+rtmidi=virtual -M0 -o %s %s %s' % (self.dacName, self.orcFilename, self.scoFilename),
            'midi':     r'csound -g -m163 -h -r 44100 -k 100 --midi-key=4 --midi-velocity=5 -M0 -o %s %s %s' % (self.dacName, self.orcFilename, self.scoFilename)
        }    
        self.csoundCommand = self.commandsForRendering[self.renderingMode]
        print 'Csound command line:     %s' % self.csoundCommand
        print
    def renderCsound(self):
        print 'RENDERING WITH CSOUND...'
        print
        self.model.setCsoundCommand(self.csoundCommand)
        self.createMusicModel()
        self.model.generate()
        self.createScore()
        self.ended = time.clock()
        self.elapsed = self.ended - self.began
        print 'Finished rendering at               %s' % time.strftime('%Y-%b-%d %A %H:%M:%S')
        print 'Elapsed time:                        %-9.2f seconds.' % self.elapsed
        self.csound.perform()
        print
        self.ended = time.clock()
        self.elapsed = self.ended - self.began
        print 'Finished rendering at                %s' % time.strftime('%Y-%b-%d %A %H:%M:%S')
        print 'Elapsed time:                        %-9.2f seconds.' % self.elapsed
        print
        if self.renderingMode == 'audio':
            exit(0)
        self.normalizeOutputSoundfile()
        self.createCdAudioTrack()
        self.createMp3Soundfile()
        if self.playback == True:
            self.openOutputSoundfile()
    def renderMidiScore(self):
        print 'GENERATING MIDI SCORE FOR PREVIEW...'
        print
        self.createMusicModel()
        self.model.generate()
        self.createScore()
        self.ended = time.clock()
        self.elapsed = self.ended - self.began
        print 'Finished generating at               %s' % time.strftime('%Y-%b-%d %A %H:%M:%S')
        print 'Elapsed time:                        %-9.2f seconds.' % self.elapsed
        self.ended = time.clock()
        self.elapsed = self.ended - self.began
        print 'Finished rendering at                %s' % time.strftime('%Y-%b-%d %A %H:%M:%S')
        print 'Elapsed time:                        %-9.2f seconds.' % self.elapsed
        print
        self.playMidifile()
        exit()
    def renderMidiVirtualTest(self):
        self.createScore()
        print 'PLAY VIRTUAL KEYBOARD FOR CSOUND ORCHESTRA TEST...'
        print
        self.csound.perform()
        exit()
    def renderMidiActualTest(self):
        self.createScore()
        print 'PLAY ACTUAL MIDI KEYBOARD FOR CSOUND ORCHESTRA TEST...'
        print
        self.csound.perform()
        exit()
    def createScore(self):
        print 'CREATING SCORE...'
        print
        self.createCsoundOrchestra()
        self.createCsoundArrangement()
        self.model.createCsoundScore()
        print
        print 'Saving MIDI file %s...' % self.midiFilename
        print
        self.score.save(self.midiFilename)
        print
    def playMidifile(self):
        print 'PLAYING MIDI FILE...'
        print
        try:
            status = subprocess.call(self.midifilePlayerCommand, shell=True)
        except:
            traceback.print_exc()
        print
    def normalizeOutputSoundfile(self):
        print 'NORMALIZING OUTPUT SOUNDFILE...'
        print
        try:
            status = subprocess.call(self.rescalerCommand, shell=True)
            os.remove(self.csoundOutputSoundfile)
        except:
            traceback.print_exc()
        print
    def createCdAudioTrack(self):
        print 'PREPARING CD-AUDIO TRACK...'
        print
        try:
            status = subprocess.call(self.cdTrackCommand, shell=True)
        except:
            traceback.print_exc()
        print
    def createMp3Soundfile(self):
        print 'ENCODING TO MP3...'
        print
        try:
            status = subprocess.call(self.encoderCommand, shell=True)
        except:
            traceback.print_exc()
        print
    def openOutputSoundfile(self):
        print 'OPENING OUTPUT SOUNDFILE...'
        print
        try:
            popen = subprocess.call(self.playerCommand, shell=True)
        except:
            traceback.print_exc()
        print
    def render(self):
        print 'RENDERING OPTIONS...'
        print
        print 'Rendering mode:          %s' % self.renderingMode
        print 'Playback:                %s' % self.playback
        print
        self.began = time.clock()
        self.timestamp = datetime.datetime.now()
        print 'Timestamp:               %s' % self.timestamp
        self.createGlobalObjects()
        self.createFilenames()
        self.createCsoundCommands()
        if   self.renderingMode == 'score':
            self.renderMidiScore()
        elif self.renderingMode == 'virtual':
            self.renderMidiVirtualTest()
        elif self.renderingMode == 'midi':
            self.renderMidiActualTest()
        else:
            self.renderCsound()
    def createCsoundOrchestra(self):
        print 'CREATING CSOUND ORCHESTRA...'
        print
        self.csoundOrchestra = \
'''
<CsoundSynthesizer>
<CsOptions>
csound -m255 -M0 -+rtmidi=null -RWf --midi-key=4 --midi-velocity=5 -o jacko_test.wav
</CsOptions>
<CsInstruments>

; Unit tests for the Jack opcodes.
; To do:
; -- Python score generator and client runner.

sr    	   = 48000
ksmps 	   = 128
nchnls 	   = 2
0dbfs 	   = 1

	   JackoInit		"default", "csound"

	   ; To use ALSA midi ports, use "jackd -Xseq"
	   ; and use "jack_lsp -A -c" or aliases from JackInfo,
	   ; probably together with information from the sequencer,
	   ; to figure out the damn port names.

	   ; JackoMidiInConnect   "alsa_pcm:in-131-0-Master", "midiin"
	   JackoAudioInConnect 	"aeolus:out.L", "leftin"
	   JackoAudioInConnect 	"aeolus:out.R", "rightin"
	   JackoMidiOutConnect 	"midiout", "aeolus:Midi/in"

           ; Note that Jack enables audio to be output to a regular
 	   ; Csound soundfile and, at the same time, to a sound 
	   ; card in real time to the system client via Jack. 

       	   JackoAudioOutConnect "leftout", "system:playback_1"
	   JackoAudioOutConnect "rightout", "system:playback_2"
	   JackoInfo

	   ; Turning freewheeling on seems automatically 
           ; to turn system playback off. This is good!

	   JackoFreewheel	1
	   JackoOn

	   alwayson		"jackin"

	   instr 1
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
ichannel   =			p1 - 1
itime 	   = 			p2
iduration  = 			p3
ikey 	   = 			p4
ivelocity  = 			p5
	   JackoNoteOut 	"midiout", ichannel, ikey, ivelocity
	   print 		itime, iduration, ichannel, ikey, ivelocity
	   endin

	   instr jackin
	   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	   JackoTransport	3, 1.0
aleft 	   JackoAudioIn		"leftin"
aright 	   JackoAudioIn 	"rightin"

	   ; Aeolus uses MIDI controller 98 to control stops. 
           ; Only 1 data value byte is used, not the 2 data 
	   ; bytes often used  with NRPNs. 
           ; The format for control mode is 01mm0ggg:
	   ; mm 10 to set stops, 0, ggg group (or Division, 0 based).
	   ; The format for stop selection is 000bbbbb:   
	   ; bbbbb for button number (0 based).

	   ; Mode to enable stops for Divison I: b1100010 (98 
           ; [this controller VALUE is a pure coincidence]).

	   JackoMidiOut          "midiout", 176, 0, 98, 98 

	   ; Stops: Principal 8 (0), Principal 4 (1) , Flote 8 (8) , Flote 2 (10)

	   JackoMidiOut          "midiout", 176, 0, 98, 0
	   JackoMidiOut          "midiout", 176, 0, 98, 1
	   JackoMidiOut          "midiout", 176, 0, 98, 8
	   JackoMidiOut          "midiout", 176, 0, 98, 10

	   ; Sends audio coming in from Aeolus out
	   ; not only to the Jack system out (sound card),
	   ; but also to the output soundfile.
           ; Note that in freewheeling mode, "leftout"
           ; and "rightout" simply go silent.

 	   JackoAudioOut 	"leftout", aleft
	   JackoAudioOut 	"rightout", aright
	   outs  		aright, aleft
	   endin

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
'''
        self.model.getCppSound().setCSD(self.csoundOrchestra)
        self.model.setCsoundCommand(self.csoundCommand)
        instruments = self.model.getCppSound().getCsoundFile().getInstrumentNames()
        for number, name in instruments.items():
            print 'Instr %4d: %s' % (number, name)
        print
    def createCsoundArrangement(self):
        print 'CREATING CSOUND ARRANGEMENT...'
        #~ #                 CsoundAC,   Csound,                                                       level (+-dB),  pan (-1.0 through +1.0)
        self.score.setDuration(5 * 60)        
        print
    def testCommand(self, command, reinitialize = True):
        print 'Testing command: %s' % command
        if reinitialize == True:
            self.lindenmayer.turtle.initialize()
        print self.lindenmayer.turtle
        self.lindenmayer.interpret(command)
        print self.lindenmayer.turtle
        print
    def createMusicModel(self):
        print 'UNIT TESTS FOR CsoundAC.ChordLindenmayer...'
        self.lindenmayer = CsoundAC.ChordLindenmayer()
        print self.lindenmayer
        print self.lindenmayer.turtle
        print
        print 'TESTING EACH TURTLE COMMAND...'
        print
        self.testCommand('=CO"CM9"')
        self.testCommand('=NNv60')
        #self.testCommand('=CO(1,2,3,4)')
        self.testCommand('=NNt')
        self.testCommand('=NNt1')
        self.testCommand('=NNt2')
        self.testCommand('=NDd1')
        self.testCommand('=NOk13')
        self.testCommand('=NOk6')
        self.testCommand('=NRk13')
        self.testCommand('=NRk61')
        self.testCommand('=NNk5')
        self.testCommand('-NNk5', False)
        self.testCommand('+NOk13', False)
        self.testCommand('+NOk6', False)
        self.testCommand('+NRk13', False)
        self.testCommand('+NRk61', False)
        self.testCommand('=SNt2')
        self.testCommand('*SNt2.5', False)
        self.testCommand('/SNt2.5', False)
        self.testCommand('+SRk61')
        self.testCommand('+ROtk2.0')
        self.testCommand('VC+')
        self.testCommand('VC+', False)
        self.testCommand('VC-', False)
        self.testCommand('VC-', False)
        self.testCommand('VC-')
        self.testCommand('=CO"CM"')
        self.testCommand('=MO"CM"', False)
        self.testCommand('QN2', False)
        self.testCommand('TN2', False)
        self.testCommand('IN6', False)
        self.testCommand('KN', False)
        print
        print 'TESTING NOTE TIEING...'
        print
        self.testCommand('=CO"CM9"')
        self.testCommand('=MO"CM9"', False)
        self.testCommand('=NNd2', False)
        self.testCommand('=NNv60', False)
        self.testCommand('WN', False)
        self.testCommand('=NNk3', False)
        print self.lindenmayer.score.toString()
        self.testCommand('+NNt1', False)
        self.testCommand('WN', False)
        self.testCommand('+NNt1', False)
        self.testCommand('WN', False)
        print self.lindenmayer.score.toString()
        self.lindenmayer.tieOverlappingNotes()
        self.lindenmayer.fixStatus()
        print self.lindenmayer.score.toString()
        print
        print 'TESTING CHORD WRITING...'
        print
        self.testCommand('=CO"CM9"')
        self.testCommand('=MO"CM9"', False)
        self.testCommand('=NNd2', False)
        self.testCommand('=NNv60', False)
        self.testCommand('WN', False)
        self.testCommand('=NNk3', False)
        print self.lindenmayer.score.toString()
        self.testCommand('+NNt1', False)
        self.testCommand('WN', False)
        self.testCommand('+NNt1', False)
        self.testCommand('WCV', False)
        self.testCommand('+NNt1', False)
        self.testCommand('WCNV', False)
        print self.lindenmayer.score.toString()
        self.testCommand('+NNt1', False)
        self.testCommand('WCV', False)
        self.lindenmayer.tieOverlappingNotes()
        self.lindenmayer.fixStatus()
        print self.lindenmayer.score.toString()
        print
        print 'TESTING CHORD VOICING...'
        print
        self.testCommand('=CO"CM9"')
        for i in xrange(20):
            self.testCommand('+NNt2', False)
            self.testCommand('+V1', False)
            self.testCommand('WCV', False)
        self.lindenmayer.fixStatus()
        print self.lindenmayer.score.toString()
        self.lindenmayer.tieOverlappingNotes()
        print self.lindenmayer.score.toString()
        print
        print 'TESTING APPLICATION OPERATIONS...'
        print
        self.testCommand('=CO"CM9"')
        for i in xrange(20):
            self.testCommand('+NNt2', False)
            self.testCommand('+V1', False)
            self.testCommand('WCV', False)
        self.testCommand('AC', False)
        for i in xrange(20):
            self.testCommand('+NNt2', False)
            self.testCommand('+V1', False)
            self.testCommand('WCV', False)
        self.testCommand('A0', False)
        for i in xrange(20):
            self.testCommand('+NNt2', False)
            self.testCommand('+V1', False)
            self.testCommand('WCV', False)
        self.lindenmayer.fixStatus()
        print self.lindenmayer.score.toString()
        self.lindenmayer.tieOverlappingNotes()
        self.lindenmayer.applyVoiceleadingOperations()
        self.lindenmayer.tieOverlappingNotes()
        print self.lindenmayer.score.toString()
        print
        print 'TESTING STEP AND ORIENTATION...'
        print
        self.testCommand('ROtk0.5')
        self.testCommand('F1', False)
        self.testCommand('F1', False)
        self.testCommand('F2', False)
        self.testCommand('ROkt0.5', False)
        self.testCommand('F1', False)
        self.testCommand('F1', False)
        self.testCommand('F2', False)
        exit(0)
        print
        print 'TESTING SCORE GENERATION...'
        print
        self.lindenmayer.axiom = '=NNd1 =NNv60 =CO"CM7" =MO"CM7" [ a ] +NNt1.5 +NRk7 b'
        self.lindenmayer.rules['a'] = 'WN a +NNt1 +NRk1 WN [ +NRk5 c ] a +NNt1 -NRk1 +NNt1 WN [ +NRk12 c ] a'
        self.lindenmayer.rules['b'] = 'AC K Q5 WN b +NNt1 +NRk1 WN [ +NRk5 c ] b +NNt1 -NRk1 +NNt1 WN [ +NRk12 c ] b'
        self.lindenmayer.rules['c'] = '*SNt.95 *SNd.95 *SRk1.02 +NNt0.5 WN c +NNt1 +NRk1 WN [ +NRk5 c ] c +NNt1 -NRk1 +NNt1 WN c'
        self.lindenmayer.iterationCount = 6
        self.lindenmayer.generate()
        print self.lindenmayer.score.toString()
        print
        self.aeolus = subprocess.Popen(string.split('aeolus -t'))
        time.sleep(1.0)
        self.rescale = CsoundAC.Rescale()
        #self.rescale.setRescale( CsoundAC.Event.TIME,       True, False, (1.0 / 40.0), 120     )
        self.rescale.setRescale( CsoundAC.Event.INSTRUMENT, True, True,  1,              0     )
        self.rescale.setRescale( CsoundAC.Event.KEY,        True, False, 36,            36     )
        self.rescale.setRescale( CsoundAC.Event.VELOCITY,   True, True,  80,            40     )
        self.rescale.setRescale( CsoundAC.Event.PAN,        True, True,  -0.9,           1.8   )
        self.rescale.addChild(self.lindenmayer)
        self.model.addChild(self.rescale)
        print

csoundComposition = CsoundComposition()
if len(sys.argv) > 1:
    csoundComposition.renderingMode = sys.argv[1]
else:
    csoundComposition.renderingMode = 'cd'
csoundComposition.dacName = 'dac'
csoundComposition.playback = True
csoundComposition.render()
try:
    time.sleep(1.0)
    csoundComposition.aeolus.kill()
    print 'Killed aeolus...'
    print
except:
    traceback.print_exc()
