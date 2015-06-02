<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;RT audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o distort1.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1


gadist init 0
  
instr 1
  iamp = p4
  ifqc = cpspch(p5)
  asig pluck iamp, ifqc, ifqc, 0, 1
  gadist = gadist + asig
endin
  
instr 50
  kpre init p4
  kpost init p5
  kshap1 init p6
  kshap2 init p7
  aout distort1 gadist, kpre, kpost, kshap1, kshap2, 1

  outs aout, aout

  gadist = 0
endin


</CsInstruments>
<CsScore>

;   Sta  Dur  Amp    Pitch
i1  0.0  3.0  0.5  6.00
i1  0.5  2.5  0.5  7.00
i1  1.0  2.0  0.5  7.07
i1  1.5  1.5  0.5  8.00
  
;   Sta  Dur  PreGain PostGain Shape1 Shape2
i50 0    4      2       .5       0      0
e

</CsScore>
</CsoundSynthesizer>
