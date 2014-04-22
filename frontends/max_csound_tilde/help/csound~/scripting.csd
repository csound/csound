<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 8
nchnls  = 2
0dbfs   = 1

gifn ftgen 1, 0, 16384, 10, 1

instr 1
    kamp   init .2
        
    kpitch chnget "pitch"    ; <~> @maxclass number @varname pitch @minimum 32. @maximum 96. </~>
    kpres  chnget "pressure" ; <~> @maxclass flonum @varname pressure @minimum 1. @maximum 5. </~>
    krat   chnget "bowpos"   ; <~> @maxclass slider @varname bowpos @floatoutput 1 @min .025 @size .205 </~>
    kvibf  chnget "vibf"     ; <~> @maxclass slider @varname vibf @floatoutput 1 @min 0. @size 12. </~>
    kviba  chnget "viba"     ; <~> @maxclass slider @varname viba @floatoutput 1 @min 0. @size 1. </~>
    
    kpitch limit kpitch, 32, 96
    kpres  limit kpres, 1, 5
    krat   limit krat, .025, .3
    
    kfreq  = cpsmidinn(kpitch)
    kviba  = kviba ^ 3
    
    a1     wgbow kamp, kfreq, kpres, krat, kvibf, kviba, gifn
    
           outs a1,a1
endin

</CsInstruments>
<CsScore>
f0 86400
i 1 0 86400
e
</CsScore>
</CsoundSynthesizer>
