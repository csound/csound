; Module with multiple globals and UDOs for testing selective import
giVar1 = 100
giVar2 = 200
giVar3 = 300
giVar4 = 400

; UDO that returns a fixed value
opcode GetValue1():i
    iout = 1000
    xout iout
endop

; UDO that returns a different value
opcode GetValue2():i
    iout = 2000
    xout iout
endop

; UDO that doubles input
opcode DoubleIt(iIn):i
    iout = iIn * 2
    xout iout
endop

; UDO that triples input
opcode TripleIt(iIn):i
    iout = iIn * 3
    xout iout
endop
