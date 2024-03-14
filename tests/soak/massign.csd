<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  -+rtmidi=virtual -M0    ;;;realtime audio out and realtime midi in 
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
;-o massign.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giengine fluidEngine
; soundfont path to manual/examples
isfnum	 fluidLoad "19Trumpet.sf2", giengine, 1
	 fluidProgramSelect giengine, 1, isfnum, 0, 56

massign 0,0	;disable triggering of all instruments on all channels, but
massign 12,10	;assign instr. 10 to midi channel 12
massign 3,30	;assign instr. 30 to midi channel 3

instr 10 ; soundfont only on midi channel 12

      mididefault   60, p3
      midinoteonkey p4, p5	; in midi notes
ikey  init p4
ivel  init p5
      fluidNote giengine, 1, ikey, ivel
endin

instr 30 ; FM-oscilator only on midi channel 3

      mididefault   60, p3
      midinoteoncps p4, p5	; in Hertz
icps  init p4
iamp  init p5
iamp  = iamp/127
kenv  madsr  0.5, 0, 1, 0.5
asig  foscil iamp*kenv, icps, 1, 1.414, 2, 1
      outs asig, asig 
endin 

instr 99 ; output sound from fluidengine

imvol init 7
aL, aR fluidOut giengine
      outs aL*imvol, aR*imvol
endin
</CsInstruments>
<CsScore>
; sine
f 1 0 16384 10 1

i 10 0 2 60 100  ;one note on the trumpet in midi and...
i 30 2 2 220 80  ;one FM note in Hz
i 99 0 60	 ;stay active for 60 sec.
e

</CsScore>
</CsoundSynthesizer>