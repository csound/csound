<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

opcode assert,0,kk
k1, k2 xin
if k1 != k2 then
 printks "assert error\n", 1
 exitnowk(-1)
endif
endop

instr 1
 sig:Complex[] hilbert oscili(p4,p5)
 mod:Complex[] = oscili(0.5,100,-1,0.25), oscili(0.5,100,-1)
 ssb:Complex[] = mod * sig
   out real(ssb)
endin


instr 2
 ca:Complex[] init 2
 ca += [complex(1,1), complex(1,1)]
 assert(real(ca[0]), 1)
 assert(imag(ca[1]), 1)
 ca *= [complex(2,0), complex(2,0)]
 assert(real(ca[0]), 2)
 assert(imag(ca[1]), 2)
 ca /= [complex(2,0), complex(2,0)]
 assert(real(ca[0]), 1)
 assert(imag(ca[1]), 1)
 ca -= [complex(1,1), complex(1,1)]
 assert(real(ca[0]), 0)
 assert(imag(ca[1]), 0)
endin

</CsInstruments>
<CsScore>
i2 0 1
i1 0 1 0.5 400
</CsScore>
</CsoundSynthesizer>

