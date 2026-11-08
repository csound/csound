<CsTest>
description = "SoundFont global-zone pan, attenuation and scale tuning act as in a sample's zone"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsFileB filename="global_zones.sf2">
UklGRnIGAABzZmJrTElTVDYAAABJTkZPaWZpbAQAAAACAAEAaXNuZwgAAABFTVU4MDAwAElOQU0N
AAAAR2xvYmFsIHpvbmVzAABMSVNUaAIAAHNkdGFzbXBsXAIAAADgQOCB4MHgAeFB4YLhwuEC4kLi
g+LD4gPjQ+OE48TjBORE5IXkxeQF5UXlhuXG5QbmRuaH5sfmB+dH54jnyOcI6EjoiejJ6AnpSemK
6crpCupK6ovqy+oL60vrjOvM6wzsTOyN7M3sDe1N7Y7tzu0O7k7uj+7P7g/vT++Q79DvEPBQ8JHw
0fAR8VHxkvHS8RLyUvKT8tPyE/NT85Tz1PMU9FT0lfTV9BX1VfWW9db1FvZW9pf21/YX91f3mPfY
9xj4WPiZ+Nn4GflZ+Zr52vka+lr6m/rb+hv7W/uc+9z7HPxc/J383fwd/V39nv3e/R7+Xv6f/t/+
H/9f/6D/4P8gAGAAoQDhACEBYQGiAeIBIgJiAqMC4wIjA2MDpAPkAyQEZASlBOUEJQVlBaYF5gUm
BmYGpwbnBicHZweoB+gHKAhoCKkI6QgpCWkJqgnqCSoKagqrCusKKwtrC6wL7AssDGwMrQztDC0N
bQ2uDe4NLg5uDq8O7w4vD28PsA/wDzAQcBCxEPEQMRFxEbIR8hEyEnISsxLzEjMTcxO0E/QTNBR0
FLUU9RQ1FXUVthX2FTYWdha3FvcWNxd3F7gX+Bc4GHgYuRj5GDkZeRm6GfoZOhp6Grsa+xo7G3sb
vBv8GzwcfBy9HP0cPR19Hb4d/h0+Hn4evx7/Hj8ffx/AHwAgAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAABMSVNUuAMAAHBkdGFwaGRyVgEAAEdsb2JhbCBwYW4gLTUwMAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAWm9uZSBwYW4gLTUwMAAAAAAAAAABAAAAAQAAAAAAAAAAAAAAAABHbG9i
YWwgcGFuICs1MDAAAAAAAAIAAAACAAAAAAAAAAAAAAAAAFpvbmUgcGFuICs1MDAAAAAAAAAAAwAA
AAMAAAAAAAAAAAAAAAAAR2xvYmFsIGF0dGVuIDMwMAAAAAAEAAAABAAAAAAAAAAAAAAAAABab25l
IGF0dGVuIDMwMAAAAAAAAAUAAAAFAAAAAAAAAAAAAAAAAEdsb2JhbCBzY2FsZSAzMDAAAAAABgAA
AAYAAAAAAAAAAAAAAAAAWm9uZSBzY2FsZSAzMDAAAAAAAAAHAAAABwAAAAAAAAAAAAAAAABFT1AA
AAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAAAAAAAAAAAAAAAHBiYWckAAAAAAAAAAEAAAACAAAAAwAA
AAQAAAAFAAAABgAAAAcAAAAIAAAAcG1vZAoAAAAAAAAAAAAAAAAAcGdlbiQAAAApAAAAKQABACkA
AgApAAMAKQAEACkABQApAAYAKQAHAAAAAABpbnN0xgAAAEdsb2JhbCBwYW4gLTUwMAAAAAAAAABa
b25lIHBhbiAtNTAwAAAAAAAAAAIAR2xvYmFsIHBhbiArNTAwAAAAAAADAFpvbmUgcGFuICs1MDAA
AAAAAAAABQBHbG9iYWwgYXR0ZW4gMzAwAAAAAAYAWm9uZSBhdHRlbiAzMDAAAAAAAAAIAEdsb2Jh
bCBzY2FsZSAzMDAAAAAACQBab25lIHNjYWxlIDMwMAAAAAAAAAsARU9JAAAAAAAAAAAAAAAAAAAA
AAAMAGliYWc0AAAAAAAAAAEAAAADAAAABgAAAAcAAAAJAAAADAAAAA0AAAAPAAAAEgAAABMAAAAV
AAAAGAAAAGltb2QKAAAAAAAAAAAAAAAAAGlnZW5kAAAAEQAM/jYAAQA1AAAAEQAM/jYAAQA1AAAA
EQD0ATYAAQA1AAAAEQD0ATYAAQA1AAAAMAAsATYAAQA1AAAAMAAsATYAAQA1AAAAOAAsATYAAQA1
AAAAOAAsATYAAQA1AAAAAAAAAHNoZHJcAAAAUmFtcAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEAAAgA
AAD4AAAAAAQAADwAAAABAEVPUwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAA=
</CsFileB>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1
; The embedded bank contains one looped ramp and four pairs of instruments,
; each with a preset of the same number. Instrument 2k sets a value in its
; global zone, and instrument 2k+1 the same value in the sample's own zone:
; 0, 1: pan -500; 2, 3: pan +500; 4, 5: attenuation 300 cB; 6, 7: scale
; tuning 300 cents per key. Global values were cut to 8 bits (-500 became 12).
giFont sfload "global_zones.sf2"
sfpassign 0, giFont, 0
gkDone init 0

instr 1
  kCycle init 0
  iGlobal = 2*p4
  iZone = 2*p4 + 1
  ; Key 61, a key above the root, so scale tuning sets the pitch.
  aPL, aPR sfplay 100, 61, 1, 1, iGlobal
  aZL, aZR sfplay 100, 61, 1, 1, iZone
  aIL, aIR sfinstr 100, 61, 1, 1, iGlobal, giFont
  aJL, aJR sfinstr 100, 61, 1, 1, iZone, giFont
  kPreset = abs(k(aPL) - k(aZL)) + abs(k(aPR) - k(aZR))
  kInstr = abs(k(aIL) - k(aJL)) + abs(k(aIR) - k(aJR))
  ; The ramp is never 0, so a zone that plays is never silent.
  kSound = min(abs(k(aZL)) + abs(k(aZR)), abs(k(aJL)) + abs(k(aJR)))
  if kPreset != 0 || kInstr != 0 || kSound <= 0 then
    printks "SoundFont global zone mismatch: pair %g, cycle %g: %g, %g, level %g\n", 0, p4, kCycle, kPreset, kInstr, kSound
    exitnowk -1
  endif
  if kCycle == 0 then
    gkDone += 1
  endif
  kCycle += 1
endin

instr 90
  iPair = 0
  while iPair < 4 do
    schedule 1, 0, .5, iPair
    iPair += 1
  od
endin

instr 99
  if i(gkDone) != 4 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 90 0 .001
i 99 .55 .001
</CsScore>
</CsoundSynthesizer>
