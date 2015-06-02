'''
D R O N E
INSPIRED BY THE WORK OF LAMONTE YOUNG
Copyright (C) 2005 by Michael Gogins.
All rights reserved.
This software is licensed under the terms of the
GNU Lesser General Public License.

This composition simulates some of the effects in
LaMonte Young's early electronic pieces, using Csound.
The piece uses just intonation chords without thirds or sixths,
tones with varying harmonic content,
tones with waveshaping distortion,
and varying amounts of reverberation
to produce a slow progression of rich drones.

The purpose of the GUI is to enable the user to interactively fine-tune
and save the configuration of the piece.

TO DO

Snapshot buttons instead of file dialogs.
Multiple rendering choices (done).
Indicator/controller slider for score position.
Indicate snapshot in filenames.
Use a toolkit that enables stopping/restarting without crashing.
'''
import 	csnd6
import 	os
import 	pickle
import 	shutil
import 	traceback

'''
Psyco (http://www.psyco.org) is a just-in-time compiler for Python.
One imagines that using Psyco here will make execution
faster and therefore smoother.
'''
try:
	import psyco
	print 'Imported pscyo.'
except:
	print 'Failed to import psyco.'

from 	Tkinter import *
import 	tkFileDialog
import 	Tix

print __doc__
'''
A class that contains all control channel configuration values,
in order to simplify saving and restoring configurations.
Naming convention:
Csound global variable name equals Csound control channel name,
equals Tkinter widget name, equals configuration variable name,
and the widget command handler name is the same but prefixed with "on_".
'''
class Configuration(object):
	def __init__(self):
		self.gkHarmonicTableFactor = 0.5
		self.gkDistortTableFactor = 0.5
		self.gkDistortFactor = 0.125
		self.gkReverbscFeedback = 0.8
		self.gkMasterLevel = 0.25
		self.output = 'dac'

print
print 'CREATING FILENAMES FOR THIS PIECE...'
print
scriptFilename = sys.argv[0]
print 'Full Python script: %s' % scriptFilename
title = os.path.basename(scriptFilename)
print 'Base Python script: %s' % title
orcFilename = scriptFilename + '.orc'
print 'Csound orchestra:   %s' % orcFilename
scoFilename = scriptFilename + '.sco'
print 'Csound score:       %s' % scoFilename
midiFilename = scriptFilename + '.mid'
print 'MIDI filename:      %s' % midiFilename
soundfileName = scriptFilename + '.wav'
print 'Soundfile name:     %s' % soundfileName
dacName = 'dac'
print 'DAC name:           %s' % dacName
soundfilePlayer = r'D:/utah/opt/Audacity/audacity.exe'
print 'Soundfile player:   %s' % soundfilePlayer
author = 'Michael_Gogins'
print 'Author:             %s' % author
#archiveDirectory = 'd:/utah/home/mkg/projects/music/__latest'
#archivedFilename = os.path.normpath(os.path.join(archiveDirectory, title))
#latestFilename = os.path.normpath(os.path.join(archiveDirectory, 'latest.py'))
#shutil.copyfile(scriptFilename, archivedFilename)
#print 'Archived as:        %s' % archivedFilename
#shutil.copyfile(scriptFilename, latestFilename)
#print 'Also as:            %s' % latestFilename
print



