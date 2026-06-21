<CsoundSynthesizer>
<CsOptions>
-n -F catherine.mid -T 
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls=1

instr 1
  kcps cpsmidib 2
  iamp ampmidi 0dbfs
  a2 oscili iamp, kcps
  a2 linenr  a2, 0.1,0.01,0.01
       	out a2*0.01
	   	
endin

instr 2
i1 miditempo
print i1
midifiletempo p4
i1 miditempo
if i1 != p4 then
  exitnow(-1)
endif
endin

instr 3
midifilepos p4
endin

instr 4
midifileloop p4
endin

</CsInstruments>
<CsScore>
i2 5 0 150
i3 10 0 20
i3 70 0 20
i4 0 0 1
i4 60 0 0
f0 100
</CsScore>
</CsoundSynthesizer>