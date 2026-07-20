<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

struct ScoreNode label:S, children:ScoreNode[]

opcode CountScoreBranches(depth:i):i
  children:ScoreNode[] init 0
  temporary:ScoreNode init "temporary", children

  if (depth == 0) then
    result:i = 1
  else
    left:i = CountScoreBranches(depth - 1)
    right:i = CountScoreBranches(depth - 1)
    result = left + right
  endif
  result += lenarray(temporary.children)
  xout result
endop

struct Annotation text:S, nested:Annotation[], catalogs:Catalog[]
struct Catalog keys:S[], values:Annotation[], parents:Catalog[], length:i, id:i

emptyAnnotations@global:Annotation[] init 0
emptyCatalogs@global:Catalog[] init 0
emptyKeys@global:S[] init 0
catalogRegistry@global:Catalog[] init 1

opcode MakeCatalog(id:i):Catalog
  result:Catalog init emptyKeys, emptyAnnotations, emptyCatalogs, 0, id
  xout result
endop

opcode ResolveCatalog(input:Catalog):Catalog
  if (input.id >= 0) then
    stored:Catalog init catalogRegistry[input.id]
    if (stored.id == input.id) then
      result:Catalog init stored
    else
      result:Catalog init input
    endif
  else
    result:Catalog init input
  endif
  xout result
endop

opcode AppendCatalog(input:Catalog, key:S):Catalog
  result:Catalog init ResolveCatalog(input)
  keys:S[] init result.length + 1

  if (result.length > 0) then
    for index in [0 ... result.length - 1] do
      keys[index] init result.keys[index]
    od
  endif
  keys[result.length] init key
  result.keys init keys
  result.length += 1
  xout result
endop

struct Section players:i

sectionChannelDimensions@global:i[] fillarray 1
chn_array "section-array", 3, "Section", sectionChannelDimensions

opcode ForwardSections(input:Section[]):Section[]
  xout input
endop

opcode ForwardCatalogs(input:Catalog[]):Catalog[]
  xout input
endop

opcode IncrementCycle(source:k):k
  result:k = 0
  result = source + 1
  xout result
endop

instr InitChecks
  ; Large enough to expose retained recursive frames while keeping the
  ; recycled case bounded by recursion depth.
  branches:i = CountScoreBranches(18)
  if (branches != 262144) then
    prints "recursive init failed: expected=262144 got=%d\n", branches
    exitnow(1)
  endif

  root:Catalog = MakeCatalog(0)
  catalogRegistry[0] = root
  first:Catalog = AppendCatalog(root, "violin")
  catalogRegistry[0] = first
  second:Catalog = AppendCatalog(first, "cello")

  if (second.length != 2 || lenarray(second.keys) != 2 || \
      strcmp(second.keys[0], "violin") != 0 || \
      strcmp(second.keys[1], "cello") != 0) then
    prints "returned recursive struct did not survive frame reuse\n"
    exitnow(1)
  endif

  original:Section[] init 1
  woodwind:Section init 1
  original[0] = woodwind
  copied:Section[] = ForwardSections(original)
  brass:Section init 2
  copied[0] = brass
  originalValue:Section = original[0]
  copiedValue:Section = copied[0]

  if (originalValue.players != 1 || copiedValue.players != 2) then
    prints "struct array copy-on-write failed: original=%d copied=%d\n", \
      originalValue.players, copiedValue.players
    exitnow(1)
  endif

  memberCopy:Section[] = ForwardSections(original)
  memberCopy[0].players = 3
  originalAfterMember:Section = original[0]
  copiedAfterMember:Section = memberCopy[0]

  if (originalAfterMember.players != 1 || copiedAfterMember.players != 3) then
    prints "struct member copy-on-write failed: original=%d copied=%d\n", \
      originalAfterMember.players, copiedAfterMember.players
    exitnow(1)
  endif

  resized:Section[] = ForwardSections(original)
  resized fillarray brass, brass
  originalAfterResize:Section = original[0]
  resizedFirst:Section = resized[0]
  resizedSecond:Section = resized[1]

  if (lenarray(original) != 1 || originalAfterResize.players != 1 || \
      lenarray(resized) != 2 || resizedFirst.players != 2 || \
      resizedSecond.players != 2) then
    prints "struct array resize copy-on-write failed\n"
    exitnow(1)
  endif

  survivor:Section[] = ForwardSections(original)
  original[0] = brass
  survivorValue:Section = survivor[0]
  changedOwnerValue:Section = original[0]
  if (survivorValue.players != 1 || changedOwnerValue.players != 2) then
    prints "struct array owner copy-on-write failed: old=%d new=%d\n", \
      survivorValue.players, changedOwnerValue.players
    exitnow(1)
  endif

  nestedOriginal:Catalog[] init 1
  nestedOriginal[0] = second
  nestedCopy:Catalog[] = ForwardCatalogs(nestedOriginal)
  replacement:Catalog = MakeCatalog(7)
  nestedCopy[0] = replacement
  preserved:Catalog = nestedOriginal[0]
  replaced:Catalog = nestedCopy[0]

  if (preserved.length != 2 || lenarray(preserved.keys) != 2 || \
      strcmp(preserved.keys[1], "cello") != 0 || replaced.id != 7) then
    prints "nested struct array copy-on-write failed\n"
    exitnow(1)
  endif

  emptySections:Section[] init 0
  emptyCopy:Section[] = ForwardSections(emptySections)
  emptyCopy fillarray brass
  if (lenarray(emptySections) != 0 || lenarray(emptyCopy) != 1) then
    prints "empty struct array copy-on-write failed: old=%d new=%d\n", \
      lenarray(emptySections), lenarray(emptyCopy)
    exitnow(1)
  endif

  matrix:Section[][] init 2, 2
  matrix[1][0] = brass
  matrixValue:Section = matrix[1][0]
  if (matrixValue.players != 2) then
    prints "multidimensional struct array indexing failed\n"
    exitnow(1)
  endif
endin

instr PerformanceCheck
  value:k init 5
  value = IncrementCycle(value)
  if (timeinstk() == 1 && value != 6) then
    printks "k-rate UDO cycle 1 failed: expected=6 got=%g\n", 0, value
    exitnowk(-1)
  endif
  if (timeinstk() == 2 && value != 7) then
    printks "k-rate UDO cycle 2 failed: expected=7 got=%g\n", 0, value
    exitnowk(-1)
  endif
endin

instr LateDurationCheck
  value:k init 0
  value = IncrementCycle(value)
  ; The final duration is not known until the complete init pass has run.
  p3 = 0.02
  if (timeinstk() == 2 && value != 2) then
    printks "late p3 UDO cycle failed: expected=2 got=%g\n", 0, value
    exitnowk(-1)
  endif
endin

instr ArrayChannelCheck
  source:Section[] init 1
  original:Section init 4
  changed:Section init 9
  source[0] = original
  chnset source, "section-array"

  ; The channel owns an independent top-level copy. A later source write must
  ; not alter the value already sent through the channel.
  source[0] = changed
  received:Section[] init 1
  received chnget "section-array"
  channelValue:Section = received[0]
  if (channelValue.players != 4) then
    prints "struct array channel copy failed: expected=4 got=%d\n", \
      channelValue.players
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
; A normal score duration must not retain frames for init-only UDO calls.
i "InitChecks" 0 0.02
i "PerformanceCheck" 0 0.02
i "LateDurationCheck" 0 0
i "ArrayChannelCheck" 0 0
</CsScore>
</CsoundSynthesizer>
