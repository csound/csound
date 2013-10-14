<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o filepeak.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1 

iscaldb = p4 ;set peak amplitude in dB
ipeak   filepeak "Church.wav"
iscal   = ampdb(iscaldb)/ipeak ;calculate amp multiplier
printf_i "\nPeak value in file '%s' is %f (%.3f dB).\n\n", 1, "Church.wav", ipeak, dbamp(ipeak)

asnd soundin "Church.wav"
     outs asnd, asnd
; scale & write file to disk
asig = asnd*iscal ;scale to p4
fout "Church_norm.wav", 14, asig

endin

instr 2 ; play scaled file

aout   soundin  "Church_norm.wav"
ipknew filepeak "Church_norm.wav"
printf_i "\nPeak value in file '%s' is %f (%.3f dB).\n\n", 1, "Church_norm.wav", ipknew, dbamp(ipknew)
       outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 2 -6 ; normalize audio to -6 dB
i 2 2 2
e
</CsScore>
</CsoundSynthesizer>
