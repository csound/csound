<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
instr 1
 ain1 oscili 0dbfs/2, 440
 idsp,a1 faustgen {{
   gain = hslider("vol",1,0,1,0.01);
   process = (_ * gain); 
  }}, ain1
 k1 line  0, p3, 1
 faustctl idsp, "vol", k1
   out a1
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
