<CsoundSynthesizer>
<CsOptions>
// -o dac -M hw:1,0,0  ; Terminal
// -Mhw:1,0,0 -+rtmidi=NULL --daemon -dm0 ;  Bela
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1
; by John ffitch and Richard Boulanger
ctrlinit  1, 21,101, 22,11, 23,12, 24,16, 25,0, 26,82      ; Starting Preset
instr 42   ; A Bank of Controller Presets - selected by ASCII Key Numbers 1->9 and a,b,c,d in instr 1 below
 
kpre1  ctrlpreset  0, 1, 21, 101, 22, 94, 23, 01, 24, 06, 25, 14, 26, 02 ; Preset 1 = ASCII 1
kpre2  ctrlpreset  0, 1, 21, 102, 22, 57, 23, 76, 24, 55, 25, 77, 26, 12 ; Preset 2 = ASCII 2
kpre3  ctrlpreset  0, 1, 21, 103, 22, 12, 23, 13, 24, 16, 25, 84, 26, 22 ; Preset 3 = ASCII 3
kpre4  ctrlpreset  0, 1, 21, 104, 22, 83, 23, 18, 24, 13, 25, 24, 26, 32 ; Preset 4 = ASCII 4
kpre5  ctrlpreset  0, 1, 21, 105, 22, 20, 23, 48, 24, 33, 25, 94, 26, 42 ; Preset 5 = ASCII 5
kpre6  ctrlpreset  0, 1, 21, 106, 22, 52, 23, 10, 24, 03, 25, 68, 26, 52 ; Preset 6 = ASCII 6
kpre7  ctrlpreset  0, 1, 21, 107, 22, 09, 23, 02, 24, 07, 25, 01, 26, 02 ; Preset 7 = ASCII 7
kpre8  ctrlpreset  0, 1, 21, 108, 22, 34, 23, 10, 24, 96, 25, 44, 26, 42 ; Preset 8 = ASCII 8
kpre9  ctrlpreset  0, 1, 21, 109, 22, 04, 23, 91, 24, 66, 25, 04, 26, 92 ; Preset 9 = ASCII 9
kpre10 ctrlpreset  0, 1, 21, 110, 22, 04, 23, 10, 24, 66, 25, 44, 26, 22 ; Preset 10 = ASCII 'a'
kpre11 ctrlpreset  0, 1, 21, 111, 22, 07, 23, 46, 24, 45, 25, 17, 26, 32 ; Preset 11 = ASCII 'b'
kpre12 ctrlpreset  0, 1, 21, 112, 22, 22, 23, 03, 24, 06, 25, 04, 26, 42 ; Preset 12 = ASCII 'c'
kpre13 ctrlpreset  0, 1, 21, 113, 22, 43, 23, 98, 24, 93, 25, 94, 26, 52 ; Preset 13 = ASCII 'd'
ctrlprintpresets ; print the CC Presets in the console in response to the 'i42 0 0.1' note in score
ctrlprintpresets "./my_ctrlpresets.txt"   ; print the CC Presets to a file
turnoff
endin
instr  1
kc[] init 6
icps cpsmidi
iamp ampmidi 0.6
kvol midic7  21, 0,1
kcar midic7  22, 1,10
kmod midic7  23, .1,10
kndx midic7  24, 1,30
kndx port    kndx,.1
iatk midic7  25, .01,1
irel midic7  26, .01,2
kpre midic7  28, 1,13   ; use CC 28 to select from the preset 1 - 13 above
ktrig changed2 kpre
if ktrig == 1 then
kpre = int(kpre)
  ctrlselect kpre
  printk2 kpre
endif
asig  foscil iamp, icps, kcar, kmod, kndx, 1
kmgate linsegr 0, iatk, 1, irel, 0
outs  (asig*kmgate)*kvol, (asig*kmgate)*kvol
kc   ctrlsave 1, 21,22,23,24,25,26    ; MIDI Channel and CC values to read & save (Chan1 & CC21->CC26)
kchar  sensekey                       ; CC values printed to console after playing next note
if kchar != 65 goto end0              ; ASCII character "65" is the letter 'A' (shift-a)
 ctrlprint kc                         ; prints Controller Settings (CC Presets) to the console
 ctrlprint kc, "./my_ctrlinits.txt"   ; appends Controller Settings (CC Presets) to a file
end0:
if kchar<49 || kchar>57 goto end1     ; ASCII numbers 1 -> 9
 kval = kchar - 48
 ctrlselect kval
end1:
if kchar <97 || kchar>122 goto end2   ; ASCII numbers 10 -> 36 (lower-case letters a -> z)
  kval = (kchar - 97) + 10
  ctrlselect kval
 end2:
if kchar != 86  goto end3                     ; ASCII character "86" is the letter 'V' (shift-v)
 kk ctrlpreset 0,kc                           ; adding and numbering the current state as a preset to the list
 ctrlprintpresets                             ; prints Controller Presets (CC Presets) to the console
 ctrlprintpresets "./my_ctrlpresets.txt"      ; appends Controller Presets (CC Presets) to a file
end3:
endin
</CsInstruments>
<CsScore>
f0 6000
f1 0 8192 10 1
i42 0 0.1
e
</CsScore>
</CsoundSynthesizer>
