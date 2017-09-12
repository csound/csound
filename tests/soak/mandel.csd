<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o mandel.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>
;example by Brian Evans
sr = 44100
ksmps = 32
nchnls = 2

instr 1

ipitchtable = 1						; pitch table in score      
ipitchndx = p5						; p5=pitch index from table
    
ipitch table  ipitchndx, ipitchtable   
kenv   expseg 1.0, 1.0, 1.0, 11.5, .0001              
asig   pluck  ampdb(p4)*kenv, cpspch(ipitch), cpspch(ipitch), 0, 1
       outs   asig, asig
       
endin
</CsInstruments>
<CsScore>

f1 0 32 -2 6.00 6.02 6.04 6.05 6.07 6.09 6.11		; f1 is a pitch table defining a four octave C major scale starting 
           7.00 7.02 7.04 7.05 7.07 7.09 7.11		; on C two octaves below middle C
           8.00 8.02 8.04 8.05 8.07 8.09 8.11
           9.00 9.02 9.04 9.05 9.07 9.09 9.11

;ins start   dur ampdb(p4) pitchndx(p5)

i1   0       12.0   75     3 
i1   1.5999  12.0   75     4 
i1   3.4000  12.0   75     5 
i1   4.2000  12.0   75     6 
i1   4.4000  12.0   75     7 
i1   4.6000  12.0   75     9 
i1   4.8000  12.0   75     10 
i1   5.0000  12.0   75     5 
i1   5.2000  12.0   75     27 
i1   5.4000  12.0   75     5 
i1   5.6000  12.0   75     20 
i1   6.0000  12.0   75     24 
i1   6.2000  12.0   75     2 
i1   6.4000  12.0   75     27 
i1   6.6000  12.0   75     20 
i1   6.8000  12.0   75     15 
i1   7.0000  12.0   75     3 
i1   7.2000  12.0   75     3 
i1   7.4000  12.0   75     23 
i1   7.6000  12.0   75     9 
i1   7.8000  12.0   75     17 
i1   8.0000  12.0   75     18 
i1   8.2000  12.0   75     3 
i1   8.4000  12.0   75     26 
i1   8.6000  12.0   75     15 
i1   8.8     12.0   75     2 
i1   9       12.0   75     26 
i1   9.2     12.0   75     8 
i1   9.3999  12.0   75     22 
i1   9.5999  12.0   75     22 
i1   9.7999  12.0   75     20 
i1   9.9999  12.0   75     19 
i1   10.399  12.0   75     20 
i1   10.799  12.0   75     22 
i1   10.999  12.0   75     27 
i1   11.199  12.0   75     25 
i1   11.399  12.0   75     20 
i1   11.599  12.0   75     21 
i1   11.799  12.0   75     24 
i1   11.999  12.0   75     24 
i1   12.199  12.0   75     4 
i1   12.399  12.0   75     13 
i1   12.599  12.0   75     15 
i1   12.799  12.0   75     14 
i1   12.999  12.0   75     3 
i1   13.199  12.0   75     21 
i1   13.399  12.0   75     6 
i1   13.599  12.0   75     3 
i1   13.799  12.0   75     10 
i1   13.999  12.0   75     25 
i1   14.199  12.0   75     21 
i1   14.399  12.0   75     20 
i1   14.599  12.0   75     19 
i1   14.799  12.0   75     18 
i1   15.199  12.0   75     17 
i1   15.599  12.0   75     16 
i1   15.999  12.0   75     15 
i1   16.599  12.0   75     14 
i1   17.199  12.0   75     13 
i1   18.399  12.0   75     12 
i1   18.599  12.0   75     11 
i1   19.199  12.0   75     10 
i1   19.799  12.0   75     9 
e
</CsScore>
</CsoundSynthesizer>

