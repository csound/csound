<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

sr = 1000
ksmps = 10
nchnls = 1

struct InstrumentRecord name:S

channelDimensions@global:i[] fillarray 1
chnarray "instrument-records", 3, "InstrumentRecord", channelDimensions

instr 1
  records:InstrumentRecord[] init 1
  record:InstrumentRecord init "cello"
  records[0] = record
  chnset records, "instrument-records"

  ; Managed elements cannot be zeroed safely with a byte-wise channel clear.
  chncleararray "instrument-records"
endin

</CsInstruments>
<CsScore>
i 1 0 0.02
</CsScore>
</CsoundSynthesizer>
