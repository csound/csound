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

  nums[0] = 42
  names[0] = "flute"

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
endin

</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
