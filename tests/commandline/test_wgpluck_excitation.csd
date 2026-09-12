<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; A one-sample block provides a reference for partial-block excitation.
opcode PluckReference, a, aiik
  setksmps 1
  aInput, iFrequency, iAmplitude, kPickup xin
  aOutput wgpluck iFrequency, iAmplitude, kPickup, .5, 10, 100, aInput
  xout aOutput
endop

instr 1
  aInput oscili .1, 600
  aReference PluckReference aInput, p4, p5, p6
  aOutput wgpluck p4, p5, p6, .5, 10, 100, aInput
  aReuse = aInput
  aReuse wgpluck p4, p5, p6, .5, 10, 100, aReuse
  kError max_k abs(aOutput-aReference)+abs(aReuse-aReference), 1, 1
  if !(kError < .000001) then
    printks "wgpluck frequency %g amplitude %g pickup %g: excitation error %g\n", 0, p4, p5, p6, kError
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 8 then
    prints "wgpluck checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03 256 0 .5
i 1 0 .03 440 .2 .5
i 1 0 .03 256 .2 0
i 1 0 .03 440 0 1
; Reuse notes, starting at sample 3 and ending at sample 13 of a block.
i 1 .0628662109375 .030517578125 256 0 .5
i 1 .0628662109375 .030517578125 440 .2 .5
i 1 .0628662109375 .030517578125 256 .2 0
i 1 .0628662109375 .030517578125 440 0 1
i 99 .125 .001
e
</CsScore>
</CsoundSynthesizer>
