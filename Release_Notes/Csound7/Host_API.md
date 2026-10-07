# Host applications and unit generators

[Csound 7 release notes](../Version_7.00.md)

Csound 7 gives host applications an explicit compile, start, and performance
sequence. It also adds a unit generator (UGen) interface for individual
opcodes. A host can use the orchestra engine, the UGen interface, or both.

## Update the engine sequence

Compilation no longer starts performance. Call `csoundStart` after compilation,
then call `csoundPerformKsmps` for each block. This complete C example performs
a CSD file supplied on the command line:

```c
#include <csound.h>

int main(int argc, char **argv)
{
    if (argc != 2)
        return 1;

    CSOUND *csound = csoundCreate(NULL, NULL);
    if (csound == NULL)
        return 1;

    int32_t result = csoundCompileCSD(csound, argv[1], 0, 0);
    if (result == 0)
        result = csoundStart(csound);
    if (result == 0) {
        do {
            result = csoundPerformKsmps(csound);
        } while (result == 0);
    }

    csoundDestroy(csound);
    return result < 0 ? 1 : 0;
}
```

The last two compile arguments select file input and synchronous compilation.
For CSD text, set `mode` to `1`. For asynchronous compilation, set `async`
to `1` where the engine state permits it.

For host audio processing, call `csoundSetHostAudioIO` before the engine
starts. Write input frames through `csoundGetSpin` and read output through
`csoundGetSpout`. Each performance call processes `ksmps` frames.
The output pointer is read-only.

The performance thread interface now belongs to the main Csound library.
Use `csPerfThread.h` from C or `csPerfThread.hpp` from C++ when Csound must
perform on its own thread.

## Send events through the combined calls

`csoundEvent` accepts numeric p-fields and an event type constant.
It requires a started engine. `csoundEventString` accepts score text.
Before start, a synchronous text call adds events to the score.
After start, it sends events to the real-time queue.

Both functions return `void`. The `async` argument selects synchronous or
asynchronous operation. Use the current
[public header](../../include/csound.h) for signatures and state requirements.

## Run one opcode from C

The UGen API creates an opcode instance with typed inputs and outputs.
It returns data in memory. The host decides how to use that data.

This complete example runs one block of an oscillator and prints its second
sample:

```c
#include <csound.h>
#include <ugen.h>
#include <stdio.h>

int main(void)
{
    int status = 1;
    CSOUND *csound = csoundCreate(NULL, NULL);
    UGEN_FACTORY *factory = NULL;
    UGEN *oscillator = NULL;
    if (csound == NULL)
        return 1;
    if (csoundSetOption(csound, "-n -d -m0") != 0)
        goto done;
    if (csoundCompileOrc(csound,
            "sr = 48000\nksmps = 32\n0dbfs = 1\n", 0) != 0)
        goto done;
    if (csoundStart(csound) != 0)
        goto done;

    factory = csoundUgenFactoryNew(csound);
    if (factory == NULL)
        goto done;
    oscillator = csoundUgenNew(factory, "oscils", "a", "iiio");
    if (oscillator == NULL)
        goto done;
    csoundUgenSetValue(oscillator, 0, 0.1);
    csoundUgenSetValue(oscillator, 1, 440);
    csoundUgenSetValue(oscillator, 2, 0);
    csoundUgenSetValue(oscillator, 3, 0);
    if (csoundUgenInit(oscillator) != 0 ||
        csoundUgenPerform(oscillator) != 0)
        goto done;

    UGEN_VAR *output = csoundUgenGetOutVar(oscillator, 0);
    const cs_float *samples = csoundUgenVarGetData(output);
    printf("sample[1] = %g\n", (double)samples[1]);
    status = 0;

done:
    if (oscillator != NULL)
        csoundUgenDelete(oscillator);
    if (factory != NULL)
        csoundUgenFactoryDelete(factory);
    csoundDestroy(csound);
    return status;
}
```

Opcode type strings must match a registered opcode entry exactly.
Use `csoundUgenListOpcodes` or `csoundUgenFindOpcode` to inspect available
entries. `oscils` has initialization-rate inputs. Use a suitable control-rate
opcode when parameters must change during performance.

## Connect and own the processing objects

`csoundUgenSetInputVar` connects a typed variable to an opcode input without
a data copy. A UGen graph runs its members in insertion order. Add producers
before the opcodes that consume their outputs.

Handles from `csoundUgenGetInVar` and `csoundUgenGetOutVar` belong to the UGen.
Keep each source alive while another UGen uses its data. The host must
release standalone variables that it creates itself.

`csoundUgenGraphDelete` deletes the graph alone.
`csoundUgenGraphDeleteAll` also deletes its UGens. Opcodes that need instrument
state can use a UGen context. Delete those UGens before their context.
Release graphs, standalone variables, UGens, and contexts before the factory.
Destroy the Csound instance last.

The [UGen header](../../include/ugen.h) defines these ownership rules.
The [Python examples](../../examples/ugen/README.md) show the same interface
through `UgenFactory`, `UgenVar`, and `UgenGraph`.
