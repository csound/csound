<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct Person relativeCount:i, relativeList:Person[]

instr 1
  relatives:Person[] init 2
  john:Person init 2, relatives
  relatives[0].relativeCount = 123
  relatives[1].relativeCount = 543
  print john.relativeList[0].relativeCount
  print john.relativeList[1].relativeCount

  if john.relativeList[0].relativeCount != 123 then
    prints "john.relativeList[0].relativeCount wasn't expected 123"
    exitnow(1)
  endif
  if john.relativeList[1].relativeCount != 543 then
    prints "john.relativeList[0].relativeCount wasn't expected 543"
    exitnow(1)
  endif

endin

</CsInstruments>
<CsScore>
i1 0 0

</CsScore>
</CsoundSynthesizer>
