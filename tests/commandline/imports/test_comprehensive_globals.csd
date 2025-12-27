<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Test comprehensive global variable scenarios
giInt = 42
gkRate init 100
gaAudio init 0
gSString = "hello"
giArray[] fillarray 1, 2, 3, 4, 5

instr 1
  ; Test i-rate global
  if (giInt != 42) then
    prints "FAIL: Expected giInt = 42, got %d\\n", giInt
    exitnow 1
  endif

  ; Test string global
  if (strcmp(gSString, "hello") != 0) then
    prints "FAIL: Expected gSString = 'hello'\\n"
    exitnow 1
  endif

  ; Test array global
  if (giArray[0] != 1) then
    prints "FAIL: Expected giArray[0] = 1, got %d\\n", giArray[0]
    exitnow 1
  endif
  if (giArray[4] != 5) then
    prints "FAIL: Expected giArray[4] = 5, got %d\\n", giArray[4]
    exitnow 1
  endif

  prints "PASS: Comprehensive global variable tests\\n"
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