csoundOrchestra = '''\
sr                      =           44100
ksmps                   =           441
nchnls                  =           2
0dbfs                   =           40000

; Bind named control channels to global variables.

gkDistortFactor         init        1.0
gkReverbscFeedback      init        0.9
gkMasterLevel           init        1.0

gareverb1               init        0
gareverb2               init        0

			instr 1
iattack                 init        40.0
idecay                  init        40.0
isustain        	init        p3 - iattack
p3          		=           p3 + idecay
			print       p1, p2, p3, p4, p5, p6
ifundamental            =           p4
inumerator              =           p5
idenominator            =           p6
ivelocity               =           p7
ipan                    =           p8
iratio                  =           inumerator / idenominator
ihertz                  =           ifundamental * iratio
iamp                    =           ampdb(ivelocity)
kenvelope               transeg     0.0, iattack / 2.0, 2.5, iamp / 2.0, iattack / 2.0, -2.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 2.5, iamp / 2.0, idecay / 2.0, -2.5, 0.0
asignal                 poscil3     kenvelope, ihertz, 1
asignal                 distort     asignal, gkDistortFactor, 2
aleft, aright           pan2        asignal , ipan
gareverb1               =           gareverb1 + aleft
gareverb2               =           gareverb1 + aright
			endin


			instr 30
aleft, aright           reverbsc    gareverb1, gareverb2, gkReverbscFeedback, 15000.0
aleft                   =           gkMasterLevel * (gareverb1 + aleft * 0.8)
aright                  =           gkMasterLevel * (gareverb2 + aright * 0.8)
			outs        aleft, aright
gareverb1               =           0
gareverb2               =           0
			endin

			instr 40
; p4 = gain of reverb. Adjust empirically
; for desired reverb time. .6 gives
; a good small "live" room sound, .8
; a small hall, .9 a large hall,
; .99 an enormous stone cavern.

; p5 = amount of random pitch modulation
; for the delay lines. 1 is the "normal"
; amount, but this may be too high for
; held pitches such as piano tones.
; Adjust to taste.

; p6 = cutoff frequency of lowpass filters
; in feedback loops of delay lines,
; in Hz. Lower cutoff frequencies results
; in a sound with more high-frequency
; damping.

; 8 delay line FDN reverb, with feedback matrix based upon
; physical modeling scattering junction of 8 lossless waveguides
; of equal characteristic impedance. Based on Julius O. Smith III,
; "A New Approach to Digital Reverberation using Closed Waveguide
; Networks," Proceedings of the International Computer Music
; Conference 1985, p. 47-53 (also available as a seperate
; publication from CCRMA), as well as some more recent papers by
; Smith and others.
; Coded by Sean Costello, October 1999
ipitchmod 		= 			0.98
itone 			= 			16000
ain1 			=			gareverb1
ain2 			=			gareverb2
asignal 		= 			(ain1 + ain2) * 0.5
afilt1 			init 			0
afilt2 			init 			0
afilt3 			init 			0
afilt4 			init 			0
afilt5 			init 			0
afilt6 			init 			0
afilt7 			init 			0
afilt8 			init 			0
idel1 			= 			(2473.000/sr)
idel2 			= 			(2767.000/sr)
idel3 			= 			(3217.000/sr)
idel4 			= 			(3557.000/sr)
idel5 			= 			(3907.000/sr)
idel6 			= 			(4127.000/sr)
idel7 			= 			(2143.000/sr)
idel8 			= 			(1933.000/sr)
; k1-k8 are used to add random pitch modulation to the
; delay lines. Helps eliminate metallic overtones
; in the reverb sound.
k1      		randi   		.001, 3.1, .06
k2      		randi   		.0011, 3.5, .9
k3      		randi   		.0017, 1.11, .7
k4      		randi   		.0006, 3.973, .3
k5      		randi   		.001, 2.341, .63
k6      		randi   		.0011, 1.897, .7
k7      		randi   		.0017, 0.891, .9
k8      		randi   		.0006, 3.221, .44
; apj is used to calculate "resultant junction pressure" for
; the scattering junction of 8 lossless waveguides
; of equal characteristic impedance. If you wish to
; add more delay lines, simply add them to the following
; equation, and replace the .25 by 2/N, where N is the
; number of delay lines.
apj 			= 			.25 * (afilt1 + afilt2 + afilt3 + afilt4 + afilt5 + afilt6 + afilt7 + afilt8)
adum1   		delayr  		1
adel1   		deltapi 		idel1 + k1 * ipitchmod
				delayw  		asignal + apj - afilt1
adum2   		delayr  		1
adel2   		deltapi 		idel2 + k2 * ipitchmod
				delayw  		asignal + apj - afilt2
adum3   		delayr  		1
adel3   		deltapi 		idel3 + k3 * ipitchmod
				delayw  		asignal + apj - afilt3
adum4   		delayr  		1
adel4   		deltapi 		idel4 + k4 * ipitchmod
				delayw  		asignal + apj - afilt4
adum5   		delayr  		1
adel5   		deltapi 		idel5 + k5 * ipitchmod
				delayw  		asignal + apj - afilt5
adum6   		delayr  		1
adel6   		deltapi 		idel6 + k6 * ipitchmod
				delayw  		asignal + apj - afilt6
adum7   		delayr  		1
adel7   		deltapi 		idel7 + k7 * ipitchmod
				delayw  		asignal + apj - afilt7
adum8   		delayr  		1
adel8   		deltapi 		idel8 + k8 * ipitchmod
				delayw  		asignal + apj - afilt8
; 1st order lowpass filters in feedback
; loops of delay lines.
afilt1  		tone    		adel1 * gkReverbscFeedback, itone
afilt2  		tone    		adel2 * gkReverbscFeedback, itone
afilt3  		tone    		adel3 * gkReverbscFeedback, itone
afilt4  		tone    		adel4 * gkReverbscFeedback, itone
afilt5  		tone    		adel5 * gkReverbscFeedback, itone
afilt6  		tone    		adel6 * gkReverbscFeedback, itone
afilt7  		tone    		adel7 * gkReverbscFeedback, itone
afilt8  		tone    		adel8 * gkReverbscFeedback, itone
; The outputs of the delay lines are summed
; and sent to the stereo outputs. This could
; easily be modified for a 4 or 8-channel
; sound system.
aout1 			= 			(afilt1 + afilt3 + afilt5 + afilt7)
aout2 			= 			(afilt2 + afilt4 + afilt6 + afilt8)
; To the master output.
aleft                   =           gkMasterLevel * (gareverb1 + aout1 * 0.8)
aright                  =           gkMasterLevel * (gareverb2 + aout2 * 0.8)
			outs        aleft, aright
gareverb1               =           0
gareverb2               =           0
			endin

instr 50
gkHarmonicTableFactor chnget "gkHarmonicTableFactor"
gkDistortTableFactor chnget "gkDistortTableFactor"
gkDistortFactor chnget "gkDistortFactor"
gkReverbscFeedback chnget "gkReverbscFeedback"
gkMasterLevel chnget "gkMasterLevel"
endin

'''

