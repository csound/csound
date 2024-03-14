<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d         -M0  -Q1;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Example by Giorgio Zucco 2007


instr 1  ;Triggered by MIDI notes on channel 1

  inote notnum
  ivel 	veloc

  kpitch = 40
  kfreq  = 2

  kdur   =  .04
  kpause =  .1

  k1 	lfo 	kpitch, kfreq,5

  ;plays a stream of notes of kdur duration on MIDI channel 1
  moscil  1, inote + k1, ivel,   kdur, kpause

endin

</CsInstruments>
<CsScore>
; Dummy ftable
f0 60
</CsScore>
</CsoundSynthesizer>









