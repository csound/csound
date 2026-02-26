#!/usr/bin/env python3
"""
UGen API example using ctcsound.

Demonstrates creating and running Csound opcodes as individual unit
generators outside of a traditional Csound orchestra/score, using the
UGen API added in Csound 7.

Based on the design from:
  "Extending Aura with Csound Opcodes"
  Steven Yi, Victor Lazzarini, Roger Dannenberg, John ffitch
  ICMC/SMC 2014

Requirements:
  - Csound 7 built with the UGen API
  - ctcsound.py on the Python path
"""

import ctcsound
import ctypes as ct
import sys
import math
import time


def example_single_ugen():
    """Example 1: Create and run a single oscils UGen.

    Uses oscils (simple sine oscillator) to generate one k-cycle of
    audio and prints the first few samples.
    """
    print("=" * 60)
    print("Example 1: Single oscils UGen")
    print("=" * 60)

    # Create and start Csound
    cs = ctcsound.Csound()
    # Minimal orchestra to set sr / ksmps
    cs.compile_orc("sr = 44100\nksmps = 64\n0dbfs = 1\ninstr 1\nendin")
    cs.start()

    # Create UGen factory
    factory = ctcsound.UgenFactory(cs)

    # Create an oscils UGen: output "a" (audio), inputs "iiio" (amp, freq, phase, [unused])
    osc = factory.new_ugen("oscils", "a", "iiio")
    if osc is None:
        print("ERROR: could not create oscils UGen")
        return

    print(f"  oscils: {osc.in_count} inputs, {osc.out_count} outputs")

    # Set input values: amplitude=0.5, frequency=440, phase=0
    osc.set_input_value(0, 0.5)   # amplitude
    osc.set_input_value(1, 440.0) # frequency
    osc.set_input_value(2, 0.0)   # phase

    # Initialize and perform one k-cycle
    osc.init()
    osc.perform()

    # Read the audio output buffer
    ksmps = cs.ksmps()
    samples = osc.get_output_buffer(0, ksmps)
    if samples:
        print(f"  Generated {len(samples)} samples (ksmps={ksmps})")
        print(f"  First 8 samples: {[f'{s:.6f}' for s in samples[:8]]}")

    # Clean up
    osc.delete()
    factory.delete()
    cs.reset()
    print()


def example_list_opcodes():
    """Example 2: List available opcodes.

    Shows how to query the opcode database and search for specific
    opcodes using the UGen API.
    """
    print("=" * 60)
    print("Example 2: List available opcodes")
    print("=" * 60)

    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)

    # List all opcodes
    opcodes = factory.list_opcodes()
    print(f"  Total opcodes available: {len(opcodes)}")

    # Show the first 10
    print("  First 10 opcodes:")
    for entry in opcodes[:10]:
        print(f"    {entry['opname']:20s} out={entry['outypes']!r:8s} "
              f"in={entry['intypes']!r}")

    # Search for a specific opcode
    if factory.find_opcode("oscils"):
        print("\n  'oscils' opcode found!")
    if factory.find_opcode("nonexistent_opcode"):
        print("  'nonexistent_opcode' found!")
    else:
        print("  'nonexistent_opcode' not found (expected)")

    factory.delete()
    cs.reset()
    print()


def example_argument_types():
    """Example 3: Query argument types and sizes.

    Demonstrates reading the type and size of each argument of an opcode.
    """
    print("=" * 60)
    print("Example 3: Query argument types")
    print("=" * 60)

    TYPE_NAMES = {
        ctcsound.UGEN_ARG_TYPE_I: "i-rate",
        ctcsound.UGEN_ARG_TYPE_K: "k-rate",
        ctcsound.UGEN_ARG_TYPE_A: "a-rate",
        ctcsound.UGEN_ARG_TYPE_S: "string",
        ctcsound.UGEN_ARG_TYPE_F: "fsig",
        ctcsound.UGEN_ARG_TYPE_UNKNOWN: "unknown",
    }

    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)

    # oscils: output "a", inputs "iiio" (the 'o' is optional i-rate)
    osc = factory.new_ugen("oscils", "a", "iiio")
    if osc is None:
        print("ERROR: could not create oscils UGen")
        return

    print("  oscils argument info:")
    for i in range(osc.out_count):
        t = osc.get_out_type(i)
        sz = osc.get_out_arg_size(i)
        print(f"    output[{i}]: type={TYPE_NAMES.get(t, '?'):6s}  "
              f"size={sz} bytes")

    for i in range(osc.in_count):
        t = osc.get_in_type(i)
        sz = osc.get_in_arg_size(i)
        print(f"    input[{i}]:  type={TYPE_NAMES.get(t, '?'):6s}  "
              f"size={sz} bytes")

    osc.delete()
    factory.delete()
    cs.reset()
    print()


