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

struct Users names:S[]
struct User name:S

opcode mkUser(users:Users):User
  SNames[] = users.names
  Susers[] slicearray_i users.names, 0, 1
  user:User init Susers[0]
  xout user
endop

instr 2
  SUsers[] fillarray "a", "b", "c"
  users:Users init SUsers
  user:User = mkUser(users)
endin

</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
</CsScore>
</CsoundSynthesizer>
