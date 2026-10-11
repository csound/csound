# Opcode and instrument objects

[Csound 7 release notes](../Version_7.00.md)

Csound 7 can store opcode and instrument references in variables. This lets
a program choose a definition, create instances, and keep those instances
in arrays. Each opcode instance has its own processing state.

## Separate definitions from instances

The four types serve different purposes:

| Type | Value |
| --- | --- |
| `OpcodeDef` | An opcode definition, such as `oscili`. |
| `Opcode` | An instance of an opcode, with its own state. |
| `InstrDef` | An instrument definition. |
| `Instr` | An instance of an instrument. |

A definition identifies the code to run. An instance holds the state for
one use of that code. Two oscillator instances can therefore keep separate
phases while they use the same opcode definition.

## Create and run an opcode instance

This orchestra creates an oscillator object for each note:

```csound
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr Tone
  oscillator:Opcode = create(oscili)
  signal:a = run(oscillator, 0.1, p4)
  out signal
  delete oscillator
endin

schedule(Tone, 0, 0.25, 440)
```

`run` supplies the opcode's initialization and performance calls.
Its arguments and outputs must match the selected opcode. To separate these
stages, use `init` and `perf`.

For opcode objects, `delete` registers cleanup for instrument deinitialization.
It releases the object after performance ends.

## Keep separate state inside a loop

An array of objects provides separate instances for repeated processing.
This example initializes two oscillators, then performs both on each control
cycle:

```csound
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr Pair
  frequencies:i[] = [440, 660]
  oscillators:Opcode[] = create(oscili, 2)
  signals:a[] init 2
  index:i = 0
  while index < 2 do
    signals[index] init oscillators[index], 0.05, frequencies[index]
    index += 1
  od
  current:k = 0
  while current < 2 do
    signals[current] perf oscillators[current], 0.05, frequencies[current]
    current += 1
  od
  out signals[0] + signals[1]
  delete oscillators
endin

schedule(Pair, 0, 0.25)
```

The object array preserves each oscillator's phase between control cycles.
The loop selects an instance through its index.

## Schedule an instrument through its definition

A named instrument provides an `InstrDef` constant. You can pass it directly
to `schedule`, as the examples above do. Inside an instrument, `this_instr`
refers to its definition.

`create` can also compile an instrument body from a string:

```csound
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

Tone:InstrDef = create({{
  signal:a = oscili(p4, p5)
  out signal
}})
voice:Instr = schedule(Tone, 0, 0.25, 0.1, 440)
```

Here, the engine schedules `voice` and performs it. The manual instance
interface also supports `create`, `init`, `perf`, and `delete`.
A manually initialized instance needs explicit performance calls.

Compilation and instance creation can allocate memory. Prepare reusable
objects before repeated performance work where possible. See the
[opcode object examples](../../tests/commandline/test_opcode_type.csd) and
[instrument instance example](../../tests/commandline/test_instance_type.csd).
