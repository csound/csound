<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>

<CsInstruments>
instr 1
  Swd           pwd
                printf_i      "Working directory is '%s'\n", 1, Swd
                prints        "Reading myself =):\n"
read:
  Sline, kLinNum readf "readf.csd"
                printf "Line %d: %s", kLinNum, kLinNum, Sline
  if kLinNum != -1 then
                kgoto read
  else 
                turnoff
  endif
endin
</CsInstruments>

<CsScore>
i1 0 1
e
</CsScore>

</CsoundSynthesizer>
