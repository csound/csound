<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac   ;;;realtime audio in
; For Non-realtime ouput leave only the line below:
; -o sflooper.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

; by  Menno Knevel - 2021

isf   sfload "07AcousticGuitar.sf2"
      sfpassign 0, isf

instr 1	

inum  = p4
ivel  = p5
kamp  linsegr 1,1,1,.1,0	; declick envelope
kamp  = kamp * .0000015		;scale amplitude
ifreq = 1			;do not change freq from sf
;"07AcousticGuitar.sf2" contains 2 samples, on notes E1 and C#4
;start loop from beginning, loop .2 seconds - on the root key of these samples
aL,aR sflooper ivel, inum, kamp*ivel, ifreq, 0, 0, .2, .05  ; make amp velocity dependent
      outs aL, aR
endin

instr 2	

ifreq  = p4
ivel  = p5
kamp  linsegr 1,1,1,.1,0	; declick envelope
kamp  = kamp * .0000015		;scale amplitude
inum = 60			;take soundfont samples belonging to midi index 60

;nearly identical instr, but now takes midi note sound 60 as reference, set iflag to 1
aL,aR sflooper ivel, inum, kamp*ivel, ifreq, 0, 0, .2, .05, 0, 0, 0, 0, 1  ; & use ifreq for frequency
      outs aL, aR

endin
</CsInstruments>
<CsScore>

i1 0 1 60 120   ; p4 = midi note
i1 + 1 62 <
i1 + 1 65 <
i1 + 1 69 10


i2 5 1 200 120  ; p4 = frequency
i2 + 1 261 <
i2 + 1 300 <
i2 + 1 1000 10
e
</CsScore>
</CsoundSynthesizer>
