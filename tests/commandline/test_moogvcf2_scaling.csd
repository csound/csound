<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
#ifndef FS
#define FS #32768#
#endif
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = $FS
gkChecks init 0

instr 1
 iScale = (p4 == 0 ? 0dbfs : p4*0dbfs)
 kBlock init 0
 kBlock += 1
 aPhase phasor 256
 aInput = iScale*(.4+.1*aPhase)
 aReuse = aInput
 kCutoff = 900+100*(kBlock%2)
 kResonance = .3+.1*(kBlock%2)
 aCutoff = 900+200*aPhase
 aResonance = .3+.2*aPhase
 ; The legacy filter multiplies iscale by 0dBFS internally. Compensating
 ; for that extra factor gives the documented moogvcf2 scaling.
 if p5 == 0 then
  aOut moogvcf2 aInput, kCutoff, kResonance, p4*0dbfs
  aReference moogvcf aInput, kCutoff, kResonance, iScale/0dbfs
  aReuse moogvcf2 aReuse, kCutoff, kResonance, p4*0dbfs
 elseif p5 == 1 then
  aOut moogvcf2 aInput, aCutoff, kResonance, p4*0dbfs
  aReference moogvcf aInput, aCutoff, kResonance, iScale/0dbfs
  aReuse moogvcf2 aReuse, aCutoff, kResonance, p4*0dbfs
 elseif p5 == 2 then
  aOut moogvcf2 aInput, kCutoff, aResonance, p4*0dbfs
  aReference moogvcf aInput, kCutoff, aResonance, iScale/0dbfs
  aReuse moogvcf2 aReuse, kCutoff, aResonance, p4*0dbfs
 else
  aOut moogvcf2 aInput, aCutoff, aResonance, p4*0dbfs
  aReference moogvcf aInput, aCutoff, aResonance, iScale/0dbfs
  aReuse moogvcf2 aReuse, aCutoff, aResonance, p4*0dbfs
 endif
 kN = 0
 while kN < ksmps do
  kOut vaget kN, aOut
  kReference vaget kN, aReference
  kReuse vaget kN, aReuse
  if !(abs(kOut-kReference)/iScale < .000002 && abs(kReuse-kOut)/iScale < .000002) then
   printks "moogvcf2 scale=%g rates=%g block=%g sample=%g actual=%g expected=%g reuse=%g\n", 0, p4, p5, kBlock, kN, kOut/iScale, kReference/iScale, kReuse/iScale
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 2
 ; A fixed numerical check also protects the reference's legacy behavior.
 aInput = .5*0dbfs
 aDefault moogvcf2 aInput, 1000, 0
 aExplicit moogvcf2 aInput, 1000, 0, 0dbfs
 aLegacy moogvcf aInput, 1000, 0, 1
 kDefault downsamp aDefault
 kExplicit downsamp aExplicit
 kLegacy downsamp aLegacy
 kBlock init 0
 kBlock += 1
 if kBlock == 128 then
  kExpected = .474289243981
  if !(abs(kDefault/0dbfs-kExpected) < .000002 && abs(kExplicit/0dbfs-kExpected) < .000002 && abs(kLegacy/0dbfs-kExpected) < .000002) then
   printks "moogvcf2 fullscale=%g default=%g explicit=%g legacy=%g expected=%g\n", 0, 0dbfs, kDefault/0dbfs, kExplicit/0dbfs, kLegacy/0dbfs, kExpected
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 17 then
  prints "moogvcf2 cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Each scale covers all control/audio parameter combinations. Alternate
; full blocks and notes starting at sample 3 and ending before the block end.
i 1 0 .0078125 0 0
i 1 0.0159912109375 .0068359375 0 1
i 1 .03125 .0078125 0 2
i 1 0.0472412109375 .0068359375 0 3
i 1 .0625 .0078125 .5 0
i 1 0.0784912109375 .0068359375 .5 1
i 1 .09375 .0078125 .5 2
i 1 0.1097412109375 .0068359375 .5 3
i 1 .125 .0078125 1 0
i 1 0.1409912109375 .0068359375 1 1
i 1 .15625 .0078125 1 2
i 1 0.1722412109375 .0068359375 1 3
i 1 .1875 .0078125 2 0
i 1 0.2034912109375 .0068359375 2 1
i 1 .21875 .0078125 2 2
i 1 0.2347412109375 .0068359375 2 3
i 2 .25 .25
i 99 .51 .001
e
</CsScore>
</CsoundSynthesizer>
