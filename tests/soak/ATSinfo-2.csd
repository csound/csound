<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-n     ;;;no audio out
</CsOptions>
<CsInstruments>
;example by joachim heintz
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1	; "fox.ats" has been created by ATSanal
Sfile   =       "fox.ats"
isr     ATSinfo Sfile, 0
ifs     ATSinfo Sfile, 1
iws     ATSinfo Sfile, 2
inp     ATSinfo Sfile, 3
inf     ATSinfo Sfile, 4
ima     ATSinfo Sfile, 5
imf     ATSinfo Sfile, 6
id      ATSinfo Sfile, 7
ift     ATSinfo Sfile, 8
        prints  {{
Sample rate =   %d Hz
Frame Size =    %d samples
Window Size =   %d samples
Number of Partials = %d
Number of Frames = %d
Maximum Amplitude = %f
Maximum Frequency = %f Hz
Duration =      %f seconds
ATS file Type = %d
}}, isr, ifs, iws, inp, inf, ima, imf, id, ift
endin
</CsInstruments>
<CsScore>
i 1 0 0 
</CsScore>
</CsoundSynthesizer>
