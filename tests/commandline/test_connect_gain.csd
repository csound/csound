<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; Omitted gain (defaults to 1) vs explicit unity must match.
connect "SourceA", "out", "SinkOmit", "in"
connect "SourceA", "out", "SinkUnity", "in", 1

; Non-unity gain scales the connection.
connect "SourceA", "out", "SinkHalf", "in", 0.5

; Fan-in with per-edge gains: 1 + 0.5 = 1.5
connect "SourceA", "out", "SinkFan", "in", 1
connect "SourceB", "out", "SinkFan", "in", 0.5

; Explicit string-instrument overload with gain.
connect.S "SourceA", "out", "SinkString", "in", 0.25

alwayson "SourceA"
alwayson "SourceB"
alwayson "SinkOmit"
alwayson "SinkUnity"
alwayson "SinkHalf"
alwayson "SinkFan"
alwayson "SinkString"

; timeinstk() counts k-cycles (not seconds). With sr/ksmps above,
; e 0.05 is many cycles, so checks after the first cycle do run.
instr SourceA
  outletk "out", 1
endin

instr SourceB
  outletk "out", 1
endin

instr SinkOmit
  kin inletk "in"
  if timeinstk() >= 2 then
    if abs(kin - 1) > 1e-6 then
      printks "connect gain omit failed: expected=1 got=%f\n", 0, kin
      exitnowk(1)
    endif
  endif
endin

instr SinkUnity
  kin inletk "in"
  if timeinstk() >= 2 then
    if abs(kin - 1) > 1e-6 then
      printks "connect gain unity failed: expected=1 got=%f\n", 0, kin
      exitnowk(1)
    endif
  endif
endin

instr SinkHalf
  kin inletk "in"
  if timeinstk() >= 2 then
    if abs(kin - 0.5) > 1e-6 then
      printks "connect gain 0.5 failed: expected=0.5 got=%f\n", 0, kin
      exitnowk(1)
    endif
  endif
endin

instr SinkFan
  kin inletk "in"
  if timeinstk() >= 2 then
    if abs(kin - 1.5) > 1e-6 then
      printks "connect gain fan-in failed: expected=1.5 got=%f\n", 0, kin
      exitnowk(1)
    endif
  endif
endin

instr SinkString
  kin inletk "in"
  if timeinstk() >= 2 then
    if abs(kin - 0.25) > 1e-6 then
      printks "connect.S gain failed: expected=0.25 got=%f\n", 0, kin
      exitnowk(1)
    endif
  endif
endin
</CsInstruments>
<CsScore>
e 0.05
</CsScore>
</CsoundSynthesizer>
