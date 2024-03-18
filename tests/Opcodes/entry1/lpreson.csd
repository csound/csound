<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  --limiter=0.95   ;;;realtime audio out, limit loud pops
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lpreson.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by Menno Knevel - 2021

ires1  system_i 1,{{ lpanal -a -p40 -h200 -P50  -Q15000 -v1 fox.wav fox_poles.lpc }}  ; pole filter file      
ires2  system_i 1,{{ lpanal    -p40 -h200 -P50  -Q15000 -v1 fox.wav fox_coeff.lpc }}  ; filter coefficient file

instr 1

ilen  filelen "fox.wav"	                  ; length of soundfile
prints "fox.wav = %f seconds\\n",ilen

ktime line 0, p3, p4
krmsr,krmso,kerr,kcps lpread ktime,"fox_poles.lpc"
krmso *= .000007			  ; scale amplitude
asig  buzz krmso, kcps, int(sr/2/kcps), 1 ; max harmonics without aliasing
aout  lpreson asig                        ; Pole file not supported!!
      outs aout, aout
endin

instr 2

aref  diskin "fox.wav", 1                 ; don't play this, but use this as an amplitude reference
ktime line 0, p3, p4
krmsr,krmso,kerr,kcps lpread ktime,"fox_coeff.lpc"
krmso *= .000007			      ; scale amplitude
asig  buzz krmso, kcps, int(sr/2/kcps), 1 ; max harmonics without aliasing
aout  lpreson asig                        ; Pole file not supported!!
abal  balance2 aout, aref                 ; use amplitude of diskin as reference       
      outs abal, abal
endin

</CsInstruments>
<CsScore>
; sine
f1 0 4096 10 1
s
;          dur
i 1 0 2.8   1      ; first words of fox_poles.lpc 
i 1 4 2.8   2.756  ; whole sentence
s
i 2 0 2.8   1      ; first words of fox_coeff.lpc  
i 2 4 2.8   2.756  ; whole sentence

e
</CsScore>
</CsoundSynthesizer>
