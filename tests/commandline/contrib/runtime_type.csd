<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

gi_var = 22
gk_var = 33
gS_var = "TEST"

opcode testUDO, 0, iiS 

i0, i1, S2  xin

print i0
print_type i0
print i1
print_type i1
prints S2
print_type S2 

prints "\nTEST UDO\n"
prints "Testing constants...\n"
print 44
print_type 44
print_type "test"

prints "Testing Global Vars...\n"
print_type gi_var
print_type gk_var
print_type gS_var

ivar = 300
kvar = 400
avar init 0

prints "Testing Local Vars...\n"
print_type ivar
print ivar
print_type kvar
print_type avar

prints "Testing PFields...\n"
print_type p1 
print p1
print_type p2 
print p2
print_type p3 
print p3
print_type p4 
print p4
print_type p5 
prints p5


endop

instr testInstr 	

prints "Testing constants...\n"
print 44
print_type 44
print_type "test"

prints "Testing Global Vars...\n"
print_type gi_var
print_type gk_var
print_type gS_var

ivar = 300
kvar = 400
avar init 0

prints "Testing Local Vars...\n"
print_type ivar
print ivar
print_type kvar
print_type avar

prints "Testing PFields...\n"
print_type p1 
print p1
print_type p2 
print p2
print_type p3 
print p3
print_type p4 
print p4
print_type p5 
prints p5

testUDO(1, 2, "testUDO")

turnoff

endin

instr 2

print_type p4
prints p4
print_type p5
prints p5
print_type p6
prints p6

turnoff

endin

instr 3
pset 3, 0, 4, 5, 6

print p3
print p4
print p5
turnoff

endin
</CsInstruments>

; ==============================================
<CsScore>
i "testInstr" 0 .5 999 "testPfield"
i2 0 3 "aaa" "bbb" "ccc"
i3 0 3
</CsScore>

</CsoundSynthesizer>

