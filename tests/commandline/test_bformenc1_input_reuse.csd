<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; 4 output channels.
instr 1
  aSource oscili .5, 440
  aRef0, aRef1, aRef2, aRef3 bformenc1 aSource, p4, p5
  aArray[] init 4
  aArray bformenc1 aSource, p4, p5
  aError = 0
  aError += abs(aRef0-aArray[0])
  aError += abs(aRef1-aArray[1])
  aError += abs(aRef2-aArray[2])
  aError += abs(aRef3-aArray[3])
  ; Reuse input as channel 0.
  aCopy0x0 = aSource
  aCopy0x0, aCopy0x1, aCopy0x2, aCopy0x3 bformenc1 aCopy0x0, p4, p5
  aReuse0[] init 4
  aReuse0[0] = aSource
  aReuse0 bformenc1 aReuse0[0], p4, p5
  aError += abs(aRef0-aCopy0x0) + abs(aRef0-aReuse0[0])
  aError += abs(aRef1-aCopy0x1) + abs(aRef1-aReuse0[1])
  aError += abs(aRef2-aCopy0x2) + abs(aRef2-aReuse0[2])
  aError += abs(aRef3-aCopy0x3) + abs(aRef3-aReuse0[3])
  ; Reuse input as channel 2.
  aCopy1x2 = aSource
  aCopy1x0, aCopy1x1, aCopy1x2, aCopy1x3 bformenc1 aCopy1x2, p4, p5
  aReuse1[] init 4
  aReuse1[2] = aSource
  aReuse1 bformenc1 aReuse1[2], p4, p5
  aError += abs(aRef0-aCopy1x0) + abs(aRef0-aReuse1[0])
  aError += abs(aRef1-aCopy1x1) + abs(aRef1-aReuse1[1])
  aError += abs(aRef2-aCopy1x2) + abs(aRef2-aReuse1[2])
  aError += abs(aRef3-aCopy1x3) + abs(aRef3-aReuse1[3])
  ; Reuse input as channel 3.
  aCopy2x3 = aSource
  aCopy2x0, aCopy2x1, aCopy2x2, aCopy2x3 bformenc1 aCopy2x3, p4, p5
  aReuse2[] init 4
  aReuse2[3] = aSource
  aReuse2 bformenc1 aReuse2[3], p4, p5
  aError += abs(aRef0-aCopy2x0) + abs(aRef0-aReuse2[0])
  aError += abs(aRef1-aCopy2x1) + abs(aRef1-aReuse2[1])
  aError += abs(aRef2-aCopy2x2) + abs(aRef2-aReuse2[2])
  aError += abs(aRef3-aCopy2x3) + abs(aRef3-aReuse2[3])
  kError max_k aError, 1, 1
  if !(kError < .00001) then
    printks "bformenc1 input reuse differs in instrument %g: %g\n", 0, p1, kError
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

