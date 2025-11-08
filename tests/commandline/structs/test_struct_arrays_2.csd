<CsoundSynthesizer>
<CsInstruments>
sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1
; Test basic struct-to-struct references (core requirement)
struct Address street:i, city:i
struct Person name:i, address:Address

instr 1
  ; Test struct with struct member (struct-to-struct reference)
  addr1:Address init 100, 200
  person1:Person init 42, addr1

  ; Test accessing nested struct members
  iName = person1.name
  iStreet = person1.address.street
  iCity = person1.address.city

  prints "person1.name=%d\n", iName
  prints "person1.address.street=%d\n", iStreet
  prints "person1.address.city=%d\n", iCity

  ; Test completed - struct-to-struct references are working!
  prints "Struct-to-struct reference test completed successfully!\n"
endin
</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>