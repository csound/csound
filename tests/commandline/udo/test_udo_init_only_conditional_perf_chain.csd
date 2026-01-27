<CsoundSynthesizer>
<CsInstruments>
sr=48000
ksmps=64
nchnls=2
0dbfs=1

; Test goal: init-only UDOs used inside conditionals should not install a
; perf chain. This covers ternary vs if/then lowering with an i-rate now_tick().

gk_tempo init 120
gk_clock_internal init 0
gk_clock_tick init 0
gk_now init 0

opcode now_tick():i
  xout i(gk_clock_tick)
endop

opcode now_tick():k
  xout gk_clock_tick
endop

opcode hexbeat(Spat, itick:o):i
  ; Ternary expression path (used to trigger perf stub with i-rate UDO).
  tick:i = (itick <= 0) ? now_tick() : itick
  istrlen = strlen(Spat)
  iout = 0
  if (istrlen > 0) then
    ipatlen = strlen(Spat) * 4
    tick = tick % ipatlen
    ipatidx = int(tick / 4)
    ibitidx = tick % 4
    ibeatPat = strtol(strcat("0x", strsub(Spat, ipatidx, ipatidx + 1)))
    iout = (ibeatPat >> (3 - ibitidx)) & 1
  endif
  xout iout
endop

opcode hexbeat_if(Spat, itick:o):i
  ; If/then baseline path for same init-only now_tick() usage.
  if (itick <= 0) then
    tick:i = now_tick()
  else
    tick:i = itick
  endif
  istrlen = strlen(Spat)
  iout = 0
  if (istrlen > 0) then
    ipatlen = strlen(Spat) * 4
    tick = tick % ipatlen
    ipatidx = int(tick / 4)
    ibitidx = tick % 4
    ibeatPat = strtol(strcat("0x", strsub(Spat, ipatidx, ipatidx + 1)))
    iout = (ibeatPat >> (3 - ibitidx)) & 1
  endif
  xout iout
endop

opcode hexplay(Spat, itick, Sinstr, idur, ifreq, iamp:p):void
  if(ifreq > 0 && iamp > 0 && strlen(Sinstr) > 0 && hexbeat(Spat, itick) == 1) then
    schedule(Sinstr, 0, idur, ifreq, iamp )
  endif
endop

opcode hexplay_if(Spat, itick, Sinstr, idur, ifreq, iamp:p):void
  if(ifreq > 0 && iamp > 0 && strlen(Sinstr) > 0 && hexbeat_if(Spat, itick) == 1) then
    schedule(Sinstr, 0, idur, ifreq, iamp )
  endif
endop

opcode hexplay(Spat, Sinstr, idur, ifreq, iamp:p):void
  tick:i = i(gk_clock_tick)

  if(ifreq > 0 && iamp > 0 && strlen(Sinstr) > 0 && hexbeat(Spat, tick) == 1) then
    schedule(Sinstr, 0, idur, ifreq, iamp )
  endif
endop

instr Perform
  ibeat = p4
  schedule("P1_if", 0, p3, ibeat)
  schedule("P1_ternary", 0, p3, ibeat)
endin

instr Clock
  kfreq = (4 * gk_tempo) / 60
  kdur = 1 / kfreq
  kstep = (gk_tempo / 60) / kr
  kstep16th = kfreq / kr
  gk_now += kstep
  gk_clock_internal += kstep16th

  if(gk_clock_internal + kstep16th >= 1.0 ) then
    gk_clock_internal -= 1.0
    gk_clock_tick += 1
    event("i", "Perform", 0, kdur, gk_clock_tick)
  endif
endin

instr P1_if
  ; Baseline: if/then assignment in hexbeat_if.
  hexplay_if("f", p4, "Ex1", p3, 440, 0.5)
endin

instr P1_ternary
  ; Ternary path: should match if/then behavior with init-only UDOs.
  ; First call uses ifreq=0 to skip scheduling, then a normal call follows.
  hexplay("f", "Ex1", p3, 0, 0.5)
  hexplay("f", "Ex1", p3, 440, 0.5)
endin

instr Ex1
  ; dummy
endin

</CsInstruments>
<CsScore>
i "Clock" 0 3
e 3
</CsScore>
</CsoundSynthesizer>
