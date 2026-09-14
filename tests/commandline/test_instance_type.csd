<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

/* Csound-test
{
  "description": "testing instance type",
  "expect": {
    "exit": 0
  }
}
*/
myInstrument:InstrDef = createinstr({{
                        out oscili(p4,p5)
                        }})

myInstance:Instr = schedule(myInstrument, 0, 1,
                            0dbfs/2, 440)

</CsInstruments>
<CsScore>
f0 1
</CsScore>
</CsoundSynthesizer>
