<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs=1
test@global:i init 1 // variable declared

instr 1
 test:k init 0   // replaced by new definition - shadowed by a local
 test:a oscili 1000, 440 // local var replaced by new definition
 out oscili(0.5,test+p4)
 prints typeof(test) // type is always of the last definition - never the first
 prints "\n"
endin

instr 2
 print test    // global var
 prints typeof(test) // 'i'
 prints "\n"
endin

num@global:i init 1   // global declaration
num@global:i init 2   // just initialisation of the same

instr 3
num:i init 0   // local var shadows global var
print num     // prints 0
endin

instr 4
num:i init 0 
num@global:i init 2 // WARNING: @global annotation ignored for variable num
endin

instr 5
oscili:i init 0
print oscili
endin

num@global:k init 0 // ok: different context, redefining global

schedule(1,0,1,440)
schedule(1,0,1,100)
schedule(2,1,1)
schedule(3,1,1)
schedule(4,1,1)
schedule(5,1,1)

</CsInstruments>
<CsScore>
f0 2
</CsScore>
</CsoundSynthesizer>

