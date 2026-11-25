<CsTest>
description = "--keep-sorted-score writes the CSD's score merged with the --events= score sorted before it"
args = ["-nd", "--keep-sorted-score", "--events=i 1 0 0.1"]

[expect]
exit = 0
output = ["notes in score.srt: 3"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

; score.srt is written before the performance: its notes of instr 1
instr 2
  icount = 0
next:
  Sline, iline readfi "score.srt"
  if iline == -1 igoto done
  if strindex(Sline, "i 1 ") == 0 then
    icount += 1
  endif
  igoto next
done:
  prints "notes in score.srt: %d\n", icount
endin

</CsInstruments>
<CsScore>
i 2 0 0
i 1 0.05 0
i 1 0.5 0
</CsScore>
</CsoundSynthesizer>
