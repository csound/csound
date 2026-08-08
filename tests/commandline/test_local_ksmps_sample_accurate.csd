<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 16
nchnls = 2
0dbfs = 4

gkDirectPasses init 0
gkUdoPasses init 0
gkSubPasses init 0
gkUdoLeft init 0
gkUdoRight init 0
gkSubLeft init 0
gkSubRight init 0

opcode LocalUdo, aa, aa
  setksmps 4
  gkUdoPasses += 1
  aInLeft, aInRight xin
  xout aInLeft, aInRight
endop

opcode LocalOne, 0, 0
  setksmps 1
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endop

opcode LocalOut, 0, 0
  setksmps 4
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endop

instr 1
  setksmps 4
  gkDirectPasses += 1
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endin

instr 2
  setksmps 4
  gkSubPasses += 1
  aLeft = 1
  aRight = 2
  out aLeft, aRight
endin

instr 10
  aInLeft = 1
  aInRight = 2
  aLeft, aRight LocalUdo aInLeft, aInRight
  gkUdoLeft downsamp aLeft, 16
  gkUdoRight downsamp aRight, 16
endin

instr 20
  aLeft, aRight subinstr 2
  gkSubLeft downsamp aLeft, 16
  gkSubRight downsamp aRight, 16
endin

instr 30
  LocalOne
endin

instr 31
  LocalOut
endin

instr 97
  setksmps 4
  aLeft, aRight monitor
  aMix[] monitor
  kLeft downsamp aLeft
  kRight downsamp aRight
  kArrayLeft downsamp aMix[0]
  kArrayRight downsamp aMix[1]
  if kLeft != 2 || kRight != 4 || \
     kArrayLeft != 2 || kArrayRight != 4 then
    exitnowk 1
  endif
endin

instr 98
  aLeft, aRight monitor
  aPhase phasor sr / ksmps
  kLeft downsamp aLeft * aPhase, 16
  kRight downsamp aRight * aPhase, 16

  if timeinstk() == 1 then
    printf "udo ksmps=%d (%.6f, %.6f)\n", 1, p6, kLeft, kRight

    if kLeft != p4 || kRight != p5 then
      exitnowk 1
    endif
  endif
endin

instr 99
  aLeft, aRight monitor
  kDirectLeft downsamp aLeft, 16
  kDirectRight downsamp aRight, 16

  if timeinstk() == 1 then
    printf "direct=%d (%.3f, %.3f), udo=%d (%.3f, %.3f), subinstr=%d (%.3f, %.3f)\n", 1, \
           gkDirectPasses, kDirectLeft, kDirectRight, \
           gkUdoPasses, gkUdoLeft, gkUdoRight, \
           gkSubPasses, gkSubLeft, gkSubRight

    if gkDirectPasses != 1 || gkUdoPasses != 1 || gkSubPasses != 1 || \
       kDirectLeft != 0.25 || kDirectRight != 0.5 || \
       gkUdoLeft != 0.25 || gkUdoRight != 0.5 || \
       gkSubLeft != 0.25 || gkSubRight != 0.5 then
      exitnowk 1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i 1 0.375 0.125
i 10 0.375 0.125
i 20 0.375 0.125
i 99 0 0.5
s
i 31 0.375 0.125
i 98 0 0.5 0.2109375 0.421875 4
s
i 30 0.375 0.03125
i 98 0 0.5 0.046875 0.09375 1
s
i 31 0 0.5
i 31 0 0.5
i 97 0 0.5
</CsScore>
</CsoundSynthesizer>
