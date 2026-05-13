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
endin

instr 2
 rewindscore
 pos:i midifilepos 0
 if pos != 0 then
   exitnow(-1)
 endif
 print pos
endin
schedule(2,10,0)

</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>

