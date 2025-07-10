<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>
0dbfs = 1


opcode testop1m, i[], i[]
  iarr[] xin
  xout iarr*iarr
endop

opcode testop1m, i[], i[]i
  iarr[],imult xin
  xout iarr*imult
endop

opcode testop2m, i[], i[]
  iarr[] xin
  iarr *= iarr
  xout iarr
endop

opcode testop2m, i[], i[]i
  iarr[],imult xin
  iarr *= imult
  xout iarr
endop

opcode testop1a, i[], i[]
  iarr[] xin
  xout iarr+iarr
endop

opcode testop1a, i[], i[]i
  iarr[],imult xin
  xout iarr+imult
endop

opcode testop2a, i[], i[]
  iarr[] xin
  iarr += iarr
  xout iarr
endop

opcode testop2a, i[], i[]i
  iarr[],imult xin
  iarr += imult
  xout iarr
endop

opcode testop1d, i[], i[]
  iarr[] xin
  xout iarr/iarr
endop

opcode testop1d, i[], i[]i
  iarr[],imult xin
  xout iarr/imult
endop

opcode testop2d, i[], i[]
  iarr[] xin
  iarr /= iarr
  xout iarr
endop

opcode testop2d, i[], i[]i
  iarr[],imult xin
  iarr /= imult
  xout iarr
endop

opcode testop1s, i[], i[]
  iarr[] xin
  xout iarr-iarr
endop

opcode testop1s, i[], i[]i
  iarr[],imult xin
  xout iarr-imult
endop

opcode testop2s, i[], i[]
  iarr[] xin
  iarr -= iarr
  xout iarr
endop


opcode testop2s, i[], i[]i
  iarr[],imult xin
  iarr -= imult
  xout iarr
endop

instr arraytestm
  iarr[] fillarray 1, 2, 3, 4
  iarr = testop1m(iarr)
  printarray iarr
  iarr = testop1m(iarr,2)
  printarray iarr
  iarr = testop2m(iarr)
  printarray iarr
  iarr = testop2m(iarr,2)
  printarray iarr
endin
schedule(arraytestm, 0, 1)
instr arraytesta
  iarr[] fillarray 1, 2, 3, 4
  iarr = testop1a(iarr)
  printarray iarr
  iarr = testop1a(iarr,2)
  printarray iarr
  iarr = testop2a(iarr)
  printarray iarr
  iarr = testop2a(iarr,2)
  printarray iarr
endin
schedule(arraytesta, 0, 1)
instr arraytestd
  iarr[] fillarray 1, 2, 3, 4
  iarr = testop1d(iarr)
  printarray iarr
  iarr = testop1d(iarr,2)
  printarray iarr
  iarr = testop2d(iarr)
  printarray iarr
  iarr = testop2d(iarr,2)
  printarray iarr
endin
schedule(arraytestd, 0, 1)
instr arraytests
  iarr[] fillarray 1, 2, 3, 4
  iarr = testop1s(iarr)
  printarray iarr
  iarr = testop1s(iarr,2)
  printarray iarr
  iarr = testop2s(iarr)
  printarray iarr
  iarr = testop2s(iarr,2)
  printarray iarr
endin
schedule(arraytests, 0, 1)

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>


