<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 8
nchnls  = 2

giSine       ftgen 1, 0, 4097, 10, 1
giTriangleBi ftgen 2, 0, 4097, 7, 0, 1024, 1, 1024, 0, 1024, -1, 1024, 0
giSquareBi   ftgen 3, 0, 4097, 7, 1, 2048, 1, 0, -1, 2048, -1
giUpBi       ftgen 4, 0, 4097, 7, -1, 4096, 1
giDownBi     ftgen 5, 0, 4097, 7, 1, 4096, -1

instr 1
    ainL inch 1
    ainR inch 2

    kmodDepth chnget "mod"
    kmodFreq  chnget "frq"
    kmodType  chnget "typ"
    kfiltFreq chnget "fcf"
    kfiltType chnget "ftype"
    kbalance  chnget "bal"
    kreso     chnget "reso"

    if kmodType < 5 then
        kTable = kmodType + 1
        kmodd oscilikt kmodDepth, kmodFreq, kTable
    else
        kmodd   randh  kmodDepth, kmodFreq, .365
    endif

    kfiltFreq = octcps(kfiltFreq)
    kmodd     = cpsoct(kmodd+kfiltFreq)

    kfr       limit kmodd, 40, 9300
    kfr       port kfr, .04  
    
    ; Prevent CPU spikes when audio is very silent.
    ; Only applies to Intel CPU's.
    denorm ainL, ainR

    aloL, ahiL, abdL svfilter ainL, kfr, kreso, 1
    aloR, ahiR, abdR svfilter ainR, kfr, kreso, 1   
 
    if kfiltType == 0 then
        aoutL   =   aloL
        aoutR   =   aloR 
    elseif kfiltType == 1 then
        aoutL   =   ahiL
        aoutR   =   ahiR
    else
        aoutL   =   abdL
        aoutR   =   abdR 
    endif

    if kbalance > 0 then
        aoutL   balance aoutL, ainL
        aoutR   balance aoutR, ainR 
    endif

    outch 1, aoutL, 2, aoutR    
endin


</CsInstruments>
<CsScore>
f0 86400
i1 0 86400
e
</CsScore>
</CsoundSynthesizer>







