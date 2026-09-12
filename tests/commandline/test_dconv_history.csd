<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
giIR ftgen 1, 0, 8, -2, 1, -.5, .25, -.125, .0625, -.03125, .015625, -.0078125
gkChecks init 0

; An explicit FIR reference avoids duplicating dconv's circular buffer.
opcode Reference, a, ai
  aInput, iLength xin
  a1 delay1 aInput
  a2 delay1 a1
  a3 delay1 a2
  a4 delay1 a3
  a5 delay1 a4
  a6 delay1 a5
  a7 delay1 a6
  aResult = aInput
  if iLength >= 2 then
    aResult -= .5 * a1
  endif
  if iLength >= 3 then
    aResult += .25 * a2
  endif
  if iLength >= 4 then
    aResult -= .125 * a3
  endif
  if iLength >= 5 then
    aResult += .0625 * a4
  endif
  if iLength >= 6 then
    aResult -= .03125 * a5
  endif
  if iLength >= 7 then
    aResult += .015625 * a6
  endif
  if iLength >= 8 then
    aResult -= .0078125 * a7
  endif
  xout aResult
endop

instr 1
  kCycle init 0
  kLength init p4
  kCycle += 1
  kLength = (p6 > 0 && kCycle >= p6 ? p5 : p4)
  aInput oscils .5, 311, 0
  if kCycle == p6 then
    reinit FILTER
  endif
FILTER:
  iLength = i(kLength)
  aExpected Reference aInput, int(min(iLength, 8))
  if p7 == 1 then
    aInput dconv aInput, iLength, giIR
    aActual = aInput
  else
    aActual dconv aInput, iLength, giIR
  endif
  rireturn
  kSample = 0
  while kSample < ksmps do
    kActual vaget kSample, aActual
    kExpected vaget kSample, aExpected
    if abs(kActual - kExpected) > .000001 then
      printks "dconv length %g cycle %g sample %g: %g expected %g\n", 0, kLength, kCycle, kSample, kActual, kExpected
      exitnowk -1
    endif
    kSample += 1
  od
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 12 then
    prints "not all dconv checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Initial length, new length, reset cycle, in-place output.
i 1 0 .0625 1 0 0 0
i 1 0 .0625 3.75 0 0 0
i 1 0 .0625 8 0 0 0
i 1 0 .0625 1e30 0 0 0
i 1 0 .0625 1 8 4 0
i 1 0 .0625 8 1 4 0
i 1 0 .0625 8 8 4 0
i 1 0 .0625 8 0 0 1
; Partial first/last blocks, including a note inside a single block.
i 1 .0006103515625 .0130615234375 3 0 0 0
i 1 .0006103515625 .0130615234375 3 0 0 1
i 1 .0006103515625 .0013427734375 8 0 0 0
; Reuse an instance after its previous note ends.
i 1 .125 .0625 8 0 0 0
i 99 .2 .01
e
</CsScore>
</CsoundSynthesizer>
