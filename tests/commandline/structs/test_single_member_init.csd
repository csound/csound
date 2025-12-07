<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

; Test single-member struct initialization
struct User name:S

instr 1
  ; This should now work with the fix
  user:User init "john"
  prints "User name: %s\n", user.name
  
  if (strcmp(user.name, "john") != 0) then
    prints "Single-member struct initialization failed!\n"
    exitnow 1
  endif
  
  prints "Single-member struct initialization test passed!\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
