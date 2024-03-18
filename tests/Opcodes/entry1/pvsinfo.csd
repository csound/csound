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

; by Menno Knevel 2021

; analyze sound file and output result to pvoc-ex file
ires  system_i 1,{{ pvanal fox.wav fox.pvx }}          ; default settings
ires1 system_i 1,{{ pvanal -n256 MSjungleMid.wav MSjungleMid.pvx }}    ; different frame size

instr 1

if p4 == 0 then
    idur filelen  "fox.pvx"		;find duration of analysis file
    kpos line     0,p3,idur		;to ensure we process whole file
    fsrc pvsfread kpos, "fox.pvx"	;create fsig from (mono) file
    prints  "\n---***you now hear the analyzed file fox.pvx***---"
else
    idur filelen  "MSjungleMid.pvx"		;find duration of analysis file
    kpos line     0,p3,idur		;to ensure we process whole file
    fsrc pvsfread kpos, "MSjungleMid.pvx"	;create fsig from (mono) file
    prints  "\n---***you now hear the analyzed file MSjungleMid.pvx***---"
endif

iovl,inb,iws,ifmt pvsinfo fsrc	;get info
prints "\n          Overlap size   = %d\n", iovl  ;print info
prints "          Number of bins = %d\n",inb
prints "          Window size    = %d\n",iws
prints "          Frame Format   = %d\n",ifmt
prints "\n"
aout pvsynth fsrc
     outs aout, aout

endin
</CsInstruments>
<CsScore>
i 1 0  3     0  ; the fox
i 1 5 13     1  ; the jungle
e
</CsScore>
</CsoundSynthesizer>
