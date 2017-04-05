;                       A few harmonics...
f   1       0           65536       10      3   0   1   0   0   2 
;                       ...distorted by waveshaping.
f   2       0           65536       13      1   1   0   3   0   2    
;                       Change the tempo, if you like.
t   0      30

;   p1      p2      p3          p4              p5         p6           p7        p8
;   insno   onset   duration    fundamental     numerator  denominator  velocity  pan

; What I want here is just intonation C major 7, G major 7, G 7, C major with voice leading.

i   1       0       60          60              1          1            60       -0.875
i   1       0      180          60              3          2            60        0.000
i   1       0       60          60             28         15            60        0.875

i   1       60      60          60              9          8            60        0.875
i   1       60      30          60              10         7            64       -0.875
i   1       90      30          60              4          3            68        0.875

i   1       120     60          60          	1          1            64       -0.875
i   1       120     60          60          	5          4            62        0.000
i   1       120     60          60         	    2          1            58        0.875

i   30      0       -1

s   10.0
e   10.0

