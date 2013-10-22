<CsoundSynthesizer>

<CsOptions>
  -d -o dac
</CsOptions>

<CsInstruments>
instr 1
irange   = p4
imu      = p5
isamples = p6
indx     = 0
icount   = 1
ix       = 0.0
ix2      = 0.0

loop:
i1       gauss   irange
i1       =       i1 + imu
ix       =       ix + i1
ix2      =       ix2 + i1*i1
if i1 >= -(irange+imu) && i1 <= (irange+imu) then
  icount = icount+1
endif
         loop_lt indx, 1, isamples, loop
        
imean    =       ix / isamples                         ;mean value
istd     =       sqrt(ix2/isamples - imean*imean)      ;standard deviation
         prints "mean = %3.3f, std = %3.3f, ", imean, istd
         prints "samples inside the given range: %3.3f\%\n", icount*100.0/isamples
endin
</CsInstruments>
<CsScore>
i 1 0   0.1 1.0   0   100000  ; range = 1, mu = 0.0, sigma = 1/3.83 = 0.261
i 1 0.1 0.1 3.83  0   100000  ; range = 3.83, mu = 0.0, sigma = 1
i 1 0.2 0.1 5.745 2.7 100000  ; range = 5.745, mu = 2.7, sigma = 5.745/3.83 = 1.5
</CsScore>
</CsoundSynthesizer>