csoundScore = '''\
;                       A few harmonics...
f   1       0           65536       10      3   0   1   0   0   2
;                       ...distorted by waveshaping.
f   2       0           65536       13      1   1   0   3   0   2
;                       Change the tempo, if you like.
t   0      30

;   p1      p2      p3          p4              p5         p6           p7        p8
;   insno   onset   duration    fundamental     numerator  denominator  velocity  pan

; What I want here is just intonation C major 7, G major 7, G 7, C major with voice leading.

i   1       0       60          60              1          1            60       -0.875
i   1       0      180          60              3          2            60        0.000
i   1       0       60          60             28         15            60        0.875

i   1       60      60          60              9          8            60        0.875
i   1       60      30          60              10         7            64       -0.875
i   1       90      30          60              4          3            68        0.875

i   1       120     60          60          	1          1            64       -0.875
i   1       120     60          60          	5          4            62        0.000
i   1       120     60          60         	    2          1            58        0.875

i   30      0       -1

i   50      0       -1

s   10.0
e   10.0
'''

csoundCommands = {
	'Audio':    'csound --messagelevel=3 --noheader                         --nodisplays --sample-rate=44100 --control-rate=100   --midi-key=4 --midi-velocity=5                                                                  --output=%s' % (			   dacName),
	'Preview':  'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=44100 --control-rate=441   --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s' % (author, author, title, soundfileName),
	'CD':       'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=44100 --control-rate=44100 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s' % (author, author, title, soundfileName),
	'Master':   'csound --messagelevel=3 -W -f --rewrite --dither --nopeaks --nodisplays --sample-rate=88200 --control-rate=88200 --midi-key=4 --midi-velocity=5 -+id_artist=%s -+id_copyright=Copyright_2007_by_%s -+id_title=%s --output=%s' % (author, author, title, soundfileName)
}

