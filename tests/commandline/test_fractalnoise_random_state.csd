<CsTest>
description = "fractalnoise wrapped random sequence and reinitialization"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1
gkDone init 0

instr 1
 setksmps p4
 seed 20120124
 ; First 64 signed values of the 32-bit recurrence, computed with
 ; integer arithmetic. At beta=0 the filter produces white noise.
 iExpected[] fillarray 20120124, 83967432, 339558500, 2050876624, -1306210612, -810993896, -1348941964, 163399328, \
    302961756, -782273176, 1930276740, 1838740848, -1967414548, 759384248, -1118762860, -2083995328, \
    969340, -1592196856, 190374564, -638486000, -661333236, 177860184, 460082612, 2067066848, \
    -15506276, -238788440, 825448900, -889064784, 1830352684, 1747691512, -1966663980, 1021596288, \
    -829611332, -1924656056, 381785316, -1185620144, -834635956, 130479512, 2068702196, 2139711776, \
    -1686338340, -1009083416, 2117604356, 1413791728, 949214060, 1183471416, -570608364, -844713024, \
    577899260, -436045944, -1411688668, -1946413936, -517233780, 920931544, 920750644, 797880928, \
    294532380, -23714008, -1744839100, -613065424, 2008043436, -91007368, -283679916, 358868224
 kCount init 0
 kSample init 0
 kStart init p5
 kAmp init .25
 if p6 != 0 && kSample == 64 then
  kSample = 0
  kAmp = .5
  reinit NOISE
 endif
NOISE:
 aNoise fractalnoise kAmp, 0
 rireturn
 kIndex = kStart
 while kIndex < p4 && kCount < p7 do
  kActual vaget kIndex, aNoise
  kExpected = kAmp * (iExpected[kSample] / 2147483648)
  if !(abs(kActual-kExpected) < .0001) then
   printks "fractalnoise block %.0f offset %.0f reinit %.0f sample %d: got %.9f, expected %.9f\n", 0, p4, p5, p6, kSample, kActual, kExpected
   exitnowk -1
  endif
  kSample += 1
  kCount += 1
  kIndex += 1
 od
 kStart = 0
 if kCount == p7 then
  gkDone += 1
  turnoff
 endif
endin

instr 99
 if i(gkDone) != 8 then
  prints "fractalnoise checks did not finish\n"
  exitnow -1
 endif
endin
</CsInstruments>
<CsScore>
; Block size, initial offset, reinit, total active samples.
i 1 0          .008  1  0 0 64
i 1 .04        .008  8  0 0 64
i 1 .08        .008 32  0 0 64
i 1 .160375    .008 32  3 0 64
; Reuse the same instance and reset its sequence at each reinit.
i 1 .20        .128  1  0 1 1024
i 1 .36        .128  8  0 1 1024
i 1 .52        .128 32  0 1 1024
i 1 .68        .008 32  0 0 64
i 99 .72 .01
e
</CsScore>
</CsoundSynthesizer>
