# Module API changes

[Csound 7 release notes](../Version_7.00.md)

The Module API serves opcode plugins, function-table generators (GENs),
audio and MIDI backends, and utilities. Modules include `csdl.h`, which
provides the structures, inline functions, macros, and `CSOUND` function
table from `csoundCore.h`.

A module calls engine functions through the supplied `CSOUND *` pointer.
For example, `csound->AppendOpcode` registers an opcode.
The [host API](Host_API.md) serves applications that create and control the
engine. Its exported functions have separate signatures and migration rules.

Rebuild all native modules with the Csound 7 headers. The `CSOUND` function
table and the `OENTRY`, `OPDS`, and `INSDS` layouts have changed.
Csound 6 binary modules cannot use these new layouts.

## Register initialization, performance, and cleanup

`OENTRY` now has `init`, `perf`, and `deinit` callbacks.
The old `thread` field and separate `kopadr` and `aopadr` slots are gone.
Its `dsblksiz` field now uses `size_t`.

Use `perf` for control-rate or audio-rate processing. Register separate
opcode entries when argument types need different routines.
Move an old audio callback into `perf`. The third callback slot now runs
at deinitialization.

Use `deinit` for opcode cleanup in place of `RegisterDeinitCallback`.
`OPDS.iopadr` and `OPDS.opadr` become `init` and `perf`.
The engine maintains a deinitialization chain in `OPDS` and `INSDS`.
Module-wide cleanup still belongs in `csoundModuleDestroy`.

The C++ `csnd::plugin<T>` wrapper still accepts its own `thread` selector.
It uses that value to select callbacks for the new registration format.

This complete C plugin applies gain to an audio input:

```c
#include <csdl.h>
#include <string.h>

typedef struct {
    OPDS h;
    cs_float *out;
    cs_float *in;
    cs_float *gain;
} RELEASE_GAIN;

static int32_t gain_perf(CSOUND *csound, void *data)
{
    RELEASE_GAIN *p = data;
    uint32_t count = GetLocalKsmps(&p->h);
    uint32_t offset = GetKsmpsOffset(&p->h);
    uint32_t end = count - GetEarlySmps(&p->h);
    (void)csound;

    memset(p->out, 0, offset * sizeof(cs_float));
    memset(p->out + end, 0, (count - end) * sizeof(cs_float));
    for (uint32_t n = offset; n < end; ++n)
        p->out[n] = p->in[n] * *p->gain;
    return OK;
}

static OENTRY localops[] = {
    {
        .opname = "release_gain",
        .dsblksiz = sizeof(RELEASE_GAIN),
        .outypes = "a",
        .intypes = "ak",
        .perf = gain_perf
    }
};

LINKAGE
```

This opcode needs only a performance callback. Other entries can set
`.init` and `.deinit` as required. `LINKAGE` exports the opcode list and
module information. The plugin uses no direct Csound library calls.

## Use the opcode's local context

A UDO can have a sample rate or block size that differs from the engine.
Use the owning opcode's context for signal processing:

| Required value | Inline function | Macro |
| --- | --- | --- |
| Sample rate | `GetLocalSr(&p->h)` | `CS_ESR` |
| Control rate | `GetLocalKr(&p->h)` | `CS_EKR` |
| Block size | `GetLocalKsmps(&p->h)` | `CS_KSMPS` |
| Control-cycle count | `GetLocalKcounter(&p->h)` | `CS_KCNT` |

These macros expect an opcode pointer named `p` with an `OPDS h` member.
In Csound 7, `CS_ESR` reads the local sample rate from `INSDS`.
Use `GetKsmpsOffset` and `GetEarlySmps` to honor sample-accurate note boundaries,
as the example does.

For engine-wide values, the function table provides `GetEngineSr`,
`GetEngineKr`, and `GetEngineKcounter`. Use these when the operation needs
the engine's rates or time base.

Several old function-table getters are now inline functions.
Replace `csound->GetInputArgCnt(p)` with `GetInputArgCnt(&p->h)`.
The same change applies to argument names, output counts, MIDI information,
and release-state queries. Call `GetTypeForArg(argument)` directly and
use its `const CS_TYPE *` result.