'''
A simple Tkinter GUI with one slider
per orchestra control channel.
'''
class Application(Frame):
	def __init__(self, master, thread_ = False):
		Frame.__init__(self, master)
		self.master = master
		self.master.title('D R O N E')
		self.pack()
		self.csound = csnd6.CppSound()
		#self.csound.setPythonMessageCallback()
		self.playing = False
		self.thread_ = thread_

		self.configuration = Configuration()

		r = 0
		c = 0

		self.leftFrame = Frame(self)
		self.leftFrame.grid(row=r, column=c, sticky=N+E+W, padx=4, pady=4)

		self.harmonicsFrame = Frame(
		self.leftFrame,
		bd = 2,
		relief = 'groove'
		)
		self.harmonicsFrame.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)

		self.harmonicsLabel = Label(
		self.harmonicsFrame,
		text = 'Oscillator harmonic content'
		)
		self.harmonicsLabel.grid(row=r, column=c, sticky=N+E+W+S)
		r = r + 1

		self.gkHarmonicTableFactor = Scale(
		self.harmonicsFrame,
		from_ = 0.0,
		to = 2.0,
		resolution = 0.001,
		length = 250,
		orient = HORIZONTAL,
		label = 'Wavetable harmonics multiplier',
		command = self.on_gkHarmonicTableFactor
		)
		self.gkHarmonicTableFactor.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.distortFrame = Frame(
		self.leftFrame,
		bd = 2,
		relief = 'groove'
		)
		self.distortFrame.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)

		self.distortLabel = Label(
		self.distortFrame,
		text = 'Waveshaping distortion'
		)
		self.distortLabel.grid(row=r, column=c, sticky=N+E+W+S)
		r = r + 1

		self.gkDistortTableFactor = Scale(
		self.distortFrame,
		from_ = 0.0,
		to = 10.0,
		resolution = 0.001,
		length = 250,
		orient = HORIZONTAL,
		label = 'Distortion table multiplier',
		command = self.on_gkDistortTableFactor
		)
		self.gkDistortTableFactor.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.gkDistortFactor = Scale(
		self.distortFrame,
		from_ = 0.0,
		to = 1.0,
		resolution = 0.001,
		length = 250,
		orient = HORIZONTAL,
		label = 'Distortion gain',
		command = self.on_gkDistortFactor
		)
		self.gkDistortFactor.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		r = 0
		c = 1

		self.rightFrame = Frame(self)
		self.rightFrame.grid(row=r, column=c, sticky=N+E+W, padx=4, pady=4)

		self.reverbFrame = Frame(
		self.rightFrame,
		bd = 2,
		relief = 'groove'
		)
		self.reverbFrame.grid(row=r, column=c, sticky=N+E+W, padx=4, pady=4)

		self.reverbLabel = Label(
		self.reverbFrame,
		text = 'Reverbsc'
		)
		self.reverbLabel.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.gkReverbscFeedback = Scale(
		self.reverbFrame,
		from_ = 0.0,
		to = 0.9999,
		resolution = 0.001,
		length = 250,
		orient = HORIZONTAL,
		label = 'Feedback (reverberation time)',
		command = self.on_gkReverbscFeedback
		)
		self.gkReverbscFeedback.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.FactorFrame = Frame(
		self.rightFrame,
		bd = 2,
		relief = 'groove'
		)
		self.FactorFrame.grid(row=r, column=c, sticky=N+E+W, padx=4, pady=4)

		self.FactorLabel = Label(
		self.FactorFrame,
		text = 'Master gain'
		)
		self.FactorLabel.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.gkMasterLevel = Scale(
		self.FactorFrame,
		from_ = 0.0,
		to = 2.0,
		resolution = 0.001,
		length = 250,
		orient = HORIZONTAL,
		label = 'Level',
		command = self.on_gkMasterLevel
		)
		self.gkMasterLevel.grid(row=r, column=c, sticky=E+W)
		r = r + 1

		self.scoreTimeFrame = Frame(
		self.rightFrame,
		bd = 2,
		relief = 'groove'
		)
		self.scoreTimeFrame.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)

		self.scoreTimeLabel = Label(
		self.scoreTimeFrame,
		text = 'Score time'
		)
		self.scoreTimeLabel.grid(row=r, column=c, sticky=N+E+W+S)
		r = r + 1

		self.scoreTimeDoubleVar = DoubleVar()
		self.scoreTimeEntry = Entry(
		self.scoreTimeFrame,
		width = 40,
		textvariable = self.scoreTimeDoubleVar
		)
		self.scoreTimeEntry.grid(row=r, column=c, sticky=N+E+W+S)
		r = r + 1

		self.outputFrame = Frame(
		self.rightFrame,
		bd = 2,
		relief = 'groove'
		)
		self.outputFrame.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)

		self.outputLabel = Label(
		self.outputFrame,
		text = 'Output'
		)
		self.outputLabel.grid(row=r, column=c, sticky=N+E+W+S)
		r = r + 1

		self.outputStringVar = StringVar()
		self.output = Tix.ComboBox(
		self.outputFrame,
		listwidth = 80,
		)
		self.output.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)
		choices = csoundCommands.keys()
		choices.sort()
		index = 0
		for choice in choices:
			self.output.insert(index, choice)
			index = index + 1
		self.output.pick(0)
		r = r + 1

		self.loadButton = Button(
		self.rightFrame,
		text = 'Load',
		command = self.on_load
		)
		self.loadButton.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)
		r = r + 1

		self.saveButton = Button(
		self.rightFrame,
		text = 'Save',
		command = self.on_save
		)
		self.saveButton.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)
		r = r + 1

		self.playButton = Button(
		self.rightFrame,
		text = 'Play',
		command = self.on_play
		)
		self.playButton.grid(row=r, column=c, sticky=N+E+W+S, padx=4, pady=4)
		r = r + 1

		self.configure()

	'''
	Set initial control channel values.
	'''
	def configure(self):
		self.gkHarmonicTableFactor.set(self.configuration.gkHarmonicTableFactor)
		self.gkDistortTableFactor.set(self.configuration.gkDistortTableFactor)
		self.gkDistortFactor.set(self.configuration.gkDistortFactor)
		self.gkReverbscFeedback.set(self.configuration.gkReverbscFeedback)
		self.gkMasterLevel.set(self.configuration.gkMasterLevel)
		self.outputStringVar.set(self.configuration.output)

	'''
	Play if stopped,
	and stop if playing.
	'''
	def on_play(self):
		if not self.playing:
			self.playButton['text'] = 'Stop'
			self.playing = True
			try:
				print 'Started playing...'
				self.csound.setOrchestra(csoundOrchestra)
				self.csound.setScore(csoundScore)
				output = self.output.cget('value')
				csoundCommand = csoundCommands[output]
				print csoundCommand
				self.csound.setCommand(csoundCommand)
				self.csound.compile()
				# Apply initial control channel values before actually starting synthesis.
				f = self.configuration.gkHarmonicTableFactor
				message = 'f  1  0  65536  10  3  0  %f  0  0 %f\n' % (f * 1.0, f * 2.0)
				self.csound.inputMessage(message)
				f = self.configuration.gkDistortTableFactor
				message = 'f  2  0  65536  13  1  %f  0  %f  0  %f\n' % (f * 1.0, f * 2.0, f * 3.0)
				self.csound.inputMessage(message)
				self.csound.SetChannel("gkDistortFactor",           float(self.gkDistortFactor.get()))
				self.csound.SetChannel("gkReverbscFeedback",        float(self.gkReverbscFeedback.get()))
				self.csound.SetChannel("gkMasterLevel",             float(self.gkMasterLevel.get()))
				# Tkinter only likes 1 thread per application.
				# So, we hack the rules and switch back and forth between
				# computing sound and handling GUI events.
				# When the user closes the application, self.update will raise
				# an exception because the application has been destroyed.
				kperiod = 1
				while self.playing and not self.csound.PerformKsmps():
					kperiod = kperiod + 1
					if kperiod % 10000 == 0:
						scoreTime = self.csound.GetScoreTime()
						self.scoreTimeDoubleVar.set(float(scoreTime))
					try:
						self.update()
					except TclError:
						traceback.print_exc()
						self.playing = False
				self.csound.Reset()
				self.playButton['text'] = 'Play'
				self.playing = False
				if output != 'Audio':
					print 'OPENING SOUNDFILE FOR PLAYBACK...'
					print
					print 'Soundfile:             "%s"' % soundfileName
					os.spawnl(os.P_NOWAIT, soundfilePlayer, soundfilePlayer, soundfileName)
					print
			except:
				traceback.print_exc()
		else:
			try:
				print 'Stopping...'
				self.playButton['text'] = 'Play'
				self.playing = False
			except:
				print traceback.print_exc()

	def on_load(self):
		try:
			filename =  tkFileDialog.askopenfilename(filetypes=[('Drone files', "*.pickled"), ("All files", "*")])
			picklefile = open(filename, 'rb')
			self.configuration = pickle.load(picklefile)
			picklefile.close()
			self.configure()
			print 'Loaded configuration: "%s".' % filename
			self.master.title('D R O N E   %s' % filename)

		except:
			traceback.print_exc()

	def on_save(self):
		try:
			self.configuration.output = self.outputStringVar.get()
			filename =  tkFileDialog.asksaveasfilename(filetypes=[('Drone files', "*.pickled"), ("All files", "*")])
			picklefile = open(filename, 'wb')
			pickle.dump(self.configuration, picklefile)
			picklefile.close()
			print 'Saved configuration: "%s".' % filename
			self.master.title('D R O N E   %s' % filename)
		except:
			traceback.print_exc()

	def on_gkHarmonicTableFactor(self, value):
		f = float(value)
		self.configuration.gkHarmonicTableFactor = f
		message = 'f  1  0  65536  10  3  0  %f  0  0 %f\n' % (f * 1.0, f * 2.0)
		self.csound.inputMessage(message)

	def on_gkDistortTableFactor(self, value):
		f = float(value)
		self.configuration.gkDistortTableFactor = f
		message = 'f  2  0  65536  13  1  %f  0  %f  0  %f\n' % (f * 1.0, f * 2.0, f * 3.0)
		self.csound.inputMessage(message)

	def on_gkDistortFactor(self, value):
		self.csound.SetChannel("gkDistortFactor", float(value))
		self.configuration.gkDistortFactor = value

	def on_gkReverbscFeedback(self, value):
		self.csound.SetChannel("gkReverbscFeedback", float(value))
		self.configuration.gkReverbscFeedback = value

	def on_gkMasterLevel(self, value):
		self.csound.SetChannel("gkMasterLevel", float(value))
		self.configuration.gkMasterLevel = value

try:
	tk = Tix.Tk()
	application = Application(tk)
	application.mainloop()
	# Before the application exits, its reference to Csound
	# must be nulled, so that Csound is not deleted twice
	# (once by Tkinter and once by the Python garbage collector).
	application.csound = None
except:
	traceback.print_exc()
