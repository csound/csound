<CsTest>
description = "a preallocated slicearray output can change length under --realtime"
args = []
[expect]
exit = 0
output = ["preallocated realtime slice reached 3 elements"]
output_excludes = ["warning: realloc", "PERF ERROR", "AddressSanitizer", "runtime error:"]
[profiles.wasm]
skip = "The command-line WASI runtime does not support --realtime."
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null -d -m4 --realtime
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aSource[] init 3
  trimi aSource, 1

  ; Reserving the largest output at init avoids growth during performance.
  aSlice[] init 3
  aSlice slicearray aSource, 0

  ; Fill the source for the next block only after restoring its length.
  ; Indexed audio writes beyond the current length would allocate storage.
  trim aSource, 3
  aSource[0] = 10
  aSource[1] = 20
  aSource[2] = 30

  kCycle timeinstk
  kLength lenarray aSlice
  kLastIndex init 2
  if kCycle == 1 && kLength != 1 then
    printks "expected one element in the first slice, got %d\n", 0, kLength
    exitnowk -1
  endif
  if kCycle == 2 then
    if kLength != 3 then
      exitnowk -1
    endif
    kLast downsamp aSlice[kLastIndex]
    if kLast != 30 then
      exitnowk -1
    endif
    printks "preallocated realtime slice reached 3 elements\n", 0
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .25
e
</CsScore>
</CsoundSynthesizer>
