# Deprecated opcode policy and metadata

Deprecated opcodes remain available for older orchestras. Use the replacements
listed in [the catalog](deprecated-opcodes.md) for new code. Replacement names
are guidance, not a promise of identical arguments, data formats, or sound.
When no direct replacement is known, the catalog says so rather than guessing.
In particular, FFT-based PVS opcodes do not directly replace the constant-Q
`spectrum` / `w`-signal chain.

## Before changing a deprecated opcode

Read the `CSOUND_DEPRECATED_OPCODE` descriptor beside the registration, even
when investigating an apparent bug. Frozen functions also carry an explicit
`CSOUND_PRESERVE_LEGACY_BEHAVIOR` marker immediately above their definitions.

- **ALIAS:** the name has changed. Maintain its shared implementation through
  the supported name; do not fork the alias into a second implementation.
- **LEGACY:** keep the old opcode available for compatibility. Put new features
  and routine changes to historical output in a supported replacement. If a
  correction under the old name is needed, discuss its compatibility impact
  with the maintainers first.
- **FROZEN:** known incorrect behavior is deliberately retained. Do not repair
  or modernize that behavior under the old name without an explicit maintainer
  decision. The entry records the reason and, where available, the discussion.

These policies do not mean that every deprecated opcode is buggy. They do not
preclude build fixes or documenting limitations. Report crashes and memory
safety problems to the maintainers; a compatibility marker is not a reason to
ignore them. Changes to shared helpers still need to account for legacy callers.

## Declare the policy where the opcode lives

Each entry uses the same four-argument descriptor:

```c
CSOUND_DEPRECATED_OPCODE("hrtfer", "hrtfstat", FROZEN, "Legacy HRTFcompact behavior is frozen; review migration.")
```

The first two arguments are the deprecated name and recommended replacement.
Use `NULL` for a replacement only when none is documented. The policy describes
maintenance intent; the final string explains migration limits or known behavior.
Place the descriptor immediately above the first registration of that public
name in each source file. Keep it on one line for the source reader. Omit internal
overload suffixes such as `.a` or `.k`. Separate spellings have separate entries;
descriptors for the same name in different files must agree.

For each frozen opcode, mark its registered initialization, processing, and
cleanup functions too:

```c
/* DO NOT FIX historical behavior here without an explicit maintainer decision. */
CSOUND_PRESERVE_LEGACY_BEHAVIOR("hrtfer")
static int32_t hrtferxk(CSOUND *csound, HRTFER *p)
{
    /* Existing implementation. */
}
```

Both macros expand to nothing. They make the policy visible while reading or
editing an implementation, with no runtime cost or plugin ABI change. The source
descriptors supply the generated `H/opcode_deprecations.def`, which feeds the
compiler's metadata table, and the Markdown/JSON inventory. Edit the source
descriptors, not the generated files.

Runtime diagnostics only apply to a resolved overload that is actually
marked deprecated; a user opcode or another overload with the same name is not
deprecated merely because that name appears in the catalog.

Keep the registration's `_QQ` flag or `OENTRY.deprecated` value (1 for deprecated,
2 for renamed). Add the local descriptor whenever adding a deprecation. Existing
plugins without catalog entries still receive a generic warning. The public
opcode list reports `_QQ` entries as deprecated too.

Compilation names the recommended replacement when known. `--error-deprecated`
turns either registration style into an error; message-level bit 1024 (`CS_NOQQ`)
suppresses ordinary deprecation warnings, but does not bypass that explicit error
option. No warning is added to the audio processing loop.

Regenerate and check the reference with:

```sh
python3 scripts/opcode_deprecations.py
python3 scripts/opcode_deprecations.py --check
python3 scripts/opcode_deprecations.py --json
```

The check covers static C/C++ opcode registrations throughout this repository,
including optional plugins, and verifies replacement names and generated output.
It also rejects missing local descriptors, conflicting descriptors for the same
name, and frozen callbacks without their preserve-behavior markers. A marker in
a comment does not count. These checks catch missing declarations; they cannot
prove that a change preserves behavior, so reviewers still need to read the policy.
External plugins can continue using their existing deprecation flags; their
replacement metadata is outside this built-in catalog.

## Inventory sources

The initial inventory combines source `_QQ` flags and `deprecated` fields with
`csound/manual` develop commit `53b2f99dc0`, its `docs/deprecated.md`, and the linked
opcode pages. It includes renamed aliases as well as legacy implementations.
The manual-only markers for `ptablew`, `sclag`, `sclagud`, `scphasor`, and `sctrig`
are now reflected in their registrations. `tb` is a documentation family name;
its actual `tb0`–`tb15` and setup opcodes have individual entries.
