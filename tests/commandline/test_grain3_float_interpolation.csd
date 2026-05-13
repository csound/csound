<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

gkMax init 0

; A non-power-of-two window table forces grain3 into the float-phase path.
giw ftgen 1, 0, 17, 7, 0, 17, 0.5
; A constant positive grain waveform isolates the window interpolation behavior.
gis ftgen 2, 0, 16, 7, 1, 16, 1

instr 1
a1 grain3 220, 0, 0, 0, 0.05, 50, 8, 2, 1, 10, 12345, 0, 0
k1 downsamp a1

if k1 > gkMax then
  gkMax = k1
endif

if k1 < -0.0001 then
  printks "grain3 float-path interpolation produced negative sample: %.9f\n", 0, k1
  exitnowk(-1)
endif
endin

instr 2
if timeinstk() > 0 then
  if gkMax <= 0.0001 then
    printks "grain3 float-path produced no positive output\n", 0
    exitnowk(-1)
  endif
  turnoff
endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.08
i 2 0.081 0.01
e
</CsScore>
</CsoundSynthesizer>
