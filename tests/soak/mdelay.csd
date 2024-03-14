<CsoundSynthesizer> 
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d         -M0  -Q0 
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

; Example by Giorgio Zucco 2007

instr 1  ;Triggered by MIDI notes on channel 1

  kstatus init 0
  ifund   notnum 	 
  ivel 	veloc 

  noteondur  1, ifund, ivel, 1

  kstatus = kstatus + 1 

  idel1 = .2
  idel2 = .4
  idel3 = .6
  idel4 = .8

  ;make four delay lines

  mdelay 	kstatus,1,ifund+2, ivel,idel1
  mdelay 	kstatus,1,ifund+4, ivel,idel2
  mdelay 	kstatus,1,ifund+6, ivel,idel3
  mdelay 	kstatus,1,ifund+8, ivel,idel4

endin

</CsInstruments>
<CsScore>
; Dummy ftable
f 0 60
</CsScore>
</CsoundSynthesizer>


















