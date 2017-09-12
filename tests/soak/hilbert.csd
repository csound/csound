<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o hilbert.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
  
instr 1
  idur = p3
  ; Initial amount of frequency shift.
  ; It can be positive or negative.
  ibegshift = p4 
  ; Final amount of frequency shift.
  ; It can be positive or negative.
  iendshift = p5 
  
  ; A simple envelope for determining the 
  ; amount of frequency shift.
  kfreq linseg ibegshift, idur, iendshift
 
  ; Use the sound of your choice.
  ain diskin2 "beats.wav", 1, 0, 1
 
  ; Phase quadrature output derived from input signal.
  areal, aimag hilbert ain
 
  ; Quadrature oscillator.
  asin oscili 1, kfreq, 1
  acos oscili 1, kfreq, 1, .25
 
  ; Use a trigonometric identity. 
  ; See the references for further details.
  amod1 = areal * acos
  amod2 = aimag * asin

  ; Both sum and difference frequencies can be 
  ; output at once.
  ; aupshift corresponds to the sum frequencies.
  aupshift = (amod1 - amod2) * 0.7
  ; adownshift corresponds to the difference frequencies. 
  adownshift = (amod1 + amod2) * 0.7

  ; Notice that the adding of the two together is
  ; identical to the output of ring modulation.

  outs aupshift, aupshift
endin


</CsInstruments>
<CsScore>

; Sine table for quadrature oscillator.
f 1 0 16384 10 1

; Starting with no shift, ending with all
; frequencies shifted up by 2000 Hz.
i 1 0 6 0 2000

; Starting with no shift, ending with all
; frequencies shifted down by 250 Hz.
i 1 7 6 0 -250
e


</CsScore>
</CsoundSynthesizer>
