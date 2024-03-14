<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen23.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
;"spectrum.txt" is created by the spectrum plotter of Audacity (set at size 128), using "fox.wav".

instr 1	;performs additive synthesis based on spectrum.txt

indx =0						;start reading at first value
loop:
ifreq tab_i indx, 2				;take odd values of list (= frequency)
iamp tab_i indx+1, 2				;take even values of list (= amplitude)
event_i "i", 10, 0, p3, iamp, ifreq   		;use "event_i" to trigger instr. 10 
    loop_lt indx, 2, 126, loop			;use all 126 frequency and amplitude values

endin

instr 10 ;generate sound

iamp  = p4
ifreq = p5
asig  poscil ampdb(iamp), ifreq, 1
asig  linen asig, .01, p3, p2
      outs  asig, asig

endin
</CsInstruments>
<CsScore>
f 1 0 16384 10 1		;sine wave
f 2 0 128 -23 "spectrum.txt"	;"spectrum.txt" can be found in /manual/examples

i1 0 2

e
</CsScore>
</CsoundSynthesizer>

