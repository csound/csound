<CsTest>
description = "pvsarp selects DC, interior, and Nyquist bins and preserves frequencies"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 iTarget = p4
 iSelectedBin = p5
 iDepth = p6
 iOtherGain = p7

 ; A 128-point spectrum has 65 bins. Give each bin a distinct amplitude
 ; and frequency so that the check also catches changes to other bins.
 kInput[] init 130
 kBin = 0
 while kBin < 65 do
  kInput[2*kBin] = (kBin+1)/64
  kInput[2*kBin+1] = kBin*sr/128
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 fOutput pvsarp fInput, iTarget, iDepth, 2
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput

 ; Wait until the input has published a complete frame.
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kBin = 0
  while kBin < 65 do
   kGain = kBin == iSelectedBin ? 2 : iOtherGain
   kExpected = kInput[2*kBin]*kGain
   if kOutput[2*kBin] != kExpected || kOutput[2*kBin+1] != kInput[2*kBin+1] then
    printks "target %g, bin %g: amplitude=%g expected=%g; frequency=%g expected=%g\n", 0, iTarget, kBin, kOutput[2*kBin], kExpected, kOutput[2*kBin+1], kInput[2*kBin+1]
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 8 then
  prints "pvsarp bin checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Normalized target, selected bin, depth, gain for all other bins.
i 1 0 .01 0     0  1    0
i 1 0 .01 .2    13 .25  .75
i 1 0 .01 .999  64 1    0
i 1 0 .01 1     64 1    0
; Targets outside [0, 1] clamp to DC or Nyquist.
i 1 0 .01 -.1   0  1    0
i 1 0 .01 1.1   64 1    0
; Depth clamps to [0, 1] without changing the selected bin's gain.
i 1 0 .01 .5    32 -1   1
i 1 0 .01 .5    32 2    0
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
