<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -+rtmidi=virtual  -M0    ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o fluidOut.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

giengine1 fluidEngine
; soundfont path to manual/examples
isfnum1	 fluidLoad "01hpschd.sf2", giengine1, 1
	 fluidProgramSelect giengine1, 1, isfnum1, 0, 0

giengine2 fluidEngine
; soundfont path to manual/examples
isfnum2	 fluidLoad "22Bassoon.sf2", giengine2, 1
	 fluidProgramSelect giengine2, 1, isfnum2, 0, 70

instr 1

	mididefault   60, p3
	midinoteonkey p4, p5
ikey	init p4
ivel	init p5
	fluidNote giengine1, 1, ikey, ivel

endin

instr 2

	mididefault   60, p3
	midinoteonkey p4, p5
ikey	init p4
ivel	init p5
	fluidNote giengine2, 1, ikey, ivel

endin

instr 98

imvol  init 7
asigl, asigr fluidOut giengine1
       outs asigl*imvol, asigr*imvol
endin

instr 99

imvol init 4
asigl, asigr fluidOut giengine2		 ;add a stereo flanger
adelL linseg 0, p3*.5, 0.02, p3*.5, 0	 ;max delay time =20ms
adelR linseg 0.02, p3*.5, 0, p3*.5, 0.02 ;max delay time =20ms		
asigL flanger asigl, adelL, .6
asigR flanger asigr, adelR, .6
      outs asigL*imvol, asigR*imvol
endin
</CsInstruments>
<CsScore>

i 1 0 2 60 100 ;play one note of instr 1
i 2 2 2 60 100 ;play another note of instr 2 and...
i 98 0 60      ;play virtual keyboard for 60 sec.
i 99 0 60
e
</CsScore>
</CsoundSynthesizer> 
