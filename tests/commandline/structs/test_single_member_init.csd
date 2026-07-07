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
struct Point x:i, y:i
struct Label name:S, value:i

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

instr 2
  globalPoint@global:Point init
  point0:Point init
  label0:Label init

  if (globalPoint.x != 0 || globalPoint.y != 0) then
    prints "Default global point initialization failed: x=%d y=%d\n", \
      globalPoint.x, globalPoint.y
    exitnow 1
  endif

  if (point0.x != 0 || point0.y != 0) then
    prints "Default point initialization failed: x=%d y=%d\n", \
      point0.x, point0.y
    exitnow 1
  endif

  if (strcmp(label0.name, "") != 0 || label0.value != 0) then
    prints "Default label initialization failed: name='%s' value=%d\n", \
      label0.name, label0.value
    exitnow 1
  endif

  prints "Default struct initialization test passed!\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.1
e
</CsScore>
</CsoundSynthesizer>
