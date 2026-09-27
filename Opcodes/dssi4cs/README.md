# DSSI and LADSPA opcodes

This module hosts native DSSI synths and LADSPA effects on Linux.
Build with `-DBUILD_DSSI_OPCODES=ON` and install the ALSA headers
(`libasound2-dev` on Debian/Ubuntu). The module does not need the ALSA
sequencer service or link to libasound. Plugins must match the host OS,
CPU, and DSSI/LADSPA ABI.

Use `ninja -C build` after configuring CMake. With `BUILD_TESTS=ON`, the
`dssi_tests` target builds small native plugins and runs the host through
Csound. Run `ctest --test-dir build -R '^DssiTests\.' --output-on-failure`.
The fixtures use the system DSSI header when available, to check
compatibility independently of the bundled host headers.

## Loading

```csound
ih dssiinit Sfilename, iindex [, iverbose]
dssilist
dssiinfo ih
```

`iindex` selects a plugin within a library. A successful load returns an
instance handle starting at zero. Each call creates a separate instance.
`iverbose` defaults to 1 and prints ports and MIDI mappings. `dssiinfo` prints
that information for an existing handle. `dssilist` lists both DSSI and LADSPA
libraries, including libraries that export only `dssi_descriptor`.

A name containing `/` is a path. For other names, search `DSSI_PATH`, then
`LADSPA_PATH`, then platform defaults. Each environment variable contains
colon-separated directories; empty entries are skipped. Lookup tries the
name as supplied, then `.so`.

Linux defaults include `/usr/local/lib`, `/usr/lib`, and `/usr/lib64`, each
with `dssi` and `ladspa` subdirectories. Set the environment paths for
multiarch directories such as `/usr/lib/aarch64-linux-gnu/dssi`.

Instances and libraries remain loaded until Csound reset or destruction.
Failed loads do not publish a handle. Cleanup calls the plugin's deactivate
and cleanup methods before releasing its buffers and library.

## Audio and activation

```csound
dssiactivate ih, kactive
a1 [, a2, ...] dssisynth ih [, ain1, ain2, ...]
a1 [, a2, ...] dssiaudio ih [, ain1, ain2, ...]
```

`kactive` must be 0 or 1. Activate before sending events or rendering.
`dssisynth` requires a DSSI synth. `dssiaudio` accepts a LADSPA effect or DSSI
plugin and chooses its synth callback when available. Audio ports follow
the descriptor order, with input and output ports counted separately.
Unused inputs receive silence. Csound's `0dbfs` scales audio to/from LADSPA
float buffers.

Give each instance one audio renderer. The renderer's `ksmps` may be smaller
than the `ksmps` used to create the instance. Partial first/last blocks produce
silence outside the instrument's active samples. Ending the renderer
instrument deactivates the instance and discards pending events. Keep a
renderer running if notes or release tails must outlive their source
instrument. Deactivation clears pending notes; resuming sends note-offs for
old sounding notes, releases sustain, and sends all-sound-off/all-notes-off
even when the plugin has no activation callbacks.

The host supports plugins that provide only `run_multiple_synths`. Every
call includes all active instances from the same library and LADSPA label.
Activate all grouped instances before the first renderer in that block.
Late activation reports an error rather than advancing a peer twice.
For groups with more than one instance, all renderers must have the same
`ksmps`, use full blocks, and the plugin must have no audio inputs. Send events
for every instance before the group's first renderer. Later renderers read
the outputs from that same group call.

## Notes and MIDI

```csound
dssinote ktrigger, ih, knote, kvelocity, kduration [, ichannel]
dssievent ktrigger, ih, knote, kvelocity
dssievent ktrigger, ih, kstatus, kchannel, kdata1, kdata2 [, koffset]
dssinrpn ktrigger, ih, kchannel, kparameter, kvalue [, koffset]
```

Each nonzero trigger sends an event in that control cycle; it is not an
edge detector. Use a one-cycle trigger for one note. Notes/velocities range
from 0 to 127. Channels range from 0 to 15 and default to 0 in `dssinote`.
The four-argument `dssievent` sends a note on channel 0; velocity zero sends
a true note-off. `dssinote` schedules a note-off after `kduration` seconds,
rounded to the nearest sample. Its pending note-off survives the source
instrument if a separate renderer remains active.

The full MIDI form accepts status bytes without channel bits:

