<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  aBuffer = 0
  kCycle init 0
  kCycle += 1
  kIndex init p4
  kValue = kCycle * .25
  vaset kValue, kIndex, aBuffer
  kRead = aBuffer[kIndex]
  if kRead != kValue then
    printks "audio buffer indexing did not read the sample written by vaset\n", 0
    exitnowk -1
  endif
  aBuffer[kIndex] = -kValue
  kRead vaget kIndex, aBuffer
  if kRead != -kValue then
    printks "vaget did not read the sample written by audio buffer indexing\n", 0
    exitnowk -1
  endif
  ; In a full block, the write must change only floor(kIndex).
  if p5 == 0 then
    kSample = 0
    while kSample < ksmps do
      kRead vaget kSample, aBuffer
      kExpected = (kSample == int(kIndex) ? -kValue : 0)
      if kRead != kExpected then
        printks "audio buffer write changed the wrong sample\n", 0
        exitnowk -1
      endif
      kSample += 1
    od
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
  setksmps 1
  aBuffer = 0
  kIndex init .75
  vaset .5, kIndex, aBuffer
  kRead = aBuffer[kIndex]
  if kRead != .5 then
    exitnowk -1
  endif
  aBuffer[kIndex] = .25
  kRead vaget kIndex, aBuffer
  if kRead != .25 then
    exitnowk -1
  endif
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "not all audio buffer index checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 0 0
i 1 0 .03125 .75 0
i 1 0 .03125 1 0
i 1 0 .03125 1.75 0
i 1 0 .03125 31 0
i 1 0 .03125 31.75 0
i 1 .0006103515625 .0130615234375 7.75 1
; Inactive samples remain accessible, with a warning at the note boundaries.
i 1 .0006103515625 .0130615234375 0 1
i 1 .0006103515625 .0130615234375 31.75 1
i 2 0 .03125
i 99 .05 .01
e
</CsScore>
</CsoundSynthesizer>