def example_ugen_graph():
    """Example 4: UGen graph – two oscillators.

    Creates two oscils UGENs, adds them to a graph, and performs
    one k-cycle through the graph so both are initialised and
    rendered together.

    osc1: 440 Hz, amp 0.5
    osc2: 660 Hz, amp 0.3
    """
    print("=" * 60)
    print("Example 4: UGen graph")
    print("=" * 60)

    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\n0dbfs = 1\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)
    graph = factory.new_graph()

    # Create two oscils UGENs
    osc1 = factory.new_ugen("oscils", "a", "iiio")  # main oscillator
    osc2 = factory.new_ugen("oscils", "a", "iiio")  # second oscillator

    if osc1 is None or osc2 is None:
        print("ERROR: could not create oscils UGENs")
        return

    # Configure osc1: amp=0.5, freq=440, phase=0
    osc1.set_input_value(0, 0.5)
    osc1.set_input_value(1, 440.0)
    osc1.set_input_value(2, 0.0)

    # Configure osc2: amp=0.3, freq=660, phase=0
    osc2.set_input_value(0, 0.3)
    osc2.set_input_value(1, 660.0)
    osc2.set_input_value(2, 0.0)

    # Add both to graph
    idx1 = graph.add(osc1)
    idx2 = graph.add(osc2)
    print(f"  Added osc1 at index {idx1}, osc2 at index {idx2}")

    # Initialize and perform one k-cycle via the graph
    graph.init()
    graph.perform()

    # Read output from each oscillator
    ksmps = cs.ksmps()
    samples1 = osc1.get_output_buffer(0, ksmps)
    samples2 = osc2.get_output_buffer(0, ksmps)

    if samples1:
        print(f"  osc1 (440 Hz): first 8 samples = "
              f"{[f'{s:.6f}' for s in samples1[:8]]}")
    if samples2:
        print(f"  osc2 (660 Hz): first 8 samples = "
              f"{[f'{s:.6f}' for s in samples2[:8]]}")

    # Clean up (delete_all frees graph + all contained UGENs)
    graph.delete_all()
    factory.delete()
    cs.reset()
    print()


def example_realtime_vibrato():
    """Example 5: Real-time sine with vibrato (pitch modulation).

    Plays a 440 Hz sine tone through the DAC with a slow vibrato
    applied to the frequency.  Uses oscili which accepts k-rate
    frequency — we simply update the freq input each k-cycle and
    call perform().  No re-initialization needed.

    The audio is written into Csound's input buffer (spin) and a
    trivial pass-through instrument forwards it to the DAC.
    """
    print("=" * 60)
    print("Example 5: Real-time sine with vibrato (oscili)")
    print("=" * 60)

    DURATION = 4.0      # seconds
    BASE_FREQ = 440.0   # Hz
    VIB_DEPTH = 40.0    # Hz deviation
    VIB_RATE = 5.0      # vibrato speed in Hz
    AMP = 0.25

    cs = ctcsound.Csound()
    cs.set_option("-odac")
    cs.set_option("-d")           # suppress displays
    cs.set_option("-m0")          # minimal messages
    # Orchestra: create a sine table and a pass-through instrument
    orc = ("sr = 44100\nksmps = 64\n0dbfs = 1\nnchnls = 1\n"
           "gifn ftgen 1, 0, 8192, 10, 1\n"
           "instr 1\n  asig in\n  out asig\nendin\n")
    cs.compile_orc(orc)
    cs.start()

    ksmps = cs.ksmps()
    sr = cs.sr()
    factory = ctcsound.UgenFactory(cs)

    # oscili: output "a", inputs "kkjo" (amp, freq, table, phase)
    osc = factory.new_ugen("oscili", "a", "kkjo")
    if osc is None:
        print("ERROR: could not create oscili UGen")
        factory.delete()
        cs.reset()
        return

    # Set initial parameters and init once
    osc.set_input_value(0, AMP)        # k-rate amplitude
    osc.set_input_value(1, BASE_FREQ)  # k-rate frequency
    osc.set_input_value(2, 1.0)        # table number (sine table)
    osc.init()

    total_kcycles = int(DURATION * sr / ksmps)
    k_dur = ksmps / sr
    t = 0.0

    # Start the pass-through instrument
    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s of {BASE_FREQ} Hz sine with "
          f"{VIB_RATE} Hz vibrato (depth +/-{VIB_DEPTH} Hz)...")

    for k in range(total_kcycles):
        # Compute vibrato frequency
        freq = BASE_FREQ + VIB_DEPTH * math.sin(2.0 * math.pi * VIB_RATE * t)

        # Update the k-rate frequency input (no re-init needed)
        osc.set_input_value(1, freq)

        # Perform one k-cycle
        osc.perform()

        # Copy UGen audio output into Csound's input buffer (spin)
        spin = cs.spin()
        samples = osc.get_output_buffer(0, ksmps)
        if samples:
            for i in range(ksmps):
                spin[i] = samples[i]

        # Let Csound process (instrument reads spin, outputs to DAC)
        if cs.perform_ksmps():
            break

        t += k_dur

    print("  Done.")

    osc.delete()
    factory.delete()
    cs.reset()
    print()


