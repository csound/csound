<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

gkUdoFirst init 0
gkUdoSecond init 0
gkSubFirst init 0
gkSubSecond init 0

opcode NestedDiskin():a
  audio:a diskin2 "fox.wav"
  xout audio
endop

instr 1
  audio:a = NestedDiskin()
  level:k rms audio
  if (p4 == 1) then
    gkUdoFirst = max(gkUdoFirst, level)
  else
    gkUdoSecond = max(gkUdoSecond, level)
  endif
endin

instr 10
  audio:a diskin2 "fox.wav"
  out audio
endin

instr 2
  audio:a subinstr 10
  level:k rms audio
  if (p4 == 1) then
    gkSubFirst = max(gkSubFirst, level)
  else
    gkSubSecond = max(gkSubSecond, level)
  endif
endin

instr 3
  firstUdo:i = i(gkUdoFirst)
  secondUdo:i = i(gkUdoSecond)
  firstSub:i = i(gkSubFirst)
  secondSub:i = i(gkSubSecond)

  if (firstUdo <= 0 || secondUdo < firstUdo * 0.9) then
    prints "UDO diskin2 reuse failed: first=%f second=%f\n", \
      firstUdo, secondUdo
    exitnow(1)
  endif
  if (firstSub <= 0 || secondSub < firstSub * 0.9) then
    prints "subinstr diskin2 reuse failed: first=%f second=%f\n", \
      firstSub, secondSub
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0.0 0.1 1
i 1 0.2 0.1 2
i 2 0.4 0.1 1
i 2 0.6 0.1 2
i 3 0.8 0
e 0.9
</CsScore>
</CsoundSynthesizer>
