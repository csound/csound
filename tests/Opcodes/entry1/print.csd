<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac    ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o print.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

irand = rnd(3)			;generate a random number from 0 to 3
print irand			;print it
asig  poscil .7, 440*irand, 1
      outs asig, asig

endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1	;sine wave

i 1 0 1
i 1 2 1
i 1 4 1
e
</CsScore>
</CsoundSynthesizer>
