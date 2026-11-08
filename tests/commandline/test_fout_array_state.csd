<CsTest>
description = "fout uses audio array strides and fout/foutk flush buffered frames at note end"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -s
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

opcode WriteArray, 0, a[]
  setksmps 1
  aInput[] xin
  fout "fout_array.wav", 14, aInput
endop

instr 1
  aChannels[] init 2
  aChannels[0] = .25
  aChannels[1] = -.5
  WriteArray aChannels
  foutk "fout_control.raw", 1, 1
  ; The writer still has 32 stereo frames to flush when this note ends.
  trim aChannels, 0
endin

instr 2
  iLength filelen "fout_array.wav"
  iLeft filepeak "fout_array.wav", 1
  iRight filepeak "fout_array.wav", 2
  iControlLength filelen "fout_control.raw"
  if abs(iLength-32/sr) > 1e-8 || iLeft != .25 || iRight != .5 || iControlLength != 1/sr then
    prints "fout array output is incorrect\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
i 2 .0625 .03125
e
</CsScore>
</CsoundSynthesizer>
