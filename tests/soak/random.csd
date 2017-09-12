<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o random.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;same values every time

krnd random 100, 1000
     printk .5, krnd			; look 
aout oscili 0.8, 440+krnd, 1		; & listen
     outs aout, aout

endin

</CsInstruments>
<CsScore>
f 1 0 16384 10 1	;sine wave.

i 1 0 1
e
</CsScore>
</CsoundSynthesizer>
