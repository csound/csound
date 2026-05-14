<CsoundSynthesizer>
<CsOptions>
-n -F catherine.mid -T
</CsOptions>
<CsInstruments>
0dbfs=1

instr 1
iamp ampmidi 0.5
icps cpsmidi
asig vco2 iamp, icps
a2 linenr asig, 0.001, 0.1, 0.01
   out a2*0.01
itime = round(times:i()*kr)
imtime = round(midifilepos:i()*kr)
if itime != imtime then
    prints "%d  %d\n", itime, imtime
   exitnow(-1)
endif
endin


</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>

