<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

myinstr:InstrDef = create({{
 sig:a oscili p4, p5
 env:a linen sig, 0.1, p3,0.1
 out env
}})

schedule(myinstr, 0, 1, 0.5, 440)
event_i("e", 0, 2)

</CsInstruments>
<CsScore>
;f0 2
</CsScore>
</CsoundSynthesizer>
