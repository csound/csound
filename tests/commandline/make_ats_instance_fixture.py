"""Recreate ats-instance.ats for the ATS instance tests. No audio input needed."""

from pathlib import Path
import struct

# ATS type 1 stores a time followed by amplitude/frequency pairs.
# Both frames contain the same four partials, so time interpolation is constant.
header = [123, 8000, 4000, 8000, 4, 2, .8, 800, 1, 1]
partials = [.1, 100, .2, 200, .4, 400, .8, 800]
values = header + [0] + partials + [.5] + partials
Path(__file__).with_name("ats-instance.ats").write_bytes(
    struct.pack("<" + "d" * len(values), *values)
)
