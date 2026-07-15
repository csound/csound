<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
; a-rate struct members must survive ksmps larger than the parse-time
; default (10): member buffers are sized when the struct type is defined,
; before the header applies, so initializeStructVar has to refresh
; memBlockSize or member writes overrun the allocation (bus error).
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

struct AudioBus left:a, right:a
sig@global:AudioBus = init()
fail@global:k = init(0)

instr 1
  ; local struct, a-rate member write + read back
  source:a = oscili(0.5, 440)
  bus:AudioBus = init()
  bus.left = source * 0.5
  bus.right = source * 0.5
  diff:a = bus.left - bus.right
  d:k = downsamp(diff)
  if abs(d) > 0.0001 then
    fail = 1
  endif
endin

instr 2
  ; global struct, same pattern
  local_sig:a = oscili(0.7, 220)
  sig.left = local_sig * 0.7
  sig.right = local_sig * 0.3
  diff:a = sig.left * 3 - sig.right * 7
  d:k = downsamp(diff)
  if abs(d) > 0.0001 then
    fail = 1
  endif
endin

instr 3
  ; init with a-rate arguments: the only legal arg form for a-rate
  ; members (constants are rejected, see the companion _fails test);
  ; the i-time member copy also runs at the full header ksmps
  source:a = oscili(0.4, 330)
  st:AudioBus = init(source, source)
  st.left = source * 0.5
  st.right = source * 0.5
  diff:a = st.left - st.right
  d:k = downsamp(diff)
  if abs(d) > 0.0001 then
    fail = 1
  endif
endin

instr 99
  if i(fail) > 0 then
    exitnow(-1)
  endif
  prints("ALL OK\n")
endin

</CsInstruments>
<CsScore>
i 1 0 0.2
i 2 0 0.2
i 3 0 0.2
i 99 0.4 0.1
</CsScore>
</CsoundSynthesizer>
