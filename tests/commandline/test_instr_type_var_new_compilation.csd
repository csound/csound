<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

giRunCount init 0

ires compilestr {{
instr Ss
 giRunCount += 1
 print p1
endin
 schedule(Ss,0,0)
}}

ires compilestr {{
 schedule(Ss,0,0)
}}

instr CheckResult
  ; Both schedule calls should have run instr Ss
  if giRunCount != 2 then
    prints "FAIL: instr Ss ran %d times, expected 2\n", giRunCount
    exitnow(-1)
  else
    prints "PASS: instr Ss ran %d times as expected\n", giRunCount
  endif
endin

</CsInstruments>
<CsScore>
; Run check after instruments have had time to execute
i "CheckResult" 0.5 0.1
f0 1
</CsScore>
</CsoundSynthesizer>
