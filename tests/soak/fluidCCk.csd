<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o fluidCCk.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

giengine fluidEngine
; soundfont path to manual/examples
isfnum	 fluidLoad "22Bassoon.sf2", giengine, 1
	 fluidProgramSelect giengine, 1, isfnum, 0, 70

instr 1

	mididefault   60, p3
	midinoteonkey p4, p5
ikey	init p4
ivel	init p5
kpan	line 0, p3, 127 ;panning from left to right
	fluidCCk giengine, 1, 10, kpan ;CC 10 = pan
	fluidNote giengine, 1, ikey, ivel

endin

instr 99

imvol  init 7
asigl, asigr fluidOut giengine
       outs asigl*imvol, asigr*imvol

endin
</CsInstruments>
<CsScore>

i 1 0 4 48 100
i 1 4 2 50 120
i 1 6 1 53 80
i 1 7 1 45 70
i 1 8 1.5 48 80

i 99 0 10      ;keep instr 99 active
e
</CsScore>
</CsoundSynthesizer>