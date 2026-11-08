giValue=11
giValues[] fillarray 1,2,3
gSLabel="alpha"
opcode GetValue, i, i
 iOffset xin
 xout giValue+iOffset
endop
opcode Over, i, i
 iX xin
 xout iX+1
endop
opcode Over, i, ii
 iX,iY xin
 xout iX+iY
endop
struct ModuleVec x:i,y:i
gVec@global:ModuleVec init 10,20
instr Voice
 chnset giValue,"voice_a"
endin
