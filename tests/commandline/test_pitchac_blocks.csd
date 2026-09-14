<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0
gkDetected init 0

opcode TrackSample, a, akki
 setksmps 1
 aInput, kMin, kMax, iLow xin
 kPitch pitchac aInput, kMin, kMax, iLow
 aPitch upsamp kPitch
 xout aPitch
endop

; The last active sample must give the same result at either block size,
; including several analysis windows per block and changing window lengths.
instr 1
 kCycle init 0
 kCycle += 1
 kMin = (kCycle % 9 < 4 ? 1024 : 256)
 kMax = (kCycle % 7 < 3 ? 8192 : 1500)
 aInput oscili .5, 700
 if kCycle % 11 == 0 then
  aInput = 0
 endif
 kPitch pitchac aInput, kMin, kMax, p4
 aReference TrackSample aInput, kMin, kMax, p4
 kEarly earlysmps
 kRef vaget ksmps-kEarly-1, aReference
 if !(abs(kPitch-kRef) < .01) then
  printks "pitchac block mismatch: cycle=%g pitch=%g reference=%g\n", 0, kCycle, kPitch, kRef
  exitnowk(-1)
 endif
 gkChecks += 1
 if kPitch > 0 then
  gkDetected += 1
 endif
endin

instr 99
 if i(gkChecks) < 100 || i(gkDetected) == 0 then
  prints "pitchac checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 1 256
; A one-sample window, partial blocks, and a note within one block.
i 1 0 .125 8192
i 1 .0006103515625 .029296875 512
i 1 .002197265625 .00244140625 1024
i 99 1.01 .01
e
</CsScore>
</CsoundSynthesizer>
