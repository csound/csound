<CsoundSynthesizer>
<CsInstruments>
sr=44100
ksmps=1
nchnls=1
0dbfs=1

opcode OscBank,a,kki

setksmps 1
au init 0
kph[] init inum
kcnt init 0
kfr,kamp,inum xin

until kcnt == inum do
au += sin(kph[kcnt])       ; this line segfaults
kph[kcnt] = kph[kcnt] + kfr*kcnt*(2*$M_PI)/sr  ; this line segfaults
kcnt += 1
od
xout au*kamp
au = 0
kcnt = 0

endop


instr 1

aout OscBank 440, .5, 8

outs aout, aout

endin

</CsInstruments>
<CsScore>
i1 0 .5
</CsScore>
</CsoundSynthesizer>


