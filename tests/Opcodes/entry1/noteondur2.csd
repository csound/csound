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

instr 1

  ifund   notnum 
  ivel 	veloc
  idur = 1

  ;chord with single key
  noteondur2 	1, ifund,   ivel, idur
  noteondur2 	1, ifund+3, ivel, idur
  noteondur2 	1, ifund+7, ivel, idur
  noteondur2 	1, ifund+9, ivel, idur

endin



</CsInstruments>
<CsScore>
; Dummy ftable
f 0 60
</CsScore>
</CsoundSynthesizer>




















