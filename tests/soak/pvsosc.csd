<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o pvsosc.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
kr = 4410
ksmps = 10
nchnls = 1

instr 1
; a band-limited sawtooth-wave oscillator		
fsig  pvsosc   10000, 440, 1, 1024 ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
out asig
endin

instr 2
; a band-limited square-wave oscillator		
fsig  pvsosc   10000, 440, 2, 1024 ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
out asig
endin


instr 3
; a pulse oscillator		
fsig  pvsosc   10000, 440, 3, 1024 ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
out asig
endin

instr 4
; a cosine-wave oscillator		
fsig  pvsosc   10000, 440, 4, 1024 ; generate wave spectral signal
asig pvsynth fsig                       ; resynthesise it
out asig
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 2 1
i 3 4 1
i 4 6 1

e

</CsScore>
</CsoundSynthesizer>
