<CsoundSynthesizer>
<CsOptions>

; Select flags here
; realtime audio out
 -o dac
; file ouput 
; -o hrtf.wav
  
</CsOptions>
<CsInstruments>

nchnls = 2

gasrc init 0	;global

instr 1		;a plucked string, distorted and filtered

  iamp = 15000
  icps = cpspch(p4)

  a1 pluck iamp, icps, icps, 0, 1
  adist distort1 a1, 10, .5, 0, 0
  afilt moogvcf2 adist, 8000, .5 
  aout linen afilt, 0, p3, .01
  
  gasrc = gasrc + aout

endin

instr 10	;uses output from instr1 as source
  
  ;simple path for source
  kx line 2, p3, 9

  ;early reflections, room default 1
  aearlyl,aearlyr, irt60low, irt60high, imfp hrtfearly gasrc, kx, 5, 1, 5, 1, 1, "hrtf-44100-left.dat", "hrtf-44100-right.dat", 1

  ;later reverb, uses outputs from above
  arevl, arevr, idel hrtfreverb gasrc, irt60low, irt60high, "hrtf-44100-left.dat", "hrtf-44100-right.dat", 44100, imfp

  ;delayed and scaled
  alatel delay arevl * .1, idel
  alater delay arevr * .1, idel

  outs	aearlyl + alatel, aearlyr + alater
  
  gasrc = 0

endin
  
</CsInstruments>
<CsScore>

; Play Instrument 1: a simple arpeggio
i1 0 .2 8.00 
i1 + .2 8.04
i1 + .2 8.07
i1 + .2 8.11
i1 + .2 9.02
i1 + 1.5 8.11
i1 + 1.5 8.07
i1 + 1.5 8.04
i1 + 1.5 8.00
i1 + 1.5 7.09
i1 + 4 8.00

; Play Instrument 10 for 13 seconds.
i10 0 13

</CsScore>
</CsoundSynthesizer>


