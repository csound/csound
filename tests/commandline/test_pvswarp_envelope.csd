<CsTest>
description = "pvswarp moves the envelope without moving frequencies and applies the cutoff only to shifting"

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
 kInput[] init 130
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = p8 == 1 ? .25 : (1+kBin%5)/8
  kInput[2*kBin+1] = kBin*62.5 + (kBin%3)*4
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 ; Keeping every coefficient makes the envelope equal the input amplitudes.
 ; Its movement is then visible directly in the output amplitudes.
 fOutput pvswarp fInput, p4, p5*62.5, p6*62.5, p7, 1, 64
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kExpected[] init 65
  ; DC and Nyquist pass through. Other bins start empty.
  kExpected[0] = kInput[0]
  kExpected[64] = kInput[128]
  kSource = 1
  while kSource < 64 do
   ; Scaling still applies below the cutoff; shifting starts at the cutoff.
   kShift = kSource >= p6 ? p5 : 0
   kPosition = kSource*p4+kShift
   if kPosition >= .5 && kPosition < 63.5 then
    kDestination = int(kPosition+.5)
    ; If envelope bins share a destination, the last one wins.
    kExpected[kDestination] = kInput[2*kSource]
   endif
   kSource += 1
  od
  kBin = 0
  while kBin <= 64 do
   kAmpError = abs(kOutput[2*kBin]-kExpected[kBin])
   ; Frequencies stay at their original destinations, including empty bins.
   kFreqError = abs(kOutput[2*kBin+1]-kInput[2*kBin+1])
   if !(kAmpError < .000001 && kFreqError < .001) then
    printks "ratio %g, shift %g bins, cutoff %g, method %g, bin %g: amplitude %g expected %g; frequency %g expected %g\n", 0, p4, p5, p6, p7, kBin, kOutput[2*kBin], kExpected[kBin], kOutput[2*kBin+1], kInput[2*kBin+1]
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 16 then
  prints "pvswarp envelope checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Ratio, shift in bins, cutoff bin, method, use a flat spectrum.
i 1 0 .01 1 1 0 1 0
i 1 0 .01 1 -1 0 1 0
i 1 0 .01 .5 0 0 1 0
i 1 0 .01 2 0 0 1 0
; The cutoff limits shifting, including when scaling is also active.
i 1 0 .01 2 0 16 1 0
i 1 0 .01 2 1 16 1 0
i 1 0 .01 1 1 16 2 0
i 1 0 .01 1 -1 16 2 0
i 1 0 .01 .5 -1 16 2 0
i 1 0 .01 2 1 80 2 0
; These controls move every interior envelope bin out of range.
i 1 0 .01 0 0 0 1 0
i 1 0 .01 100 0 0 1 0
i 1 0 .01 1 100 0 1 0
i 1 0 .01 1 -100 0 1 0
; A moving average of a flat envelope stays flat, including both edges.
i 1 0 .01 .5 0 0 3 1
i 1 0 .01 2 0 0 3 1
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
