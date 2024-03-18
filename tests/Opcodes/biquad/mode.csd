<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in    No messages
-odac           -iadc     -d     ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o moogvcf.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; Initialize the global variables.
sr = 44100
kr = 4410
ksmps = 10
nchnls = 2

instr 1; 2 modes excitator

idur init p3
ifreq11 init p4
ifreq12 init p5
iQ11    init p6
iQ12    init p7
iamp    init ampdb(p8)
ifreq21 init p9
ifreq22 init p10
iQ21    init p11
iQ22    init p12

; to simulate the shock between the excitator and the resonator
ashock  mpulse  3,0 

aexc1  mode ashock,ifreq11,iQ11
aexc1 = aexc1*iamp
aexc2  mode ashock,ifreq12,iQ12
aexc2 = aexc2*iamp


aexc = (aexc1+aexc2)/2

;"Contact" condition : when aexc reaches 0, the excitator looses 
;contact with the resonator, and stops "pushing it"
aexc limit aexc,0,3*iamp 

; 2modes resonator

ares1  mode aexc,ifreq21,iQ21
ares2  mode aexc,ifreq22,iQ22

ares = (ares1+ares2)/2

display aexc+ares,p3
outs  aexc+ares,aexc+ares

endin


</CsInstruments>
<CsScore>

;wooden excitator against glass resonator
i1 0 8  1000   3000  12  8  70  440   888   500  420 

;felt against glass
i1 4 8  80   188  8  3  70  440   888   500  420 

;wood against wood
i1 8 8  1000   3000  12  8  70  440  630   60  53 

;felt against wood
i1 12 8  80   180  8  3  70  440  630   60  53 


i1 16 8  1000   3000  12  8  70  440  888   2000  1630
i1 23 8  80   180  8  3  70  440  888   2000  1630


;With a metallic excitator

i1 33 8 1000  1800  1000  720  70   440   882  500  500
i1 37 8 1000  1800  1000  850  70   440   630  60  53

i1 42 8 1000  1800  2000  1720  70   440   442  500  500


</CsScore>
</CsoundSynthesizer>
