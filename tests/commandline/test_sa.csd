<CsoundSynthesizer>
<CsOptions>
-n --sample-accurate
</CsOptions>
<CsInstruments>
0dbfs = 1
ksmps = 10

opcode Test,0,kk
 k2,k1 xin
 koffs offsetsmps
 if koffs != k2 then
  schedulek(2,0,0)
 else
  printk2 koffs
 endif
 kearly earlysmps
 if kearly != k1 then
  schedulek(2,0,0)
 else
  printk2 kearly
 endif
endop

instr 1
kflag init 1
koffs offsetsmps
if koffs == 0 && kflag == 1 then
  schedulek(2,0,0)
else
 kflag = 0
 printk2 koffs
endif

kearly earlysmps
Test koffs,kearly

if release() != 0 then
 if kearly == 0 then
  schedulek(2,0,0)
 else
  printk2 kearly
 endif
endif

endin

instr 2
exitnow(-1)
endin

schedule(1,6/sr,0.5)
</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>

