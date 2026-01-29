<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

#include "libassert.orc"

opcode test, i, 0
  ival = 5
  xout ival
endop

opcode ov, i, i
  iin xin
  xout iin + 100
endop

;; overload with 2 args - different signature
opcode ov, i, ii
  iin1, iin2 xin
  xout iin1 + iin2 + 200
endop

;; instr 2 is defined BEFORE the UDO redefinition
;; When run before redefinition: expects 5
;; When run after redefinition: expects 7 (picks up new UDO code)
instr 2
  iexpected = p4
  ires = test()
  assertEquals(ires, iexpected)
endin

;; instr 4 checks overloaded UDO selection before redefinition
instr 4
  ;; single-arg version
  iov1 = ov(1)
  assertEquals(iov1, 101)
  ;; two-arg version
  iov2 = ov(1, 2)
  assertEquals(iov2, 203)
endin

;; instr 1 runs the compilestr to redefine the UDO
instr 1
  ires = test()
  assertEquals(ires, 5)

  i1 compilestr {{
opcode test, i, 0
  ival = 7
  xout ival
endop

opcode ov, i, i
  iin xin
  xout iin + 300
endop

instr 99
  ires = test()
  assertEquals(ires, 7)
endin

instr 98
  ;; After redefining ov(i):i, single-arg should use new definition (301)
  iov1 = ov(1)
  assertEquals(iov1, 301)
  ;; Two-arg overload should still use original definition (203)
  iov2 = ov(1, 2)
  assertEquals(iov2, 203)
endin
}}

  schedule(99, 0.05, 0.01)
  schedule(98, 0.06, 0.01)
endin

;; instr 3 triggers instr 2 AFTER instr 1 has redefined the UDO
instr 3
  ;; test that instr 2 picks up the redefined UDO after compilestr
  ires = test()
  assertEquals(ires, 7)
  schedule(2, 0.01, 0.01, 7)
endin

</CsInstruments>
<CsScore>
;; instr 2 runs BEFORE instr 1 redefines the UDO - expects 5
i2 0 0.01 5
;; instr 4 checks overloaded UDOs before redefinition
i4 0 0.01
;; instr 1 redefines the UDO at time 0.02
i1 0.02 0.1
;; instr 3 runs AFTER instr 1 redefines the UDO, triggers instr 2 again
i3 0.15 0.01
</CsScore>
</CsoundSynthesizer>
