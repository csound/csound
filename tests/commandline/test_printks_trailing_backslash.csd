<CsTest>
description = "printks and prints read a format that ends in a backslash no further than its end"

[expect]
exit = 0
output = ["k[]|", "i[]|"]
output_regex = ['\[x{8189}\]\|full']
output_excludes = ["AddressSanitizer", "runtime error:"]
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
  ; A long text first, so that what each variable's buffer holds after the
  ; format's terminator is that text's end: read on past the terminator, the
  ; format would be longer than its buffer, and be refused or overrun it.
  SformatK = "yyyyyyyyyy"
  iDoubling = 0
  while iDoubling < 10 do
    SformatK strcat SformatK, SformatK
    iDoubling += 1
  od
  SformatI = SformatK
  SformatK sprintf "k[%c", 92
  SformatI sprintf "i[%c", 92
  printks SformatK, 0
  printks "]|\n", 0
  prints SformatI, 0
  prints "]|\n"
  turnoff
endin

instr 2
  ; 8191 characters fill the format's buffer, and a backslash after them
  ; adds nothing.
  prints strcat(strcat(strcat("[", Xs(8189)), "]"), sprintf("%c", 92)), 0
  prints "|full\n"
endin

</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
e
</CsScore>
</CsoundSynthesizer>
