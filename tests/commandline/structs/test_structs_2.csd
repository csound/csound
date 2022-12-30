<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct People count:i, firstNames:S[]

instr 1
  SFirstNames[] fillarray "john", "victor", "steven"
  people:People init lenarray:i(SFirstNames), SFirstNames
  print people.count
  SFirstNames2[] = people.firstNames
  SFirstPerson = SFirstNames2[0]
  prints "%s\n", SFirstPerson
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
