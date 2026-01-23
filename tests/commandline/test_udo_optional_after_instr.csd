<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>

sr	= 48000
ksmps	=	64
nchnls	=	2
0dbfs	=	1

gk_tempo init 120
gk_clock_tick init 0

opcode now_tick():i
  xout 0
endop

opcode go_tempo(inewtempo, incr):void
  icurtempo = i(gk_tempo)
  itemp init icurtempo

  if(inewtempo > icurtempo) ithen
    itemp = min:i(inewtempo, icurtempo + abs(incr))
    gk_tempo init itemp
  elseif (inewtempo < icurtempo) ithen
    itemp = max:i(inewtempo, icurtempo - abs(incr))
    gk_tempo init itemp
  endif
endop

instr Perform
  ibeat = p4
  schedule("P1", 0, p3, ibeat)
endin

opcode hexbeat(Spat, itick:o):i
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

instr 1
  ival = hexbeat("f3e") * 2 + hexbeat("f1c0") * 2
  print ival
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
