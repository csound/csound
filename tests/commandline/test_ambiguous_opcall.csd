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

opcode toStrArray, S[], S
  StringIn xin
  SArray[] fillarray StringIn
  xout SArray
endop

instr 1
 // from github issue 1697
 // could be parsed as either of the following, but we want the first:
 // - opcode = chnset, args = k(1), "bla"
 // - out_arg = chnset, opcode = k, args = (1), "bla"
 chnset k(1), "bla"
endin


instr 3
  // found while testing github issue 1964:
  // variable shadowing udo that returns array - we can tell the difference
  String = toStrArray("Inline string-array get")[0]
  prints "%s\n", String
endin
</CsInstruments>
<CsScore>
i 1 0 1
i 3 0 1
</CsScore>
</CsoundSynthesizer>
