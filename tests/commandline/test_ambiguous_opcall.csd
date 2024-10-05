<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

chn_k "bla",2

instr 1
 // from github issue 1697
 // could be parsed as either of the following, but we want the first:
 // - opcode = chnset, args = k(1), "bla"
 // - out_arg = chnset, opcode = k, args = (1), "bla"
 chnset k(1), "bla"
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
