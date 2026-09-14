"""Check spatializer input reuse, active samples, and free-field tables."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class Spat3dTests(unittest.TestCase):
    def setUp(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def run_csd(self, body, tables="", score="i 1 0 .05", error=None):
        source = self.root / "test.csd"
        output = self.root / "output.wav"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=48000
ksmps=16
nchnls=4
0dbfs=1
{tables}
instr 1
{body}
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        result = subprocess.run([str(self.executable), "-W", "-f", "-o",
                                 str(output), str(source)], capture_output=True,
                                text=True, timeout=30, cwd=self.root)
        messages = result.stdout + result.stderr
        if error is not None:
            self.assertNotEqual(result.returncode, 0, messages)
            self.assertIn(error, messages)
            return None
        self.assertEqual(result.returncode, 0, messages)
        raw = output.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                return struct.unpack_from(f"<{size // 4}f", raw, position + 8)
            position += 8 + size + (size & 1)
        self.fail("No WAV sample data")

    def test_input_reuse_and_partial_blocks(self):
        # One filtered reflection exercises the recursive sample span as well.
        room = "giRoom ftgen 90,0,64,-2,1,0,-1,-1,-1,0,1,6,0,.7,4000,.6,.7,2"
        for opcode in ("spat3di", "spat3d"):
            for mode in range(5):
                with self.subTest(opcode=opcode, mode=mode):
                    extra = ",.1,2" if opcode == "spat3d" else ""
                    args = f"1,2,3,1,giRoom,{mode}{extra}"
                    body = ["aInput upsamp .125",
                            f"aW,aX,aY,aZ {opcode} aInput,{args}"]
                    for alias in range(4):
                        outputs = [f"aAlias{alias}{channel}" for channel in "WXYZ"]
                        source = outputs[alias]
                        body += [f"{source} init 0", "kN=0", "while kN<ksmps do",
                                 # Fill only allocated slots, including inactive padding.
                                 f" vaset .125,kN,{source}", " kN+=1", "od",
                                 f"{','.join(outputs)} {opcode} {source},{args}"]
                    body += ["kCycle init 0", "kStart=(kCycle==0 ? 3 : 0)",
                             "kEnd=min(ksmps,2409-kCycle*ksmps)", "kN=0",
                             "while kN<ksmps do"]
                    for channel in "WXYZ":
                        body += [f" kReference vaget kN,a{channel}",
                                 " if (kN<kStart || kN>=kEnd) && kReference!=0 then",
                                 '  printks "inactive output is not zero\\n",0',
                                 "  exitnowk 1", " endif"]
                        for alias in range(4):
                            body += [f" kAlias vaget kN,aAlias{alias}{channel}",
                                     " if abs(kAlias-kReference)>0.000001 then",
                                     '  printks "input reuse or inactive input changed output\\n",0',
                                     "  exitnowk 1", " endif"]
                    body += [" kN+=1", "od", "kCycle+=1", "outq aW,aX,aY,aZ"]
                    samples = self.run_csd("\n".join(body), room,
                                          "i 1 .0000625 .050125\ni 1 .1000625 .050125")
                    first = samples[3*4:2409*4]
                    second = samples[4803*4:7209*4]
                    self.assertGreater(max(map(abs, first)), .001)
                    self.assertEqual(first, second, "reused note state")

    def test_free_field(self):
        room = "giRoom ftgen 90,0,64,-2,0,0,-1,-1,-1"
        for opcode in ("spat3di", "spat3d"):
            with self.subTest(opcode=opcode):
                extra = ",.1,2" if opcode == "spat3d" else ""
                def render(table):
                    return self.run_csd(f"aInput upsamp .125\n"
                                        f"aW,aX,aY,aZ {opcode} aInput,1,2,3,1,{table},3{extra}\n"
                                        "outq aW,aX,aY,aZ", room)
                reference = render("giRoom")
                self.assertGreater(max(map(abs, reference)), .001)
                self.assertEqual(reference, render(0))
                self.assertEqual(reference, render(-1))

    def test_impulse_response_free_field(self):
        tables = "\n".join(f"giOut{n} ftgen {n},0,4096,-2,0" for n in (1, 2, 3))
        tables += "\ngiRoom ftgen 90,0,64,-2,-1,0,-1,-1,-1"
        body = "\n".join(f"spat3dt {n},1,2,3,1,{room},3,.005"
                         for n, room in ((1, 0), (2, -1), (3, "giRoom")))
        body += '''
iN=0
iPeak=0
while iN<4096 do
 iA table iN,1
 iB table iN,2
 iC table iN,3
 if iA!=iB || iA!=iC then
  exitnow 1
 endif
 iPeak=max(iPeak,abs(iA))
 iN+=1
od
if iPeak<.001 then
 exitnow 1
endif
'''
        self.run_csd(body, tables)

    def test_table_errors(self):
        for opcode in ("spat3di", "spat3d", "spat3dt"):
            extra = ",.1,2" if opcode == "spat3d" else ""
            def call(room):
                if opcode == "spat3dt":
                    return f"spat3dt 1,1,2,3,1,{room},3,.005"
                return f"aInput init .125\naW,aX,aY,aZ {opcode} aInput,1,2,3,1,{room},3{extra}"
            for room, error in ((99, "Invalid ftable no."),
                                (2, "room table needs 54 values")):
                with self.subTest(opcode=opcode, room=room):
                    self.run_csd(call(room), "giOut ftgen 1,0,4096,-2,0\n"
                                 "giShort ftgen 2,0,32,-2,0", error=error)
        self.run_csd("spat3dt 99,1,2,3,1,0,3,.005", error="Invalid ftable no.")
        self.run_csd("spat3dt 1,1,2,3,1,0,3,.005",
                     "giOut ftgen 1,0,2,-2,0", error="output table needs at least 4 values")


if __name__ == "__main__":
    unittest.main()
