<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1

opcode Crashy():i
  ikeepGoing = 1

  while (ikeepGoing == 1) do
    if (ikeepGoing == 1) then
      xout -1
    endif

    ivalue = Crashy()
  od

  xout 0
endop

instr 1
  ivalue = Crashy()
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>