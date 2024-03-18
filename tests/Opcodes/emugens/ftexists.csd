<CsoundSynthesizer>
<CsOptions>

--nosound 

</CsOptions>

<CsInstruments>

; This is the example file for ftexists

/*

  ftexists

  Returns 1 if a given table index refers to an existing
  ftable

  iexists  ftexists  ifn
  kexists  ftexists  kfn

  Args:
  	ifn / kfn: the table index to query

  Returns:
    iexists / kexists: 1 if a table with index ifn exists, 0 otherwise

*/

gifn1 ftgen 0, 0, 8, 2, 0

instr 1
    iexists1 ftexists gifn1
    print iexists1

    iexists2 ftexists 2
    print iexists2

    kexists ftexists 3
    printf "table 3 exists at time %f", kexists, timeinsts()
endin

</CsInstruments>

<CsScore>

f 2 0 8 2 0
f 3 1.5 8 2 0

i 1 0 2

</CsScore>
</CsoundSynthesizer>