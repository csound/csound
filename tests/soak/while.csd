<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o ifthen.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

instr 1
lab99:
if p4<0 goto lab100
  p4 = p4-1
  print p4
  goto lab99
lab100:
endin

instr 2
  while p4>=0 do
    p4 = p4-1
    print p4
  od
endin
</CsInstruments>
<CsScore>
i 1 1 1 4
i 2 2 1 4
e

</CsScore>
</CsoundSynthesizer>
