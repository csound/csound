<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

; Define basic struct types
struct Point x:i, y:i

instr 1
  ; This should cause a compilation error - accessing non-existent member
  point1:Point init 10, 20
  
  ; Try to access a member that doesn't exist
  point1.nonexistent = 42
  
  prints "This should not print because compilation should fail\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
