"""Check that source edits cannot silently discard legacy maintenance policy."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "opcode_deprecations",
    Path(__file__).resolve().parents[2] / "scripts/opcode_deprecations.py")
metadata = importlib.util.module_from_spec(spec)
spec.loader.exec_module(metadata)

DESCRIPTOR = 'CSOUND_DEPRECATED_OPCODE("old", "new", FROZEN, "Keep old output.")'
MARKER = 'CSOUND_PRESERVE_LEGACY_BEHAVIOR("old")'
SOURCE = f'''{MARKER}
static int32_t old_init(CSOUND *csound, OP *p) {{ return 0; }}
{MARKER}
static int32_t old_perf(CSOUND *csound, OP *p) {{ return 0; }}
static OENTRY localops[] = {{
{DESCRIPTOR}
{{ "old", sizeof(OP), _QQ, "a", "k", (SUBR)old_init, (SUBR)old_perf, NULL }},
{{ "new", sizeof(OP), 0, "a", "k", NULL, NULL, NULL }}
}};
'''


class OpcodeDeprecationSourceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / "Opcodes").mkdir()
        self.source = self.root / "Opcodes/old.c"
        self.source.write_text(SOURCE)
        override = patch.object(metadata, "ROOT", self.root)
        override.start()
        self.addCleanup(override.stop)

    def check(self):
        rows = metadata.catalog()
        metadata.validate(rows)
        return rows

    def test_inline_policy_supplies_catalog(self):
        rows = self.check()
        self.assertEqual(rows, [dict(name="old", replacement="new", policy="FROZEN",
                                     note="Keep old output.")])

    def test_commented_descriptor_does_not_count(self):
        self.source.write_text(SOURCE.replace(DESCRIPTOR, "// " + DESCRIPTOR))
        with self.assertRaisesRegex(ValueError, "No source annotations"):
            self.check()

    def test_descriptor_in_another_file_does_not_cover_registration(self):
        self.source.write_text(SOURCE.replace(DESCRIPTOR, ""))
        (self.root / "Opcodes/elsewhere.c").write_text(DESCRIPTOR)
        with self.assertRaisesRegex(ValueError, "must be local"):
            self.check()

    def test_each_frozen_callback_needs_a_marker(self):
        for function in ("old_init", "old_perf"):
            with self.subTest(function=function):
                self.source.write_text(SOURCE.replace(
                    MARKER + "\nstatic int32_t " + function,
                    "// " + MARKER + "\nstatic int32_t " + function))
                with self.assertRaisesRegex(ValueError, function + " needs a preserve"):
                    self.check()

    def test_conflicting_descriptors_fail(self):
        (self.root / "Opcodes/duplicate.c").write_text(
            DESCRIPTOR.replace("FROZEN", "LEGACY"))
        with self.assertRaisesRegex(ValueError, "conflicting descriptors"):
            self.check()

    def test_identical_descriptors_share_one_catalog_entry(self):
        (self.root / "Opcodes/duplicate.c").write_text(DESCRIPTOR)
        self.assertEqual(len(self.check()), 1)

    def test_unknown_replacement_fails(self):
        self.source.write_text(SOURCE.replace(', "new", FROZEN', ', "missing", FROZEN'))
        with self.assertRaisesRegex(ValueError, "unknown replacement missing"):
            self.check()


if __name__ == "__main__":
    unittest.main()
