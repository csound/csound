<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;real-time audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mvmfilter.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr=44100
ksmps=32
0dbfs=1
nchnls=2

gaverb init 0

instr highQResonator
  ap mpulse .5, 0
  a1 mvmfilter ap, p4, .25
  out a1, a1
endin
  
instr dustyResonator
  ap dust .1, 30
  a1 mvmfilter ap, p4, .15
  out a1, a1
endin

instr oscillator
  ap mpulse .5, 0
  kenv madsr .1, .2, .6, .4
  a1 mvmfilter ap, p4, 1e6
  a1 *= kenv
  out a1, a1
endin

instr resonatorBank1
  ap mpulse .5, 0
  kampenv madsr .01, .2, .6, .4
  kenv init 1
  iDecayTime init 1
  a1 mvmfilter ap, p4*1*kenv, iDecayTime 
  a2 mvmfilter ap, p4*3*kenv, iDecayTime*.8
  a3 mvmfilter ap, p4*5*kenv, iDecayTime*.4
  a4 mvmfilter ap, p4*7*kenv, iDecayTime*.3
  a5 mvmfilter ap, p4*9*kenv, iDecayTime*.2
  a6 mvmfilter ap, p4*11*kenv, iDecayTime*.1
  aout = (a1+a2+a3+a4+a5+a6) * kampenv * (.1667)
  gaverb += aout * .3 
  out aout, aout
endin
  
instr resonatorBank2
  ap noise 0.005, 0
  kampenv madsr .01, .2, .6, .4
  kenv init 1
  iDecayTime init 1
  a1 mvmfilter ap, p4*1*kenv, iDecayTime
  a2 mvmfilter ap, p4*3*kenv, iDecayTime*.8
  a3 mvmfilter ap, p4*5*kenv, iDecayTime*.5
  a4 mvmfilter ap, p4*7*kenv, iDecayTime*.4
  a5 mvmfilter ap, p4*9*kenv, iDecayTime*.3
  a6 mvmfilter ap, p4*11*kenv, iDecayTime*.2
  aout = (a1+a2+a3+a4+a5+a6) * kampenv * (.1667)
  gaverb += aout * .3 
  out aout, aout
endin

instr harmonicArp
  avco vco2 .01, 50
  avco moogladder2 avco, 3000, .1
  kenv linseg 1.3,p3,2
  kampenv madsr 4, .1, 1, .4
  iDecayTime init .02
  a1 mvmfilter avco, p4*1*kenv, iDecayTime
  a2 mvmfilter avco, p4*2*kenv, iDecayTime
  a3 mvmfilter avco, p4*3*kenv, iDecayTime
  a4 mvmfilter avco, p4*4*kenv, iDecayTime
  a5 mvmfilter avco, p4*5*kenv, iDecayTime
  a6 mvmfilter avco, p4*6*kenv, iDecayTime
  aout = (a1+a2+a3+a4+a5+a6) * kampenv * .3
  aout tanh aout
  gaverb += aout*.3
  out aout, aout
endin

instr reverb
  adel	init 0
  ain = gaverb
  aleftout, arightout reverbsc ain, ain, .91, 12000
  outs    aleftout, arightout
  gaverb = 0
endin

</CsInstruments>
<CsScore>
s
; mvmfilter is basically a damped resonator
i "highQResonator" 	0 1 220

; putting some 'dust' through it
i "dustyResonator" 	2 2 300

; with a large time-constant it becomes an oscillator
i "oscillator" 		4 3 440

s
; It works for a modal synthesis type use
i "resonatorBank1" 	0 5 50
i "resonatorBank1" 	2 5 100
i "resonatorBank1" 	4 5 150

s
; and some slightly more interesting effects...
i "resonatorBank2" 	0 5 50
i "resonatorBank2" 	2 5 125
i "harmonicArp" 	4 12 100
i "resonatorBank2" 	7 5 50
i "resonatorBank2" 	10 5 180
i "reverb"		0 21


</CsScore>
</CsoundSynthesizer>
