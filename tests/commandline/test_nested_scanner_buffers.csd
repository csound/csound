<CsTest>
description = "nested orchestra and score macros resume their parent buffers"

[expect]
exit = 0
output = ["nested scanner buffers"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
#define VOICE #instr 1
  prints "nested scanner buffers\n"
endin#
#define ORCHESTRA #$VOICE.#
$ORCHESTRA.
</CsInstruments>
<CsScore>
#define NOTE #i 1 0 0.001#
#define SCORE #$NOTE.#
$SCORE.
e
</CsScore>
</CsoundSynthesizer>
