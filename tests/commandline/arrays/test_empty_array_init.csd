<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

struct Box value:i

instr 1
  nums:i[] init 0
  names:S[] init 0
  boxes:Box[] init 0

  if (lenarray(nums) != 0 || lenarray(names) != 0 || lenarray(boxes) != 0) then
    prints "Empty array lengths failed: nums=%d names=%d boxes=%d\n", \
      lenarray(nums), lenarray(names), lenarray(boxes)
    exitnow(1)
  endif

  nums init 1
  names init 1
  boxes init 1

  box:Box init 64
  nums[0] = 42
  names[0] = "flute"
  boxes[0] = box

  if (lenarray(nums) != 1 || nums[0] != 42) then
    prints "Empty numeric array resize failed: len=%d value=%d\n", \
      lenarray(nums), nums[0]
    exitnow(1)
  endif

  if (lenarray(names) != 1 || strcmp(names[0], "flute") != 0) then
    prints "Empty string array resize failed: len=%d value='%s'\n", \
      lenarray(names), names[0]
    exitnow(1)
  endif

  if (lenarray(boxes) != 1 || boxes[0].value != 64) then
    prints "Empty struct array resize failed: len=%d value=%d\n", \
      lenarray(boxes), boxes[0].value
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
