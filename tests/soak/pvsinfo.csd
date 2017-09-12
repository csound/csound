<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac  ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsinfo.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
; create a PVOC-EX (*.pvx) file with PVANAL first
idur filelen  "fox.pvx"		;find duration of analysis file
kpos line     0,p3,idur		;to ensure we process whole file
fsrc pvsfread kpos, "fox.pvx"	;create fsig from (mono) file

iovl,inb,iws,ifmt pvsinfo fsrc	;get info
print iovl,inb,iws,ifmt		;print info

aout pvsynth fsrc
     outs aout, aout

endin
</CsInstruments>
<CsScore>

i 1 0 3
e
</CsScore>
</CsoundSynthesizer>
