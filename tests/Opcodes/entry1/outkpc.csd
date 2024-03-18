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

kprogram init 0

instr 1 ;Triggered by MIDI notes on channel 1

  ifund   notnum
  ivel 	veloc
  idur = 1

; Sends a MIDI program change message according to
; the triggering note's velocity
outkpc     1 ,ivel ,0 ,127

noteondur  1 ,ifund ,ivel ,idur

endin

</CsInstruments>
<CsScore>
; Dummy ftable
f 0 60
</CsScore>
</CsoundSynthesizer>





















