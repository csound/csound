<CsoundSynthesizer>
<CsOptions>
</CsOptions>

<CsInstruments>
sr      =           44100
nchnls 	= 		2

gi1 ftgen 1, 0, 4096, 10, 1
gi2 ftgen 2, 0, 4096, 7, -1, 4096, 1    ; sawtooth
gi3 ftgen 3, 0, 4096, 7,  0, 1024, 1, 2048, -1, 1024, 0  ;triangle

instr 101,102,103
  iamp =      ampdbfs(p5)
  a1   oscil  iamp, p4, p1-100
  kenv expseg 1, p3, .01
  a1   =      a1 * kenv
       outs   a1, a1
endin
 
</CsInstruments>
<CsScore bin="csbeats">
; by Brian Baughn 3-14-05
; bbaughn@berklee.net

beats = 100
permeasure = 4

i101    m1 b1   q    mp   D3
i101                      F3
i101                      D3

i101    m2 b1             D3
i101       b3             D3

i101    m3 b1             D3
i101                      F3
i101                      D3

i101    m4 b1             D3
i101       b3             D3

i101    m5 b1              D3
i101       b4              G5

i101    m6 b1              E5
i101       b2              F5
i101       b3     e      Eb5
i101       b3.5   e          
i101       b4     q          

i101    m7 b1     e       D5
i101                            
i101                q          
i101                e      Db5
i101                            
i101                q          

i101    m8 b1     q       D5
i101                         E5
i101                         D5

i102    m1 b2      q      D4
i102       b4              E4
i102       b4             Bb3

i102    m2 b2              F4
i102       b2              B3
i102       b4             C#4
i102       b4             Bb3

i102    m3 b2      q      D4
i102       b4              E4
i102       b4             Bb3

i102    m4 b2              F4
i102       b2              B3
i102       b4             C#4
i102       b4             Bb3

i103    m5 b2      e     F6
i103       b2      e     A5
i103       b2.5    e     D6
i103       b3      e     F6
i103       b3      e     A5
i103       b4      e     E6

i103    m6 b1      q     C#6
i103               q     D6
i103               e     C6
i103                           
i103               q        

i103    m7 b1      e     B5
i103                            
i103                 q         
i103                 e    Bb5
i103                            
i103                 q         

i103     m8 b1     e     F5
i103        b1     e     A5
i103        b1.5   e     D6
i103        b2     e    Bb5
i103        b2.5   e     D6
i103        b3     q     F5
i103        b3           A5
end
</CsScore>

</CsoundSynthesizer>
