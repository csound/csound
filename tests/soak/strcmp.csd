<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o strcmp.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;modified example from Joachim Heintz
sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

   opcode Triad, iii, S				;define UDO
Sname      xin
iMaj       strcmp     "maj", Sname
iMin       strcmp     "min", Sname
iPrim      =          8.00			;notes in pitch notattion
iQuint     =          8.05
  if iMaj == 0 then
iTer       =          8.03
  elseif iMin == 0 then
iTer       =          8.02
  endif
            xout       iPrim, iTer, iQuint
   endop

instr 1

Sname strget p4
ia, ib, ic Triad Sname				;apply UDO
      print ia, ib, ic
asig1 pluck 0.7, cpspch(ia), 220, 0, 1
asig2 pluck 0.7, cpspch(ib), 220, 0, 1
asig3 pluck 0.7, cpspch(ic), 220, 0, 1
asig  = (asig1+asig2+asig3)*.5
      outs asig, asig

endin
</CsInstruments>
<CsScore>
i1 0 3 "maj"
i1 4 3 "min"
</CsScore>
</CsoundSynthesizer> 

