# Explicit types and user-defined opcodes

[Csound 7 release notes](../Version_7.00.md)

A variable name can now describe its purpose without a type prefix.
For example, `frequency:i` declares an initialization-rate number named
`frequency`. The type follows the colon. Csound still uses the type to
select opcodes and determine when calculations run.

The familiar types keep their meanings: `i` for initialization, `k` for
control rate, `a` for audio rate, and `S` for strings. Classic declarations
remain valid. You can add explicit types to an existing instrument without
renaming every variable.

## Declare the type where you create the variable

This orchestra defines a global amplitude and a named instrument:

```csound
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

amplitude@global:i = 0.1

instr Tone
  frequency:i = p4
  envelope:k = linen(amplitude, 0.01, p3, 0.05)
  signal:a = oscili(envelope, frequency)
  out signal
endin

schedule(Tone, 0, 0.25, 440)
```

`@global` marks the declaration as global. Later uses need only the name.
The local declarations belong to each instance of `Tone`.

Type annotations do not make all names available. Keywords and opcode names
still have special meanings. Choose names that describe your data and do
not conflict with those names.

## Put UDO inputs in the declaration

A user-defined opcode (UDO) groups Csound statements into a reusable operation.
The new form declares each input by name and type. Its output type follows
the closing parenthesis.

```csound
opcode RaisePitch(frequency:i):i
  result:i = frequency * 2
  xout result
endop

instr Example
  frequency:i = RaisePitch(220)
  print frequency
endin

schedule(Example, 0, 0)
```

This example prints `440`. Use parentheses around multiple output types,
as in `(a,a)`. Use `void` for no output. The classic declaration and `xin`
remain available for existing UDOs.

## Account for input changes

The new form uses references at the caller's rates. An assignment to an
input can change the caller's variable:

```csound
opcode RaiseInPlace(frequency:i):void
  frequency *= 2
endop

instr Example
  frequency:i = 220
  RaiseInPlace(frequency)
  print frequency
endin

schedule(Example, 0, 0)
```

Here, `frequency` becomes `440` in the instrument. Use a separate local
variable inside the UDO when you need to preserve the input.

This behavior differs from classic UDO input copies. UDOs that contain
`setksmps`, `oversample`, or `undersample` also use copies.
This applies even when the setting leaves the rate unchanged.
Review input assignments when you change declaration forms.

## Read arrays with a loop

The new `for` loop can provide both an element and its index:

```csound
instr ListNotes
  notes:i[] = [60, 64, 67]
  for note:i, index:i in notes do
    prints "note[%g] = %g\n", index, note
  od
endin

schedule(ListNotes, 0, 0)
```

The loop variable's type determines the loop rate. Without an explicit or
existing type, it follows the array element type. This loop runs at initialization.
The optional index starts at zero. Existing array operations remain available.

See the [UDO reference](https://csound.com/manual/opcodes/opcode/) for call
rules. The repository also has examples of
[input changes](../../tests/commandline/udo/pass_by_ref.csd) and
[array loops](../../tests/commandline/test_for_in2.csd).