`GetPFields(&p->h)` now returns `CS_VAR_MEM *`.
Read a numeric p-field through its `value` member, such as
`GetPFields(&p->h)[3].value`. Do not treat this result as a sample array.
Only p1, p2, and p3 are guaranteed to exist.

## Update calls through the function table

The public part of `CSOUND` remains the module's engine interface.
These changes affect several module types:

| Csound 6 use | Csound 7 use |
| --- | --- |
| `AppendOpcode` with a `thread` argument | Supply `init`, `perf`, and `deinit` callbacks without that argument. |
| `GetChannelPtr` with a sample-pointer address | Pass `void **` for the channel data address. |
| `RealFFT` and `InverseRealFFT` with a buffer and size | Create a setup with `RealFFTSetup(csound, size, FFT_FWD)` or `FFT_INV`. Pass it to `RealFFT(csound, setup, buffer)`. |
| `FileOpen2` | Use `FileOpen`. |
| `FileClose(csound, handle)` | Add `CSFILE_CLOSE_SYNC` or `CSFILE_CLOSE_DEFER` to select the close behavior. |
| `AddUtility` and related utility functions directly in `CSOUND` | Obtain the `CSOUND_UTIL` table through `csound->GetUtility(csound)`. |

Create FFT setups during initialization and reuse them during performance.
For file operations, use the current argument types in
[`csoundCore.h`](../../include/csoundCore.h).
The `SndfileOpen`, `SndfileRead`, and related entries provide the engine's
sound-file interface.

## GENs, backends, and utilities

GEN modules continue to export an `NGFENS` list through `FLINKAGE`.
Rebuild them for the current `FGDATA`, `FUNC`, and numeric types.
GEN callbacks receive `FGDATA *` and `FUNC *` arguments and return `int32_t`.
Use `FGDATA.csound` to access the module function table.

`FTFind` replaces the separate `FTFindP`, `FTnp2Find`, and `FTnp2Finde` lookup
entries. It accepts tables whose sizes are not powers of two.
If your code requires a power-of-two size, verify that size after lookup.
Use `FtError` in place of the old `ftError` member for GEN errors.

`EVTBLK.p` now points to separate p-field storage.
If a module constructs an event, it must supply that storage.
Allocating an `EVTBLK` alone no longer provides a p-field array.

Audio backends still register through module callbacks such as
`csound->SetPlayopenCallback`, `csound->SetRtplayCallback`, and
`csound->SetRecopenCallback`. MIDI backends use the `SetExternalMidi*Callback`
entries. The removal of audio registration calls from the host API does
not remove these module functions.

Utilities now obtain registration and sound-input functions through
`csound->GetUtility(csound)`. For example, register a utility with
`csound->GetUtility(csound)->AddUtility(csound, name, callback)`.
The table also provides descriptions, sample-rate setup, and input functions.
See the [scale utility](../../util/scale.c) for a complete use of this interface.

## Types, storage, and module information

Use `cs_float` for samples and opcode arguments. `MYFLT` remains a deprecated
alias. Match both `cs_float` and `cs_double` precision to the loaded engine.
The [numeric type guide](../../docs/numeric-types.md) gives the build options.

`LINKAGE` and `FLINKAGE` provide the current `csoundModuleInfo` value.
A hand-written `csoundModuleInfo` must return `CSOUND_MODULE_INFO`.
This value includes version and precision information for the loader.

`CreateInstanceVariable` and `QueryInstanceVariable` provide named storage
for opcodes in the same instrument instance. Use them instead of private
`INSDS` fields for shared instance state. The engine owns this storage.
The [instance variable guide](../../docs/instance-variables.md) defines its
lifetime and access rules.

Plugins can register struct types through `csound->RegisterStruct`.
Custom `CS_TYPE` constructors and copy callbacks now receive the canonical
type and an `INSDS` context. Update callback implementations to the signatures
in [`csound_type_system.h`](../../include/csound_type_system.h).

Managed arrays and structs need their type-aware storage operations.
Use the current array write helpers and type copy functions before changes
that could affect shared data. See the
[plugin struct guide](../../docs/plugin-structs.md) and
[array interface](../../include/arrays.h).