; 9 output channels.
instr 2
  aSource oscili .5, 440
  aRef0, aRef1, aRef2, aRef3, aRef4, aRef5, aRef6, aRef7, aRef8 bformenc1 aSource, p4, p5
  aArray[] init 9
  aArray bformenc1 aSource, p4, p5
  aError = 0
  aError += abs(aRef0-aArray[0])
  aError += abs(aRef1-aArray[1])
  aError += abs(aRef2-aArray[2])
  aError += abs(aRef3-aArray[3])
  aError += abs(aRef4-aArray[4])
  aError += abs(aRef5-aArray[5])
  aError += abs(aRef6-aArray[6])
  aError += abs(aRef7-aArray[7])
  aError += abs(aRef8-aArray[8])
  ; Reuse input as channel 0.
  aCopy0x0 = aSource
  aCopy0x0, aCopy0x1, aCopy0x2, aCopy0x3, aCopy0x4, aCopy0x5, aCopy0x6, aCopy0x7, aCopy0x8 bformenc1 aCopy0x0, p4, p5
  aReuse0[] init 9
  aReuse0[0] = aSource
  aReuse0 bformenc1 aReuse0[0], p4, p5
  aError += abs(aRef0-aCopy0x0) + abs(aRef0-aReuse0[0])
  aError += abs(aRef1-aCopy0x1) + abs(aRef1-aReuse0[1])
  aError += abs(aRef2-aCopy0x2) + abs(aRef2-aReuse0[2])
  aError += abs(aRef3-aCopy0x3) + abs(aRef3-aReuse0[3])
  aError += abs(aRef4-aCopy0x4) + abs(aRef4-aReuse0[4])
  aError += abs(aRef5-aCopy0x5) + abs(aRef5-aReuse0[5])
  aError += abs(aRef6-aCopy0x6) + abs(aRef6-aReuse0[6])
  aError += abs(aRef7-aCopy0x7) + abs(aRef7-aReuse0[7])
  aError += abs(aRef8-aCopy0x8) + abs(aRef8-aReuse0[8])
  ; Reuse input as channel 4.
  aCopy1x4 = aSource
  aCopy1x0, aCopy1x1, aCopy1x2, aCopy1x3, aCopy1x4, aCopy1x5, aCopy1x6, aCopy1x7, aCopy1x8 bformenc1 aCopy1x4, p4, p5
  aReuse1[] init 9
  aReuse1[4] = aSource
  aReuse1 bformenc1 aReuse1[4], p4, p5
  aError += abs(aRef0-aCopy1x0) + abs(aRef0-aReuse1[0])
  aError += abs(aRef1-aCopy1x1) + abs(aRef1-aReuse1[1])
  aError += abs(aRef2-aCopy1x2) + abs(aRef2-aReuse1[2])
  aError += abs(aRef3-aCopy1x3) + abs(aRef3-aReuse1[3])
  aError += abs(aRef4-aCopy1x4) + abs(aRef4-aReuse1[4])
  aError += abs(aRef5-aCopy1x5) + abs(aRef5-aReuse1[5])
  aError += abs(aRef6-aCopy1x6) + abs(aRef6-aReuse1[6])
  aError += abs(aRef7-aCopy1x7) + abs(aRef7-aReuse1[7])
  aError += abs(aRef8-aCopy1x8) + abs(aRef8-aReuse1[8])
  ; Reuse input as channel 8.
  aCopy2x8 = aSource
  aCopy2x0, aCopy2x1, aCopy2x2, aCopy2x3, aCopy2x4, aCopy2x5, aCopy2x6, aCopy2x7, aCopy2x8 bformenc1 aCopy2x8, p4, p5
  aReuse2[] init 9
  aReuse2[8] = aSource
  aReuse2 bformenc1 aReuse2[8], p4, p5
  aError += abs(aRef0-aCopy2x0) + abs(aRef0-aReuse2[0])
  aError += abs(aRef1-aCopy2x1) + abs(aRef1-aReuse2[1])
  aError += abs(aRef2-aCopy2x2) + abs(aRef2-aReuse2[2])
  aError += abs(aRef3-aCopy2x3) + abs(aRef3-aReuse2[3])
  aError += abs(aRef4-aCopy2x4) + abs(aRef4-aReuse2[4])
  aError += abs(aRef5-aCopy2x5) + abs(aRef5-aReuse2[5])
  aError += abs(aRef6-aCopy2x6) + abs(aRef6-aReuse2[6])
  aError += abs(aRef7-aCopy2x7) + abs(aRef7-aReuse2[7])
  aError += abs(aRef8-aCopy2x8) + abs(aRef8-aReuse2[8])
  kError max_k aError, 1, 1
  if !(kError < .00001) then
    printks "bformenc1 input reuse differs in instrument %g: %g\n", 0, p1, kError
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

