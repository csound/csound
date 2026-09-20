<CsTest>
description = "createinstr keeps valid references when the instrument table grows"
[expect]
exit = 0
stderr = ["created instrument ran"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1

instr 1
  ; Fill slots 2..199 with aliases of one empty definition.
  SCode = "instr 2"
  iNumber = 3
  while iNumber < 200 do
    SCode strcat SCode, sprintf(",%d", iNumber)
    iNumber += 1
  od
  iResult compilestr strcat(SCode, "\nendin\n")
  if iResult != 0 then
    exitnow -1
  endif
  ; Creating instrument 200 must grow the instrument table.
  ref:InstrDef = createinstr({{ prints "created instrument ran" }})
  if instrnum(ref) != 200 || strlen(str(ref)) != 0 then
    exitnow -1
  endif
  numbered:InstrDef init instrnum(ref)
  schedule numbered, 0, .01
endin
</CsInstruments>
<CsScore>
i 1 0 .01
f 0 .1
e
</CsScore>
</CsoundSynthesizer>
