<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out  
-odac  -m0 ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o pvsosc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100 
ksmps = 32 
0dbfs  = 1 
nchnls = 2

; example by Menno Knevel 2021

instr 1 ; a band-limited sawtooth-wave oscillator

prints "\nsawtooth-wave\n"		
fsig pvsosc   .5, 440, 1, 1024          ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
     outs asig, asig
endin

instr 2 ; a band-limited square-wave oscillator	

prints "\nsquare-wave\n"	
fsig pvsosc   .5, 440, 2, 1024          ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
     outs asig, asig
endin


instr 3 ; a pulse oscillator

prints "\npulse-wave\n"		
fsig pvsosc   .5, 440, 3, 1024          ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
     outs asig, asig
endin

instr 4 ; a cosine-wave oscillator

prints "\ncosine-wave\n\n"			
fsig pvsosc   .5, 440, 4, 1024          ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
     outs asig, asig
endin

instr 5 ;cycle

ktyp randh 2, 10
fsig pvsosc   .5, 440, int(ktyp+3), 1024 ; make it to go from 1-4
printks2 "\ncycle through all waves...: %d\n", int(ktyp+3)
asig pvsynth fsig                    
     outs asig, asig
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 2 1
i 3 4 1
i 4 6 1
i 5 10 4
e
</CsScore>
</CsoundSynthesizer>
