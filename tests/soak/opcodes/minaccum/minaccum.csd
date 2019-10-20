<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	;saw

asig  vco2 .2, p4		
      outs  asig, asig				
gasaw = asig
endin

instr 2	;sine

aout  poscil .3, p4, 1		
      outs  aout, aout				
gasin = aout
endin

instr 10

accum init 0	
      minaccum accum, gasaw + gasin		;saw and sine accumulated		
      outs  accum, accum
     		
clear accum
endin

</CsInstruments>
<CsScore>
f 1 0 4096 10 1	

i 1 0 7 330
i 2 3 3 440

i 1 10 7 330	;same notes but without minaccum, for comparison
i 2 13 3 440

i 10 0 6	;accumulation note stops after 6 seconds

e

</CsScore>
</CsoundSynthesizer>
