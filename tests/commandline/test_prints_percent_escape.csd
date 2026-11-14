<CsTest>
description = "In a prints or printks format, %% is a percent sign, and the letter after it is not taken for %n, %t, %r or %!"

[expect]
exit = "nonzero"
stderr_regex = ['INIT ERROR in instr 2 .*format too long in formatted printing']
output = ["a 50%rise 1|", "b 50%nothing 2|", "c 50%tall 3|", "d 50%! 4|", "k 50%rise 5|"]
output_regex = ['\[x{8188}%\|fits']
output_excludes = ["%%", "TOOLONG"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

opcode Xs(count:i):S
  text:S = "x"
  while strlen(text) < count do
    text = strcat(text, text)
  od
  xout strsub(text, 0, count)
endop

instr 1
  prints "a 50%%rise %d|\n", 1
  prints "b 50%%nothing %d|\n", 2
  prints "c 50%%tall %d|\n", 3
  prints "d 50%%! %d|\n", 4
  printks "k 50%%rise %d|\n", 0, 5
  ; 8189 characters and %%: 8191, which fit.
  prints strcat(strcat("[", Xs(8188)), "%%"), 0
  prints "|fits\n"
  turnoff
endin

instr 2
  ; One more does not.
  prints strcat(strcat("TOOLONG", Xs(8183)), "%%"), 0
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
e
</CsScore>
</CsoundSynthesizer>