| Status | Meaning | Data |
| --- | --- | --- |
| 128 / 0x80 | Note off | note, release velocity |
| 144 / 0x90 | Note on | note, velocity; zero means note off |
| 160 / 0xA0 | Key pressure | note, pressure |
| 176 / 0xB0 | Controller | controller, value |
| 192 / 0xC0 | Program change | program, 0 |
| 208 / 0xD0 | Channel pressure | pressure, 0 |
| 224 / 0xE0 | Pitch bend | low 7 bits, high 7 bits |

`koffset` defaults to zero and counts samples from the instrument's first
active sample in this block. It must fall within that block's active range.
Events sort by sample time; equal-time events retain insertion order.
Queue events before rendering their block. Sending an event after its
samples have rendered reports a performance error.

Bank select controllers 0/32 and MIDI program changes call `select_program`
instead of reaching the synth as MIDI. CC and NRPN mappings use the plugin's
control-port hints and do not also deliver the mapped event. Rendering splits
at mapped control/program changes so they take effect at the requested
sample. Use `dssinrpn` for assembled 14-bit NRPN messages: parameters and values
range from 0 to 16383. The host does not assemble NRPN from raw CC 98/99/6/38.

## Controls, programs, and configuration

```csound
dssictls ih, iport, kvalue, ktrigger
kvalue dssiget ih, iport
dssiprogram ih, kbank, kprogram, ktrigger
Sname, ibank, iprogram dssiprograminfo ih, iindex
dssiconfigure ih, Skey, Svalue
```

Port numbers are the descriptor's absolute port indices, including audio
ports. `dssictls` writes input control ports on each nonzero trigger, and
retains its existing sample-rate-hint scaling. `dssiget` reads an input or
output control port in the plugin's units. Output controls reflect the last
render. Input controls start with LADSPA defaults; selecting a program may
replace them. Put control writes after program selection when overriding a
preset. Audio ports and output controls cannot be written through `dssictls`.

`dssiprograminfo` enumerates by list index, not by bank/program number. It
copies the name because plugins may reuse their descriptor storage. An
index beyond the list returns an empty name and -1 for bank/program.
`dssiprogram` selects the given bank/program when triggered. Select a program
explicitly when the plugin needs one; plugins need not choose a default on
activation. Program selection does not imply a note-on.

`dssiconfigure` runs at initialization. Use it to load sample files, patches,
or other plugin settings before performance. It reports and frees plugin
error strings. `GLOBAL:` keys apply to all currently loaded instances of the
same plugin. Configuration invalidates program information; enumerate and
select again afterwards. Put configuration calls and sample paths in the
orchestra so reloading it restores them. This module does not save separate
plugin state files or launch plugin GUIs.

## Example

```csound
sr = 48000
ksmps = 64
nchnls = 2
0dbfs = 1

gih dssiinit "my_synth.so", 0, 1

instr 1
  dssiactivate gih, 1
  konce init 1
  ; If needed: dssiprogram gih, 0, 0, konce
  dssinote konce, gih, 60, 100, 0.5
  konce = 0
  aleft, aright dssisynth gih
  outs aleft, aright
endin
```

Run the instrument long enough for the note and its release tail. Request
one output for a mono plugin.

## Limits

- Native Linux only; no macOS or WASM backend or OSC plugin GUI host.
- One Csound performance thread (`-j` values greater than 1 are rejected).
- At most 256 instances per engine and 1024 pending events per instance.
  A timed note reserves both its on and off events before enqueueing either.
  Queue overflow reports an error.
- Up to nine audio inputs and nine outputs per opcode, as in the existing API.
- Grouped rendering has the restrictions described above.
- No SysEx, MIDI system messages, or raw NRPN assembly in this version.
- Control ports are scalar LADSPA controls. Mixing uses Csound; the optional
  additive DSSI/LADSPA callbacks are not needed.

## ABI sources

The [DSSI](../../third_party/dssi/README.md) and
[LADSPA](../../third_party/ladspa/README.md) headers are unmodified copies
of upstream releases, with their licenses and source details. They retain
upstream's C ABI types (`unsigned long`, `int`, and `float`). See the
[DSSI specification](https://dssi.sourceforge.net/RFC.html) and
[LADSPA SDK](https://www.ladspa.org/ladspa_sdk/).
ALSA declarations come from the installed system headers.

Run `third_party/update.py` from the repository root to update both vendored
headers, or pass `--pinned` to reproduce the recorded releases and check their
archive hashes. Each dependency also has an `update.py` that accepts a version.
