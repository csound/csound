<CsoundSynthesizer>
<CsOptions>
-n 
</CsOptions>
<CsInstruments>

0dbfs=1

instr 1
 ival[] fillarray p5, p5*1.5
 asig[] init 2
 ref:OpcodeDef init "oscili"
 obj:Opcode[] create ref,2
 n:i = 0
 while n < 2 do
  asig[n] init obj[n], p4, ival[n]
  n += 1
 od
 m:k = 0
 while m < 2 do
  asig[m] perf obj[m], p4, ival[m]
  out asig[m]
  m += 1
 od  
endin

schedule(1,0,1,0.1,440)
</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>

