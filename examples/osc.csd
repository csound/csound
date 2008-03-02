<CsoundSynthesizer>
-m3 -otest1
<CsInstruments>

	instr 1
OSCsend 1, "", 7770, "/foo/bar", "fs", 42.0, "bar"
    	endin

</CsInstruments>

<CsScore>
i 1 0 1000.0
e
</CsScore>

</CsoundSynthesizer>
