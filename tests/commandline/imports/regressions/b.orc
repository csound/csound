giValue=22
opcode GetValue, i, i
 iOffset xin
 xout giValue+iOffset
endop
instr Voice
 chnset giValue,"voice_b"
endin
