<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out  
-odac           ;;;RT audio 
; For Non-realtime ouput leave only the line below:
 -o partikkel-getset.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

nchnls 	= 2
0dbfs	= 1

giSine ftgen 0, 0, 65536, 10, 1
giCosine ftgen 0, 0, 8192, 9, 1, 1, 90
giSigmoRise ftgen 0, 0, 8193, 19, 0.5, 1, 270, 1
giSigmoFall ftgen 0, 0, 8193, 19, 0.5, 1, 90, 1

instr 1

; freqstart masking table, simple ascending semitones pattern
iwavfreqstarttab ftgentmp 0, 0, 32, -2, 0, 11,  
    semitone(0), semitone(1), semitone(2), semitone(3),
    semitone(4), semitone(5), semitone(6), semitone(7),
    semitone(8), semitone(9), semitone(10), semitone(11)

iwavfreqendtab ftgentmp 0, 0, 32, -2, 0, 11, 
    semitone(0), semitone(1), semitone(2), semitone(3),
    semitone(4), semitone(5), semitone(6), semitone(7),
    semitone(8), semitone(9), semitone(10), semitone(11)

; channel masking table, simple panning back and forth
ichannelmasks ftgentmp 0, 0, 32, -2, 0, 7, 0.0, 0.25, 0.5, 0.75, 1.0, 0.75, 0.5, 0.25

; init unused arate signals
awavfm = 0
asamplepos1	= 0
async = 0

; system
id = 1

a1,a2 partikkel 6, 0, -1, async, 0, -1, giSigmoRise, giSigmoFall, 
    0.9, 0.5, 100, ampdbfs(-9), -1, 1, 0,iwavfreqstarttab, iwavfreqendtab, awavfm, 
    -1, -1, giCosine, 1, 1, 1, ichannelmasks, 0, 
    giSine, giSine, giSine, giSine, 
    -1, asamplepos1, asamplepos1, asamplepos1, asamplepos1, 
    440, 440, 440, 440, 100, id

outch 1, a1, 2, a2

; using partikkelget and partikkelset to make the channelmask and freqmask indices 
; interact to create a more complex pattern
kfqmask partikkelget 1, id
kchmask partikkelget 4, id
kcount1 init 0
kcount2 init 0
if kchmask > (kfqmask+kchmask)%11  then
partikkelset 1, kcount1, id
partikkelset 2, kcount1, id
kcount1 = (kcount1+1)%7
elseif kfqmask > (kfqmask*kchmask)%7  then
partikkelset 4, kcount2, id
kcount2 = (kcount2+kcount1)%6
endif
        
endin

</CsInstruments>
<CsScore>
i1 0 30
</CsScore>
</CsoundSynthesizer>
