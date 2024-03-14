<CsoundSynthesizer> 
<CsOptions> 
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d        -M0  -Q1 ;;;RT audio I/O with MIDI in
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Example by Giorgio Zucco 2007

instr 1 ; Triggered by MIDI notes on channel 1

  ivel 	veloc

  krate = 8
  iscale = 100 ;f

  ; Random sequence from table f100
  krnd  randh int(14),krate,-1
  knote table abs(krnd),iscale
  ; Generates random notes from the scale on ftable 100
  ; on channel 1 of csound's MIDI output
  midion 1,knote,ivel

endin

</CsInstruments>
<CsScore>
f100 0 32 -2  40 50 60 70 80 44 54 65 74 84 39 49 69 69

; Dummy ftable
f0 60
</CsScore>
</CsoundSynthesizer>













