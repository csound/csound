<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2
0dbfs = 1.0

; Example file for linlin.csd

/*

linlin

linear to linear interpolation between two ranges

ky    linlin kx, ky0, ky1, kx0=0, kx1=1
kys[] linlin kxs[], ky0, ky1, kx0=0, kx1=1
iys[] linlin ixs[], iy0, iy1, ix0=0, ix1=1

linlin can be also used to blend between two arrays:

kC[]  linlin kx, kA[], kB[], kx0=0, kx1=1

if kx==0.5, kC[] will hold the avg of kA[] and kB[] for each value
(assumes that kA[] and kB[] are arrays of the same size)

*/

; Map a value within the range 1-3 to the range 0-10.
instr 1
  kx line 1, p3, 3
  ky linlin kx, 0, 10, 1, 3
  printks "kx: %f   ky: %f \n", 1/kr, kx, ky
endin

; Map an array of values
instr 2
  kX[] fillarray 0, 0.5, 1, 1.5, 2
  trim kX, 4
  kY[] linlin kX, 0, 10, 0, 2
  printarray kY
  turnoff
endin

; Blend between two arrays
instr 3
  kA[] fillarray 0, 1, 2, 3, 4, 5
  kB[] fillarray 0, 2, 4, 6, 8, 10
  kx line 0, p3, 1
  kC[] linlin kx, kA, kB
  printarray kC, -1, "", "blend"
endin

</CsInstruments>
<CsScore>
; i 1 0   0.2
i 2 0.5 0.2
; i 3 1 0.5

</CsScore>
</CsoundSynthesizer>
