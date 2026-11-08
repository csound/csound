<CsTest>
description = "UDO turnoff does not read freed output snapshots"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=48000
ksmps=32
nchnls=1
gkA init 3
gkB init 8
gkStopped init 0

opcode StopPair():(k,k)
  gkStopped += 1
  turnoff
  xout gkA, gkB
endop
opcode ClassicStopPair, kk, 0
  gkStopped += 1
  turnoff
  xout gkA, gkB
endop
opcode LocalStopPair, kk, i
  iSize xin
  setksmps iSize
  gkStopped += 1
  turnoff
  xout gkA, gkB
endop
instr 1
  gkB, gkA = StopPair()
endin
instr 2
  gkB, gkA ClassicStopPair
endin
instr 3
  gkB, gkA LocalStopPair 1
endin
instr 4
  gkB, gkA LocalStopPair 8
endin
instr 5
  if i(gkStopped) != 4 then
    prints "UDO turnoff did not stop each call\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i1 0 .01
i2 .02 .01
i3 .04 .01
i4 .06 .01
i5 .08 .01
</CsScore>
</CsoundSynthesizer>
