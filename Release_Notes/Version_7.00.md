# Csound 7.0

## Deprecated opcodes

`--error-deprecated` now rejects every opcode marked deprecated, including
older registrations that previously only warned. This is intentional: callers
who request strict checking should get the same result regardless of how an
opcode records its deprecation. Scores that use these opcodes may now fail
under this option. Without it, deprecated opcodes remain available for older
scores.

Message-level bit 1024 (`CS_NOQQ`, as in `-m1024`) now suppresses ordinary
deprecation warnings for both registration styles. It does not override an
explicit `--error-deprecated` request.
