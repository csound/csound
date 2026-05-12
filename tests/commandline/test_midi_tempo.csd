<CsoundSynthesizer>
<CsOptions>
-n -F rain.mid -T
</CsOptions>
<CsInstruments>
0dbfs=1
nchnls=1
ksmps=64



instr 1
  itp miditempo
  if int(itp) != 78 then
    print itp
    exitnow(-1)
  endif

  kcps cpsmidib 2
  iamp ampmidi 0dbfs
  //print iamp
  a2 oscili iamp, kcps
  a2 linenr  a2, 0.1,0.1,0.01
       	out a2*0.09	   	
endin

</CsInstruments>
<CsScore>
f0 10000
</CsScore>
</CsoundSynthesizer>