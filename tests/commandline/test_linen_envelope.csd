<CsTest>
description = "linen stage timing, zero decay, and partial audio blocks"

[expect]
exit = 0
</CsTest>
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

; Reference uses elapsed samples/cycles and the requested stage lengths.
; A decay longer than idur starts at unity and is truncated by the note.
opcode EnvelopeAt, k, kiii
  kElapsed, iRise, iDuration, iDecay xin
  kRise = 1
  if iRise > 0 && kElapsed < iRise then
    kRise = kElapsed/iRise
  endif
  kDecay = 1
  if iDecay > 0 then
    kDecay = 1 - max(0, kElapsed - max(0, iDuration-iDecay))/iDecay
  endif
  xout kRise*kDecay
endop

instr 1
  iRise = max(0, round(p4))
  iDuration = round(p5)
  iDecay = max(0, round(p6))
  kElapsed init 0
  kAmp = .5 + kElapsed*.01
  kOut linen kAmp, p4/kr, p5/kr, p6/kr
  kExpected EnvelopeAt kElapsed, iRise, iDuration, iDecay
  if abs(kOut - kAmp*kExpected) > .000001 then
    printks "linen control mismatch: rise=%g duration=%g decay=%g elapsed=%g expected=%g actual=%g\n", 0, p4, p5, p6, kElapsed, kAmp*kExpected, kOut
    exitnowk(-1)
  endif
  kElapsed += 1
  if kElapsed == 16 then
    gkChecks += 1
  endif
endin

instr 2
  iRise = max(0, round(p4))
  iDuration = round(p5)
  iDecay = max(0, round(p6))
  aAmp line .5, p3, 1
  aOut linen aAmp, p4/sr, p5/sr, p6/sr
  aConst linen .5, p4/sr, p5/sr, p6/sr
  aGate = 1
  kElapsed init 0
  kN = 0
  while kN < ksmps do
    kGate vaget kN, aGate
    kOut vaget kN, aOut
    kConst vaget kN, aConst
    kAmp vaget kN, aAmp
    kExpected = 0
    if kGate != 0 then
      kExpected EnvelopeAt kElapsed, iRise, iDuration, iDecay
      kElapsed += 1
    endif
    if abs(kOut-kAmp*kExpected) + abs(kConst-.5*kExpected) > .000001 then
      printks "linen audio mismatch: rise=%g duration=%g decay=%g elapsed=%g expected=%g actual=%g\n", 0, p4, p5, p6, kElapsed-1, kAmp*kExpected, kOut
      exitnowk(-1)
    endif
    kN += 1
  od
  if kElapsed == 48 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 18 then
    prints "linen envelope checks incomplete: %g\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Control-rate stages, including output after idur, which may go negative.
i 1 0 .03125 0 4 4
i 1 .0625 .03125 2 8 4
i 1 .125 .03125 0 4 0
i 1 .1875 .03125 2 4 0
i 1 .25 .03125 2.75 8.75 4.25
i 1 .3125 .03125 4 6 4
i 1 .375 .03125 0 4 8
i 1 .4375 .03125 -2 8 4
i 1 .5 .03125 0 4 .25
; Audio-rate stages: full blocks and starts three samples into a block.
i 2 .5625 .005859375 0 4 4
i 2 .6253662109375 .005859375 2 8 4
i 2 .6875 .005859375 0 4 0
i 2 .7503662109375 .005859375 2 4 0
i 2 .8125 .005859375 2.75 8.75 4.25
i 2 .8753662109375 .005859375 4 6 4
i 2 .9375 .005859375 0 4 8
i 2 1.0003662109375 .005859375 -2 8 4
i 2 1.0625 .005859375 0 4 .25
i 99 1.125 .002
e
</CsScore>
</CsoundSynthesizer>
