<CsoundSynthesizer>
<CsOptions>
-odac -d -F "MIDIpad.mid"
; For Non-realtime ouput leave only the line below:
; -o scanu-musical.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Shengzheng Zhang (aka John Towse)

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

itmp  ftgen 1, 0, 128, 7, 0, 64, 1,64, 0      ; generating all bunch of tables needed
itmp  ftgen 2, 0, 128, -7, 1, 128, 1
itmp  ftgen 3, 0, 16384, -23, "string-128.matrxB"
itmp  ftgen 4, 0, 128,-7 ,0.5 ,128, 0
itmp  ftgen 5, 0, 128, -7, 1, 128, 1
itmp  ftgen 6, 0, 128, -7, 0, 128, 0
itmp  ftgen 7, 0, 128, -7, .001, 128,128

gal   init 0                                 ; getting reverb and delay ready
gar   init 0
gavl  init 0
gavr  init 0

instr 1
kpan    randh 1, 3                           ; random panning
kpan    port kpan, 0.05 ;make the panning change not so quick
kpitch  cpsmidib 12
ipitch  cpsmidi
kenvm   madsr 0.02, 0.6, 0.2, 1
kenv2   madsr 0.1, 0.6, 0.8, 1
kenv3   madsr 0.8, 0.6, 0.8, 1
kvib    lfo 1, 5
ifnvel  = 1
ifnmass = 2
ifnstif = 3
ifncentr = 4
ifndamp = 5
kstif   = 0.3 + 0.1*kvib * kenv3
kmass   = 10-(5*kenvm)                      ; modulated mass
kcentr  = 0.1
kdamp   = -0.02
ileft   = 0.2
iright  = 0.8
kpos    = 0.2
kstrngth = 0.005
;setups for scan synthesis
a2      madsr 0.002, 0.01, 0.001, 0 ;get a initial pick, but seemed like not working 
scanu 1, 0.1,ifnvel, ifnmass,ifnstif,ifncentr, ifndamp,kmass, kstif, kcentr, kdamp, ileft, iright,kpos,kstrngth, a2*0.5, 0.2, 1
a1      scans .01*kenv2, kpitch+kpitch*kvib*kenv3*0.01, 7, 1
a1      dcblock a1
a1      butterlp	a1, kpitch*8
a1      butterlp	a1, kpitch*12
al      = a1*kpan
ar      = a1*(1-kpan)
        outs al, ar
        vincr gal, al
        vincr gar, ar
        vincr gavl, al
        vincr gavr, ar
endin

instr 2
abuf    delayr 5
adl     deltap 0.3
        delayw gal + (adl * 0.2)

abuf    delayr 5
adr     deltap 0.4
        delayw gal + (adr * 0.2)
        
        outs adl, adr
        clear gal
        clear gar
endin

instr 3
adl, adr reverbsc gavl, gavr, 0.85, 12000
        outs adl, adr
        clear gavl
        clear gavr
endin

</CsInstruments>
<CsScore>
i2 0 70
i3 0 70
e
</CsScore>
</CsoundSynthesizer>
