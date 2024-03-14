<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o fluidSetInterpMethod.wav -W ;;; for file output any platform
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
iInterpMethod = p6
fluidSetInterpMethod giengine, 1, iInterpMethod
	fluidNote giengine, 1, ikey, ivel

endin

instr 99

imvol  init 7
asigl, asigr fluidOut giengine
       outs asigl*imvol, asigr*imvol

endin
</CsInstruments>
<CsScore>
;hear the difference
i 1 0 2 60 120 0 ;no interpolation
i 1 3 2 72 120 0
i 1 6 2 60 120 7 ;7th order interpolation
i 1 9 2 72 120 7

i 99 0 12    

e
</CsScore>
</CsoundSynthesizer>