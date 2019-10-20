<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1	; Lo-Fi sound

  kpow	  = 10						;exponent
  kbase	  line 1, p3, 1.4				;vary the base
  kQuantize pow kbase, kpow
  kQuantize = kQuantize*0.5				;half the number of steps for each side of a bipolar signal
  asig	  diskin2 "fox.wav", 1, 0, 1			;loop the fox
  asig	  = round(asig * kQuantize) / kQuantize		;quantize and scale audio signal
  outs asig, asig

endin
</CsInstruments>
<CsScore>

i1 0 19.2

e
</CsScore>
</CsoundSynthesizer>
