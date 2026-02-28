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

# Shorthand for the ctypes MYFLT type
MYFLT = ctcsound.MYFLT


def example_single_ugen():
    """Example 1: Create and run a single oscils UGen.

    Uses oscils (simple sine oscillator) to generate one k-cycle of
    audio and prints the first few samples.

    Demonstrates the convenience methods set_value() and get_value()
    for one-off init-time parameter setup.
    """
    print("=" * 60)
    print("Example 1: Single oscils UGen")
    print("=" * 60)

    # Create and start Csound
    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\n0dbfs = 1\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)

    # Create an oscils UGen: output "a" (audio), inputs "iiio"
    osc = factory.new_ugen("oscils", "a", "iiio")
    if osc is None:
        print("ERROR: could not create oscils UGen")
        return

    print(f"  oscils: {osc.in_count} inputs, {osc.out_count} outputs")

    # Use convenience methods for init-time setup.
    # osc.set_value(index, val) is shorthand for
    #   osc.get_in_var(index).set_value(val)
    osc.set_value(0, 0.5)    # amplitude
    osc.set_value(1, 440.0)  # frequency
    osc.set_value(2, 0.0)    # phase

    # Initialize and perform one k-cycle
    osc.init()
    osc.perform()

    # Read the audio output buffer via UGEN_VAR data pointer
    ksmps = cs.ksmps()
    out_var = osc.get_out_var(0)
    ptr = out_var.data_ptr
    if ptr:
        buf = (MYFLT * ksmps).from_address(ptr)
        samples = list(buf)
        print(f"  Generated {len(samples)} samples (ksmps={ksmps})")
        print(f"  First 8 samples: {[f'{s:.6f}' for s in samples[:8]]}")

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

    opcodes = factory.list_opcodes()
    print(f"  Total opcodes available: {len(opcodes)}")

    print("  First 10 opcodes:")
    for entry in opcodes[:10]:
        print(f"    {entry['opname']:20s} out={entry['outypes']!r:8s} "
              f"in={entry['intypes']!r}")

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
    """Example 3: Query argument types and sizes via UGEN_VAR.

    Demonstrates reading the type and size of each argument by obtaining
    UGEN_VAR handles and querying their .arg_type and .size properties.
    """
    print("=" * 60)
    print("Example 3: Query argument types via UGEN_VAR")
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

    osc = factory.new_ugen("oscils", "a", "iiio")
    if osc is None:
        print("ERROR: could not create oscils UGen")
        return

    print("  oscils argument info (via UGEN_VAR):")
    for i in range(osc.out_count):
        var = osc.get_out_var(i)
        t = var.arg_type
        sz = var.size
        print(f"    output[{i}]: type={TYPE_NAMES.get(t, '?'):6s}  "
              f"size={sz} bytes")

    for i in range(osc.in_count):
        var = osc.get_in_var(i)
        t = var.arg_type
        sz = var.size
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

    Uses convenience methods for init-time parameter setup.
    """
    print("=" * 60)
    print("Example 4: UGen graph")
    print("=" * 60)

    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\n0dbfs = 1\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)
    graph = factory.new_graph()

    osc1 = factory.new_ugen("oscils", "a", "iiio")
    osc2 = factory.new_ugen("oscils", "a", "iiio")

    if osc1 is None or osc2 is None:
        print("ERROR: could not create oscils UGENs")
        return

    # Convenience methods — ideal for one-time init setup
    osc1.set_value(0, 0.5)    # amp
    osc1.set_value(1, 440.0)  # freq
    osc1.set_value(2, 0.0)    # phase

    osc2.set_value(0, 0.3)
    osc2.set_value(1, 660.0)
    osc2.set_value(2, 0.0)

    idx1 = graph.add(osc1)
    idx2 = graph.add(osc2)
    print(f"  Added osc1 at index {idx1}, osc2 at index {idx2}")

    graph.init()
    graph.perform()

    # Read output via UGEN_VAR data pointers (needed for audio buffers)
    ksmps = cs.ksmps()
    ptr1 = osc1.get_out_var(0).data_ptr
    ptr2 = osc2.get_out_var(0).data_ptr

    if ptr1:
        buf1 = (MYFLT * ksmps).from_address(ptr1)
        print(f"  osc1 (440 Hz): first 8 samples = "
              f"{[f'{s:.6f}' for s in list(buf1)[:8]]}")
    if ptr2:
        buf2 = (MYFLT * ksmps).from_address(ptr2)
        print(f"  osc2 (660 Hz): first 8 samples = "
              f"{[f'{s:.6f}' for s in list(buf2)[:8]]}")

    graph.delete_all()
    factory.delete()
    cs.reset()
    print()


def example_realtime_vibrato():
    """Example 5: Real-time sine with vibrato (pitch modulation).

    Plays a 440 Hz sine tone through the DAC with vibrato.

    Shows the two-level API pattern:
      - Convenience set_value() for one-off init-time setup
      - Cached UGEN_VAR handle for the freq input that changes every
        k-cycle (more efficient: avoids index lookup each call)
    """
    print("=" * 60)
    print("Example 5: Real-time sine with vibrato (oscili)")
    print("=" * 60)

    DURATION = 4.0
    BASE_FREQ = 440.0
    VIB_DEPTH = 40.0
    VIB_RATE = 5.0
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

    osc = factory.new_ugen("oscili", "a", "kkjo")
    if osc is None:
        print("ERROR: could not create oscili UGen")
        factory.delete()
        cs.reset()
        return

    # Convenience methods for init-time parameters that don't change
    osc.set_value(0, AMP)        # k-rate amplitude (fixed)
    osc.set_value(1, BASE_FREQ)  # k-rate frequency (will be updated)
    osc.set_value(2, 1.0)        # table number (fixed)
    osc.init()

    # Cache the UGEN_VAR handle for the freq input — this is more
    # efficient than calling osc.set_value(1, ...) every k-cycle
    # because it avoids the internal index lookup each iteration.
    freq_var = osc.get_in_var(1)

    total_kcycles = int(DURATION * sr / ksmps)
    k_dur = ksmps / sr
    t = 0.0

    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s of {BASE_FREQ} Hz sine with "
          f"{VIB_RATE} Hz vibrato (depth +/-{VIB_DEPTH} Hz)...")

    for k in range(total_kcycles):
        freq = BASE_FREQ + VIB_DEPTH * math.sin(2.0 * math.pi * VIB_RATE * t)

        # Use the cached UGEN_VAR handle for efficient per-k-cycle updates
        freq_var.set_value(freq)

        osc.perform()

        spin = cs.spin()
        ptr = osc.get_out_var(0).data_ptr
        if ptr:
            buf = (MYFLT * ksmps).from_address(ptr)
            for i in range(ksmps):
                spin[i] = buf[i]

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
    from 660 Hz down to 330 Hz over 3 seconds.

    Init-time: convenience set_value()
    Per-k-cycle: cached UGEN_VAR handle for the gliding frequency
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

    osc1 = factory.new_ugen("oscili", "a", "kkjo")
    osc2 = factory.new_ugen("oscili", "a", "kkjo")
    if osc1 is None or osc2 is None:
        print("ERROR: could not create oscili UGENs")
        factory.delete()
        cs.reset()
        return

    # Convenience for init-time setup
    osc1.set_value(0, AMP)
    osc1.set_value(1, 440.0)
    osc1.set_value(2, 1.0)

    osc2.set_value(0, AMP)
    osc2.set_value(1, 660.0)
    osc2.set_value(2, 1.0)

    # Cache the osc2 freq var for per-k-cycle updates in the loop
    osc2_freq_var = osc2.get_in_var(1)

    graph = factory.new_graph()
    graph.add(osc1)
    graph.add(osc2)
    graph.init()

    total_kcycles = int(DURATION * sr / ksmps)
    start_freq2 = 660.0
    end_freq2 = 330.0

    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s: osc1=440Hz fixed, "
          f"osc2 glides {start_freq2}->{end_freq2} Hz...")

    for k in range(total_kcycles):
        frac = k / max(total_kcycles - 1, 1)
        freq2 = start_freq2 + (end_freq2 - start_freq2) * frac

        osc2_freq_var.set_value(freq2)
        graph.perform()

        spin = cs.spin()
        ptr1 = osc1.get_out_var(0).data_ptr
        ptr2 = osc2.get_out_var(0).data_ptr
        if ptr1 and ptr2:
            buf1 = (MYFLT * ksmps).from_address(ptr1)
            buf2 = (MYFLT * ksmps).from_address(ptr2)
            for i in range(ksmps):
                spin[i] = buf1[i] + buf2[i]

        if cs.perform_ksmps():
            break

    print("  Done.")
    graph.delete_all()
    factory.delete()
    cs.reset()
    print()


def example_ugen_wiring():
    """Example 7: Direct UGen-to-UGen wiring with set_input_var.

    A k-rate LFO modulator feeds into the carrier oscillator's
    frequency.  Because we need to add a base-frequency offset,
    we use cached UGEN_VAR handles (read LFO output, write carrier
    input) each k-cycle.

    Convenience set_value() is used for init-time parameters.
    Cached UGEN_VAR handles are used for the per-k-cycle updates.
    """
    print("=" * 60)
    print("Example 7: UGen wiring with UGEN_VAR")
    print("=" * 60)

    DURATION = 3.0
    BASE_FREQ = 440.0
    MOD_DEPTH = 40.0
    MOD_RATE = 5.0
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

    lfo = factory.new_ugen("oscili", "k", "kkjo")
    carrier = factory.new_ugen("oscili", "a", "kkjo")

    if lfo is None or carrier is None:
        print("ERROR: could not create UGens")
        factory.delete()
        cs.reset()
        return

    # Convenience for init-time setup
    lfo.set_value(0, MOD_DEPTH)
    lfo.set_value(1, MOD_RATE)
    lfo.set_value(2, 1.0)

    carrier.set_value(0, AMP)
    carrier.set_value(2, 1.0)

    # Cache UGEN_VAR handles for the per-k-cycle loop —
    # more efficient than calling set_value()/get_value() each cycle
    lfo_out_var = lfo.get_out_var(0)
    carrier_freq_var = carrier.get_in_var(1)

    # NOTE: For DIRECT zero-copy wiring (when no offset is needed):
    #   carrier.set_input_var(1, lfo.get_out_var(0))
    # Here we need to add BASE_FREQ, so we read/write manually.

    lfo.init()
    carrier.init()

    total_kcycles = int(DURATION * sr / ksmps)
    cs.event_string("i1 0 %f" % (DURATION + 1.0))

    print(f"  Playing {DURATION}s: LFO ({MOD_RATE} Hz, +/-{MOD_DEPTH} Hz) "
          f"-> carrier ({BASE_FREQ} Hz)")

    for k in range(total_kcycles):
        lfo.perform()
        lfo_val = lfo_out_var.get_value()
        carrier_freq_var.set_value(BASE_FREQ + lfo_val)
        carrier.perform()

        spin = cs.spin()
        ptr = carrier.get_out_var(0).data_ptr
        if ptr:
            buf = (MYFLT * ksmps).from_address(ptr)
            for i in range(ksmps):
                spin[i] = buf[i]

        if cs.perform_ksmps():
            break

    print("  Done.")
    lfo.delete()
    carrier.delete()
    factory.delete()
    cs.reset()
    print()


def example_string_ugen():
    """Example 8: String manipulation with UGen API.

    Demonstrates using opcodes that process string (S-type) arguments.
    Uses the strcat opcode to concatenate two strings, showcasing:
      - Convenience set_string() / get_string() for one-off operations
      - UGEN_VAR string handles for direct access
    """
    print("=" * 60)
    print("Example 8: String manipulation (strcat)")
    print("=" * 60)

    cs = ctcsound.Csound()
    cs.compile_orc("sr = 44100\nksmps = 64\ninstr 1\nendin")
    cs.start()

    factory = ctcsound.UgenFactory(cs)

    # strcat: S -> SS  (concatenates two strings)
    cat = factory.new_ugen("strcat", "S", "SS")
    if cat is None:
        print("  strcat opcode not available, skipping")
        factory.delete()
        cs.reset()
        print()
        return

    print(f"  strcat: {cat.in_count} inputs, {cat.out_count} outputs")

    # --- Method 1: Convenience set_string() / get_string() ---
    # Ideal for one-off init-time string setup
    cat.set_string(0, "Hello, ")
    cat.set_string(1, "Csound UGen World!")

    cat.init()

    # get_string() reads from an output argument by index
    result = cat.get_string(0)
    print(f"  Convenience:  strcat(\"Hello, \", \"Csound UGen World!\") "
          f"= \"{result}\"")

    # --- Method 2: UGEN_VAR handles for direct access ---
    # Useful when you need the var handle for other operations
    # (e.g., type queries, wiring, or repeated updates)
    in_var0 = cat.get_in_var(0)
    in_var1 = cat.get_in_var(1)
    out_var = cat.get_out_var(0)

    print(f"  Input[0] type: {in_var0.arg_type} (S={ctcsound.UGEN_ARG_TYPE_S})")
    print(f"  Output[0] type: {out_var.arg_type}")

    # Update strings via UGEN_VAR and re-init
    in_var0.set_string("UGen API ")
    in_var1.set_string("rocks!")
    cat.init()

    result2 = out_var.get_string()
    print(f"  UGEN_VAR:     strcat(\"UGen API \", \"rocks!\") = \"{result2}\"")

    # --- Standalone string var ---
    print("\n  Standalone string var:")
    svar = factory.new_var(ctcsound.UGEN_ARG_TYPE_S)
    if svar:
        svar.set_string("standalone string value")
        print(f"  Created S var: \"{svar.get_string()}\"")
        svar.set_string("updated value")
        print(f"  Updated S var: \"{svar.get_string()}\"")
        svar.delete()

    cat.delete()
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
    example_string_ugen()
    example_realtime_vibrato()
    example_realtime_graph()
    example_ugen_wiring()

    print("All examples completed.")


if __name__ == "__main__":
    main()
