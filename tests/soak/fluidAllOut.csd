<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0    ;;;realtime audio out and realtime midi in
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o fluidAllOut.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giengine1 fluidEngine
isfnum1	  fluidLoad "sf_GMbank.sf2", giengine1, 1
	  fluidProgramSelect giengine1, 1, isfnum1, 0, 0

giengine2 fluidEngine
; soundfont path to manual/examples
isfnum2	  fluidLoad "22Bassoon.sf2", giengine2, 1
	  fluidProgramSelect giengine2, 1, isfnum2, 0, 70

instr 1

     mididefault   60, p3
     midinoteonkey p4, p5
ikey init p4
ivel init p5
     fluidNote giengine1, 1, ikey, ivel

endin

instr 2

     mididefault   60, p3
     midinoteonkey p4, p5
ikey init p4
ivel init p5
     fluidNote giengine2, 1, ikey, ivel

endin

instr 100

imvol init 7 ;amplify a bit
asigl, asigr fluidAllOut
      outs asigl*imvol, asigr*imvol

endin
</CsInstruments>
<CsScore>

i 1 0 2 60 127 ;play one note on instr 1
i 2 2 2 60 127 ;play another note on instr 2 and...
i 100 0 60     ;play virtual midi keyboard
e
</CsScore>
</CsoundSynthesizer> 
