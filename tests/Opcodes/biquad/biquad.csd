<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o biquad.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Instrument #1.
instr 1
  ; Get the values from the score.
  idur = p3
  iamp = p4
  icps = cpspch(p5)
  kfco = p6
  krez = p7

  ; Calculate the biquadratic filter's coefficients 
  kfcon = 2*3.14159265*kfco/sr
  kalpha = 1-2*krez*cos(kfcon)*cos(kfcon)+krez*krez*cos(2*kfcon)
  kbeta = krez*krez*sin(2*kfcon)-2*krez*cos(kfcon)*sin(kfcon)
  kgama = 1+cos(kfcon)
  km1 = kalpha*kgama+kbeta*sin(kfcon)
  km2 = kalpha*kgama-kbeta*sin(kfcon)
  kden = sqrt(km1*km1+km2*km2)
  kb0 = 1.5*(kalpha*kalpha+kbeta*kbeta)/kden
  kb1 = kb0
  kb2 = 0
  ka0 = 1
  ka1 = -2*krez*cos(kfcon)
  ka2 = krez*krez
  
  ; Generate an input signal.
  axn vco 1, icps, 1

  ; Filter the input signal.
  ayn biquad axn, kb0, kb1, kb2, ka0, ka1, ka2
  outs ayn*iamp/2, ayn*iamp/2
endin


</CsInstruments>
<CsScore>

; Table #1, a sine wave.
f 1 0 16384 10 1

;    Sta  Dur  Amp    Pitch Fco   Rez
i 1  0.0  1.0  20000  6.00  1000  .8
i 1  1.0  1.0  20000  6.03  2000  .95
e


</CsScore>
</CsoundSynthesizer>
