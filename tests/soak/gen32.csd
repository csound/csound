<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o gen32.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

itmp    ftgen 1, 0, 16384, 7, 1, 16384, -1		; sawtooth
itmp    ftgen 2, 0, 8192, 10, 1				; sine
itmp    ftgen 5, 0, 4096, -32, -2, 1.5, 1.0, 0.25, 1, 2, 0.5, 0, 1, 3, -0.25, 0.5	; mix tables
itmp    ftgen 6, 0, 16384, 20, 3, 1			; window
; generate band-limited waveforms
inote   =  0
loop0:
icps    =  440 * exp(log(2) * (inote - 69) / 12)        ; one table for
inumh   =  sr / (2 * icps)                              ; each MIDI note number
ift     =  int(inote + 256.5)
itmp    ftgen ift, 0, 4096, -30, 5, 1, inumh
inote   =  inote + 1
        if (inote < 127.5) igoto loop0

instr 1

kcps    expon 20, p3, 16000
kft     =  int(256.5 + 69 + 12 * log(kcps / 440) / log(2))
kft     =  (kft > 383 ? 383 : kft)
a1      phasor kcps
a1      tableikt a1, kft, 1, 0, 1
        outs a1*.5, a1*.5
endin
        
instr 2

kcps    expon 20, p3, 16000
kft     =  int(256.5 + 69 + 12 * log(kcps / 440) / log(2))
kft     =  (kft > 383 ? 383 : kft)
kgdur   limit 10 / kcps, 0.1, 1
a1      grain2 kcps, 0.02, kgdur, 30, kft, 6, -0.5
        outs a1*.08, a1*.08

endin
</CsInstruments>
<CsScore>
t 0 60

i 1 0 10
i 2 12 10
e
</CsScore>
</CsoundSynthesizer>