def example_realtime_graph():
    """Example 6: Real-time two-oscillator additive synthesis.

    Plays two oscili tones simultaneously through the DAC using a
    UGen graph.  The second oscillator's frequency glides smoothly
    from 660 Hz down to 330 Hz over 3 seconds by updating its
    k-rate frequency input each cycle.
    """
    print("=" * 60)
    print("Example 6: Real-time two-oscillator graph (oscili)")
    print("=" * 60)

    DURATION = 3.0
    AMP = 0.2

    cs = ctcsound.Csound()
    cs.set_option("-odac")
    cs.set_option("-d")
    cs.set_option("-m0")
    orc = ("sr = 44100\nksmps = 64\n0dbfs = 1\nnchnls = 1\n"
           "gifn ftgen 1, 0, 8192, 10, 1\n"
           "instr 1\n  asig in\n  out asig\nendin\n")
    cs.compile_orc(orc)
    cs.start()

    ksmps = cs.ksmps()
    sr = cs.sr()
    factory = ctcsound.UgenFactory(cs)

    # Create two oscili UGENs
    osc1 = factory.new_ugen("oscili", "a", "kkjo")
    osc2 = factory.new_ugen("oscili", "a", "kkjo")
    if osc1 is None or osc2 is None:
        print("ERROR: could not create oscili UGENs")
        factory.delete()
        cs.reset()
        return

    # Configure osc1: fixed 440 Hz
    osc1.set_input_value(0, AMP)
    osc1.set_input_value(1, 440.0)
    osc1.set_input_value(2, 1.0)   # sine table

    # Configure osc2: starts at 660 Hz
    osc2.set_input_value(0, AMP)
    osc2.set_input_value(1, 660.0)
    osc2.set_input_value(2, 1.0)   # sine table

    # Build graph and init once
    graph = factory.new_graph()
    graph.add(osc1)
    graph.add(osc2)
    graph.init()

    total_kcycles = int(DURATION * sr / ksmps)
    start_freq2 = 660.0
    end_freq2 = 330.0

    # Start the pass-through instrument
    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s: osc1=440Hz fixed, "
          f"osc2 glides {start_freq2}->{end_freq2} Hz...")

    for k in range(total_kcycles):
        frac = k / max(total_kcycles - 1, 1)
        freq2 = start_freq2 + (end_freq2 - start_freq2) * frac

        # Update osc2's k-rate frequency (no re-init needed)
        osc2.set_input_value(1, freq2)

        # Perform one k-cycle via the graph
        graph.perform()

        # Mix both outputs into spin
        spin = cs.spin()
        s1 = osc1.get_output_buffer(0, ksmps)
        s2 = osc2.get_output_buffer(0, ksmps)
        if s1 and s2:
            for i in range(ksmps):
                spin[i] = s1[i] + s2[i]

        if cs.perform_ksmps():
            break

    print("  Done.")

    graph.delete_all()
    factory.delete()
    cs.reset()
    print()


