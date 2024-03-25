<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=0.95   ;;;realtime audio out, limit loud pops
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpfreson.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021
;do not use -a option when analyzing "fox.wav" with lpfreson as
;it needs a filter coefficient type of file
ires  system_i 1,{{ lpanal -p50 -h200 -P50  -Q15000 -v1 fox.wav fox_coef.lpc }}

instr 1

ilen  filelen "fox.wav"	                  ; length of soundfile
prints "fox.wav = %f seconds\\n",ilen
aref  diskin "fox.wav", 1                 ; don't play this, but use this as an amplitude reference

ktime line 0, p3, p4
krmsr,krmso,kerr,kcps lpread ktime,"fox_coef.lpc"
krmso *= .000007			              ; scale amplitude
asig  buzz krmso, kcps, int(sr/2/kcps), 1 ; max harmonics without aliasing
aout  lpfreson asig, p5                   ; Pole file not supported!!
abal  balance2 aout, aref                 ; use amplitude of diskin as reference       
      outs abal, abal

endin
</CsInstruments>
<CsScore>
; sine
f1 0 4096 10 1
;          dur    ratio
i 1 0 2.8   1       1   ; first words   
i 1 4 2.8   1       .5  ; same, but lower ratio
i 1 8 2.8   2.756   1   ; whole sentence
i 1 12 2.8  2.756   2   ; same, but higher ratio

e
</CsScore>
</CsoundSynthesizer>
