<CsTest>
description = "An indented <CsFileB>, <CsSampleB> or <CsFile> tag names its file"
args = ["-nd"]

[expect]
exit = 0
output = ["files 1 1 1"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "files %d %d %d\n", filevalid("csd_indented_fileb.txt"), \
         filevalid("soundin.9732"), filevalid("csd_indented_file.txt")
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
  <CsFileB filename=csd_indented_fileb.txt>
aGVsbG8K
  </CsFileB>
  <CsSampleB filename="9732">
aGVsbG8K
  </CsSampleB>
  <CsFile filename=csd_indented_file.txt>
  </CsFile>
</CsoundSynthesizer>