def example_buffer_passing():
    """Example 7: Passing output buffers between UGens.

    Shows how to connect the output of one UGen to the input of
    another using shared buffers.  You cannot wire UGens directly:

        osc2.set_input_value(1, osc1)   # NOT supported

    Instead, use set_output() / set_input() to point two UGens at
    the same ctypes buffer — done ONCE, before the loop.  After that,
    each perform() automatically reads/writes through the shared
    memory with no extra copying.

    This example uses a k-rate oscili as an LFO whose output is wired
    via a shared MYFLT to the frequency input of an audio-rate oscili
    carrier.  Because we need to add a base-frequency offset, a small
    arithmetic step remains in the loop, but the UGen ↔ buffer wiring
    itself is set up once.
    """
    print("=" * 60)
    print("Example 7: Passing output buffers between UGens")
    print("=" * 60)

    DURATION = 3.0
    BASE_FREQ = 440.0
    MOD_DEPTH = 40.0   # Hz deviation for vibrato
    MOD_RATE = 5.0      # LFO speed in Hz
    AMP = 0.25

    cs = ctcsound.Csound()
    cs.set_option("-odac")
    cs.set_option("-d")
    cs.set_option("-m0")
    orc = ("sr = 44100\nksmps = 64\n0dbfs = 1\nnchnls = 1\n"
           "gifn ftgen 1, 0, 8192, 10, 1\n"
           "instr 1\n  asig in\n  out asig\nendin\n")
    cs.compile_orc(orc)
    cs.start()

    ksmps = cs.ksmps()
    sr = cs.sr()
    factory = ctcsound.UgenFactory(cs)

    # --- Modulator: k-rate LFO ---
    # oscili with k-rate output ("k") produces one value per k-cycle.
    lfo = factory.new_ugen("oscili", "k", "kkjo")

    # --- Carrier: a-rate oscillator ---
    carrier = factory.new_ugen("oscili", "a", "kkjo")

    if lfo is None or carrier is None:
        print("ERROR: could not create UGens")
        factory.delete()
        cs.reset()
        return

    # Configure LFO: amp = MOD_DEPTH (outputs +/- MOD_DEPTH),
    # freq = MOD_RATE, table = sine
    lfo.set_input_value(0, MOD_DEPTH)
    lfo.set_input_value(1, MOD_RATE)
    lfo.set_input_value(2, 1.0)

    # Configure carrier: amp, table = sine
    carrier.set_input_value(0, AMP)
    carrier.set_input_value(2, 1.0)

    # --- Wire shared buffers (done ONCE, before the loop) ---
    #
    # Create a shared MYFLT that the LFO writes to and we read from:
    lfo_out = ctcsound.MYFLT(0.0)
    lfo.set_output(0, ct.byref(lfo_out))
    #
    # Create a shared MYFLT for the carrier's frequency input:
    carrier_freq = ctcsound.MYFLT(BASE_FREQ)
    carrier.set_input(1, ct.byref(carrier_freq))

    # Init both UGens
    lfo.init()
    carrier.init()

    total_kcycles = int(DURATION * sr / ksmps)

    # Start pass-through instrument
    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s: LFO ({MOD_RATE} Hz, +/-{MOD_DEPTH} Hz) "
          f"-> carrier ({BASE_FREQ} Hz)")
    print(f"  Wiring: lfo.output[0] -> shared MYFLT -> carrier.input[1]")

    for k in range(total_kcycles):
        # 1) Perform the LFO — writes its value into lfo_out
        lfo.perform()

        # 2) Compute carrier frequency from the shared LFO output
        carrier_freq.value = BASE_FREQ + lfo_out.value

        # 3) Perform the carrier — reads frequency from carrier_freq
        carrier.perform()

        # 4) Copy carrier audio output into Csound's input buffer
        spin = cs.spin()
        samples = carrier.get_output_buffer(0, ksmps)
        if samples:
            for i in range(ksmps):
                spin[i] = samples[i]

        if cs.perform_ksmps():
            break

    print("  Done.")

    lfo.delete()
    carrier.delete()
    factory.delete()
    cs.reset()
    print()


def main():
    print("\nCsound UGen API - Python Examples")
    print("=" * 60)
    print()

    example_single_ugen()
    example_list_opcodes()
    example_argument_types()
    example_ugen_graph()
    example_realtime_vibrato()
    example_realtime_graph()
    example_buffer_passing()

    print("All examples completed.")


if __name__ == "__main__":
    main()
