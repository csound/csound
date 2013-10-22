<CsoundSynthesizer>
<CsOptions>
; Select flags here
; realtime audio out 
 -o dac 
; For Non-realtime ouput leave only the line below:
 ;-o hrtf.wav
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

gasrc init 0

instr 1		;a plucked string

  kamp = p4
  kcps = cpspch(p5)
  icps = cpspch(p5)

  a1 pluck kamp, kcps, icps, 0, 1

  gasrc = a1

endin

instr 10	;uses output from instr1 as source

 kaz	linseg 0, p3, 720		;2 full rotations

 aleft,aright hrtfmove gasrc, kaz,0, "hrtf-44100-left.dat","hrtf-44100-right.dat"

 outs	aleft, aright
  
endin

</CsInstruments>
<CsScore>

; Play Instrument 1: a simple arpeggio
i1 0 .2 15000 8.00 
i1 + .2 15000 8.04
i1 + .2 15000 8.07
i1 + .2 15000 8.11
i1 + .2 15000 9.02
i1 + 1.5 15000 8.11
i1 + 1.5 15000 8.07
i1 + 1.5 15000 8.04
i1 + 1.5 15000 8.00
i1 + 1.5 15000 7.09
i1 + 1.5 15000 8.00

; Play Instrument 10 for 10 seconds.
i10 0 10

</CsScore>
</CsoundSynthesizer>
