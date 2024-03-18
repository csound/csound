<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d         -M0  -Q1 ;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Example by Giorgio Zucco 2007


instr 1 ;Triggered by MIDI notes on channel 1

  ifund notnum
  ivel  veloc

  knote1 init ifund
  knote2 init ifund + 3
  knote3 init ifund + 5

  ;minor chord on MIDI out channel 1
  ;Needs something plugged to csound's MIDI output
  midion 1, knote1,ivel
  midion 1, knote2,ivel
  midion 1, knote3,ivel

endin


</CsInstruments>
<CsScore>
; Dummy ftable
f0 60
</CsScore>
</CsoundSynthesizer>











