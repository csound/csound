<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o octave.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

iroot = 440		; root note is A above middle-C (440 Hz)
koct  lfo 5, 1, 5	; generate sawtooth, go from 5 octaves higher to root
koc = int(koct)		; produce only whole numbers
kfactor = octave(koc)	; for octave
knew = iroot * kfactor
printk2 knew

asig pluck 1, knew, 1000, 0, 1 
asig dcblock asig	;remove DC
     outs asig, asig

endin
</CsInstruments>
<CsScore>

i 1 0 5
e

</CsScore>
</CsoundSynthesizer>
