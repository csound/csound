<CsoundSynthesizer>
<CsOptions>
-n -F catherine.mid -T --midi-velocity-amp=4 --midi-key-cps=5
</CsOptions>
<CsInstruments>
/* Csound-test
{
  "description": "test midi default instr",
  "expect": {
    "exit": 0
  }
}
*/
out(madsr:a(0.01,0.1,0.7,0.1)*vco2(p4*0.5,p5))
</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>