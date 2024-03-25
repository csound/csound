<CsoundSynthesizer>
<CsOptions>
-o dac -d
</CsOptions>
<CsInstruments>
sr = 44100
nchnls = 1
ksmps = 32
0dbfs = 1

instr 1
ires compilecsd "does_not_exist.csd"
print ires ; -1 as could not compile
ires compilecsd "my.csd"
print ires ; 0 as compiled successfully
event_i "i", 2, 0, 3, .2, 465 ;send event 
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>

