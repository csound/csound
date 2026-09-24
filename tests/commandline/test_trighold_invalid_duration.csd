<CsTest>
description = "trighold rejects invalid durations when starting a hold"

[expect]
exit = "nonzero"
stderr = ["trighold: duration is out of range"]
stderr_regex = ['8 errors in performance']
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1

instr RejectControlDuration
  SSampleCount strget p4
  iSampleCount strtod SSampleCount
  kTrigger = 1
  kHeld trighold kTrigger, iSampleCount/kr
endin

instr RejectAudioDuration
  SSampleCount strget p4
  iSampleCount strtod SSampleCount
  aTrigger = 1
  aHeld trighold aTrigger, iSampleCount/sr
endin
</CsInstruments>
<CsScore>
; Negative, NaN, infinite, and 2^64 sample counts cannot describe a hold.
i "RejectControlDuration" 0 [8/8192] "-1"
i "RejectControlDuration" 0 [8/8192] "nan"
i "RejectControlDuration" 0 [8/8192] "inf"
i "RejectControlDuration" 0 [8/8192] "18446744073709551616"
i "RejectAudioDuration" 0 [8/8192] "-1"
i "RejectAudioDuration" 0 [8/8192] "nan"
i "RejectAudioDuration" 0 [8/8192] "inf"
i "RejectAudioDuration" 0 [8/8192] "18446744073709551616"
e
</CsScore>
</CsoundSynthesizer>
