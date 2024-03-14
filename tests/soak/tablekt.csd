<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if RT audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o tablekt.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

seed 0

gift1 ftgen 1, 0, 1024, 10, 1				;sine wave
gift2 ftgen 2, 0, 1024, 10, 1, 1, 1, 1, 1, 1, 1, 1, 1	;pulse


instr 1
andx phasor 400 		;phasor for reading the index
kfn init 1 			;initialize the choice of the function table
kmetro init 1			;initialize the frequency of the metro
knewft metro kmetro		;make a new choice for selecting the function table once a second

if knewft == 1 then
  kfn = (kfn == 1 ? 2 : 1) 	;switch between 1 and 2
  kmetro random .5, 2 		;create new metro frequency
  printk2 kfn
endif

ares tablekt andx, kfn, 1
outs ares,  ares
endin

</CsInstruments>
<CsScore>
i 1 0 10
e
</CsScore>
</CsoundSynthesizer>

