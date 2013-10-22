<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpread.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; works with or without -a option when analyzing "fox.wav" from the manual
ilen  filelen "fox.wav"	; length of soundfile
prints "fox.wav = %f seconds\\n",ilen

ktime line 0, p3, p4
krmsr,krmso,kerr,kcps lpread ktime,"fox_poles.lpc"
krmso = krmso*.00007			; low volume
aout  buzz krmso, kcps, 15, 1
krmsr = krmsr*.0001			; low volume
asig  rand krmsr
      outs (aout*2)+asig, (aout*2)+asig	; mix buzz and rand

endin
</CsInstruments>
<CsScore>
; sine
f1 0 4096 10 1 

i 1 0 2.8 1	; first words only
i 1 4 2.8 2.8	; whole sentence
e
</CsScore>
</CsoundSynthesizer>
