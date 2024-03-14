<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   -+rtmidi=virtual  -M0    ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o fluidLoad.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

giengine fluidEngine
; soundfont path to manual/examples
isfnum	 fluidLoad "07AcousticGuitar.sf2", giengine, 1
	 fluidProgramSelect giengine, 1, isfnum, 0, 0

instr 1

	mididefault   60, p3
	midinoteonkey p4, p5
ikey	init p4
ivel	init p5
	fluidNote giengine, 1, ikey, ivel

endin

instr 99

imvol  init 7
asigl, asigr fluidOut giengine
       outs asigl*imvol, asigr*imvol

endin
</CsInstruments>
<CsScore>

i 1 0 2 60 100 ;play one note from score and...
i 99 0 60      ;play virtual keyboard for 60 sec.
e

</CsScore>
</CsoundSynthesizer>