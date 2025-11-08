<CsoundSynthesizer>
<CsInstruments>
sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct People count:i, firstNames:S[]
struct Users names:S[]
struct User name:S

instr 1
  SFirstNames[] fillarray "john", "victor", "steven"
  icount = lenarray(SFirstNames)
  people:People init icount, SFirstNames
  print people.count
  SFirstNames2[] = people.firstNames
  SFirstPerson = SFirstNames2[0]
  prints "%s\n", SFirstPerson
endin
opcode mkUser(users:Users):User
  SNames[] = users.names
  prints "SNames length: %d\n", lenarray(SNames)
  prints "SNames[0]: %s\n", SNames[0]
  Susers[] slicearray_i users.names, 0, 1
  prints "Susers length: %d\n", lenarray(Susers)
  user:User init Susers[0]
  xout user
endop
instr 2
  SUsers[] fillarray "a", "b", "c"
  users:Users init SUsers
  prints "users.names length: %d\n", lenarray(users.names)
  prints "users.names[0]: %s\n", users.names[0]

  ; Test simple array assignment
  SSimple[] fillarray "x", "y", "z"
  STestSimple[] = SSimple
  prints "STestSimple length: %d\n", lenarray(STestSimple)
  prints "STestSimple[0]: %s\n", STestSimple[0]

  ; Test struct member assignment
  STestNames[] = users.names
  prints "STestNames length: %d\n", lenarray(STestNames)
  prints "STestNames[0]: %s\n", STestNames[0]
  user:User = mkUser(users)
  prints "should be \"a\": %s\n", user.name
  prints "should be \"c\": %s\n", users.names[2]
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
</CsScore>
</CsoundSynthesizer>