; 16 output channels.
instr 3
  aSource oscili .5, 440
  aRef0, aRef1, aRef2, aRef3, aRef4, aRef5, aRef6, aRef7, aRef8, aRef9, aRef10, aRef11, aRef12, aRef13, aRef14, aRef15 bformenc1 aSource, p4, p5
  aArray[] init 16
  aArray bformenc1 aSource, p4, p5
  aError = 0
  aError += abs(aRef0-aArray[0])
  aError += abs(aRef1-aArray[1])
  aError += abs(aRef2-aArray[2])
  aError += abs(aRef3-aArray[3])
  aError += abs(aRef4-aArray[4])
  aError += abs(aRef5-aArray[5])
  aError += abs(aRef6-aArray[6])
  aError += abs(aRef7-aArray[7])
  aError += abs(aRef8-aArray[8])
  aError += abs(aRef9-aArray[9])
  aError += abs(aRef10-aArray[10])
  aError += abs(aRef11-aArray[11])
  aError += abs(aRef12-aArray[12])
  aError += abs(aRef13-aArray[13])
  aError += abs(aRef14-aArray[14])
  aError += abs(aRef15-aArray[15])
  ; Reuse input as channel 0.
  aCopy0x0 = aSource
  aCopy0x0, aCopy0x1, aCopy0x2, aCopy0x3, aCopy0x4, aCopy0x5, aCopy0x6, aCopy0x7, aCopy0x8, aCopy0x9, aCopy0x10, aCopy0x11, aCopy0x12, aCopy0x13, aCopy0x14, aCopy0x15 bformenc1 aCopy0x0, p4, p5
  aReuse0[] init 16
  aReuse0[0] = aSource
  aReuse0 bformenc1 aReuse0[0], p4, p5
  aError += abs(aRef0-aCopy0x0) + abs(aRef0-aReuse0[0])
  aError += abs(aRef1-aCopy0x1) + abs(aRef1-aReuse0[1])
  aError += abs(aRef2-aCopy0x2) + abs(aRef2-aReuse0[2])
  aError += abs(aRef3-aCopy0x3) + abs(aRef3-aReuse0[3])
  aError += abs(aRef4-aCopy0x4) + abs(aRef4-aReuse0[4])
  aError += abs(aRef5-aCopy0x5) + abs(aRef5-aReuse0[5])
  aError += abs(aRef6-aCopy0x6) + abs(aRef6-aReuse0[6])
  aError += abs(aRef7-aCopy0x7) + abs(aRef7-aReuse0[7])
  aError += abs(aRef8-aCopy0x8) + abs(aRef8-aReuse0[8])
  aError += abs(aRef9-aCopy0x9) + abs(aRef9-aReuse0[9])
  aError += abs(aRef10-aCopy0x10) + abs(aRef10-aReuse0[10])
  aError += abs(aRef11-aCopy0x11) + abs(aRef11-aReuse0[11])
  aError += abs(aRef12-aCopy0x12) + abs(aRef12-aReuse0[12])
  aError += abs(aRef13-aCopy0x13) + abs(aRef13-aReuse0[13])
  aError += abs(aRef14-aCopy0x14) + abs(aRef14-aReuse0[14])
  aError += abs(aRef15-aCopy0x15) + abs(aRef15-aReuse0[15])
  ; Reuse input as channel 8.
  aCopy1x8 = aSource
  aCopy1x0, aCopy1x1, aCopy1x2, aCopy1x3, aCopy1x4, aCopy1x5, aCopy1x6, aCopy1x7, aCopy1x8, aCopy1x9, aCopy1x10, aCopy1x11, aCopy1x12, aCopy1x13, aCopy1x14, aCopy1x15 bformenc1 aCopy1x8, p4, p5
  aReuse1[] init 16
  aReuse1[8] = aSource
  aReuse1 bformenc1 aReuse1[8], p4, p5
  aError += abs(aRef0-aCopy1x0) + abs(aRef0-aReuse1[0])
  aError += abs(aRef1-aCopy1x1) + abs(aRef1-aReuse1[1])
  aError += abs(aRef2-aCopy1x2) + abs(aRef2-aReuse1[2])
  aError += abs(aRef3-aCopy1x3) + abs(aRef3-aReuse1[3])
  aError += abs(aRef4-aCopy1x4) + abs(aRef4-aReuse1[4])
  aError += abs(aRef5-aCopy1x5) + abs(aRef5-aReuse1[5])
  aError += abs(aRef6-aCopy1x6) + abs(aRef6-aReuse1[6])
  aError += abs(aRef7-aCopy1x7) + abs(aRef7-aReuse1[7])
  aError += abs(aRef8-aCopy1x8) + abs(aRef8-aReuse1[8])
  aError += abs(aRef9-aCopy1x9) + abs(aRef9-aReuse1[9])
  aError += abs(aRef10-aCopy1x10) + abs(aRef10-aReuse1[10])
  aError += abs(aRef11-aCopy1x11) + abs(aRef11-aReuse1[11])
  aError += abs(aRef12-aCopy1x12) + abs(aRef12-aReuse1[12])
  aError += abs(aRef13-aCopy1x13) + abs(aRef13-aReuse1[13])
  aError += abs(aRef14-aCopy1x14) + abs(aRef14-aReuse1[14])
  aError += abs(aRef15-aCopy1x15) + abs(aRef15-aReuse1[15])
  ; Reuse input as channel 15.
  aCopy2x15 = aSource
  aCopy2x0, aCopy2x1, aCopy2x2, aCopy2x3, aCopy2x4, aCopy2x5, aCopy2x6, aCopy2x7, aCopy2x8, aCopy2x9, aCopy2x10, aCopy2x11, aCopy2x12, aCopy2x13, aCopy2x14, aCopy2x15 bformenc1 aCopy2x15, p4, p5
  aReuse2[] init 16
  aReuse2[15] = aSource
  aReuse2 bformenc1 aReuse2[15], p4, p5
  aError += abs(aRef0-aCopy2x0) + abs(aRef0-aReuse2[0])
  aError += abs(aRef1-aCopy2x1) + abs(aRef1-aReuse2[1])
  aError += abs(aRef2-aCopy2x2) + abs(aRef2-aReuse2[2])
  aError += abs(aRef3-aCopy2x3) + abs(aRef3-aReuse2[3])
  aError += abs(aRef4-aCopy2x4) + abs(aRef4-aReuse2[4])
  aError += abs(aRef5-aCopy2x5) + abs(aRef5-aReuse2[5])
  aError += abs(aRef6-aCopy2x6) + abs(aRef6-aReuse2[6])
  aError += abs(aRef7-aCopy2x7) + abs(aRef7-aReuse2[7])
  aError += abs(aRef8-aCopy2x8) + abs(aRef8-aReuse2[8])
  aError += abs(aRef9-aCopy2x9) + abs(aRef9-aReuse2[9])
  aError += abs(aRef10-aCopy2x10) + abs(aRef10-aReuse2[10])
  aError += abs(aRef11-aCopy2x11) + abs(aRef11-aReuse2[11])
  aError += abs(aRef12-aCopy2x12) + abs(aRef12-aReuse2[12])
  aError += abs(aRef13-aCopy2x13) + abs(aRef13-aReuse2[13])
  aError += abs(aRef14-aCopy2x14) + abs(aRef14-aReuse2[14])
  aError += abs(aRef15-aCopy2x15) + abs(aRef15-aReuse2[15])
  kError max_k aError, 1, 1
  if !(kError < .00001) then
    printks "bformenc1 input reuse differs in instrument %g: %g\n", 0, p1, kError
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 9 then
    prints "bformenc1 checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .004 35 27
i 1 .010375 .001 -70 -40
i 1 .02 .004 0 0
i 2 0 .004 35 27
i 2 .010375 .001 -70 -40
i 2 .02 .004 0 0
i 3 0 .004 35 27
i 3 .010375 .001 -70 -40
i 3 .02 .004 0 0
i 99 .03 .001
e
</CsScore>
</CsoundSynthesizer>
