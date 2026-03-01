"""Tests for the ctcsound UGEN API, focusing on csoundUgenGetValue.

Run with:
    python -m pytest tests/python/test_ugen.py -v
or:
    python tests/python/test_ugen.py
"""

import sys
import os
import unittest
import math

# Ensure ctcsound can be found (adjust path if needed)
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'Python'))
import ctcsound


class TestUgenGetValue(unittest.TestCase):
    """Test Ugen.get_value() — reads from output args after perform."""

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.set_option('-d')       # no displays
        self.cs.set_option('-n')       # no audio output
        self.cs.set_option('--nchnls=1')
        self.cs.set_option('--0dbfs=1')
        self.cs.start()
        self.factory = ctcsound.UgenFactory(self.cs)

    def tearDown(self):
        self.factory.delete()
        self.cs.reset()

    # ---- oscils: a-rate output ----

    def test_get_value_oscils_after_perform(self):
        """GetValue on oscils output returns first audio sample after perform."""
        osc = self.factory.new_ugen('oscils', 'a', 'iiio')
        self.assertIsNotNone(osc)

        osc.set_value(0, 1.0)    # amp
        osc.set_value(1, 1000.0) # freq
        osc.set_value(2, 0.25)   # phase — quarter-cycle so sin(π/2)≈1

        self.assertEqual(osc.init(), 0)
        self.assertEqual(osc.perform(), 0)

        # get_value reads output[0]; first sample should be near 1.0
        out_val = osc.get_value(0)
        self.assertAlmostEqual(out_val, 1.0, delta=0.05,
                               msg='oscils first sample with phase=0.25')
        osc.delete()

    def test_get_value_matches_var_get_value(self):
        """GetValue returns the same value as get_out_var().get_value()."""
        osc = self.factory.new_ugen('oscils', 'a', 'iiio')
        self.assertIsNotNone(osc)

        osc.set_value(0, 1.0)
        osc.set_value(1, 1000.0)
        osc.set_value(2, 0.25)

        self.assertEqual(osc.init(), 0)
        self.assertEqual(osc.perform(), 0)

        out_var = osc.get_out_var(0)
        self.assertIsNotNone(out_var)
        self.assertAlmostEqual(osc.get_value(0), out_var.get_value(),
                               places=10, msg='get_value == get_out_var().get_value()')
        osc.delete()

    # ---- line: k-rate output ----

    def test_get_value_line_k_rate(self):
        """GetValue on line (k-rate output) returns a scalar between ia and ib."""
        ln = self.factory.new_ugen('line', 'k', 'iii')
        self.assertIsNotNone(ln)

        # Start at 1.0, ramp to 0.0 — first output should be near 1.0
        ln.set_value(0, 1.0)  # ia
        ln.set_value(1, 1.0)  # dur
        ln.set_value(2, 0.0)  # ib

        self.assertEqual(ln.init(), 0)
        self.assertEqual(ln.perform(), 0)

        out_k = ln.get_value(0)
        self.assertGreater(out_k, 0.0,
                           msg='line output should be > 0 after one k-cycle')
        self.assertLessEqual(out_k, 1.0,
                             msg='line output should be <= 1.0')
        ln.delete()

    # ---- edge cases ----

    def test_get_value_before_perform_is_zero(self):
        """GetValue before perform returns 0 (output not yet written)."""
        osc = self.factory.new_ugen('oscils', 'a', 'iiio')
        self.assertIsNotNone(osc)

        osc.set_value(0, 1.0)
        osc.set_value(1, 440.0)
        osc.set_value(2, 0.0)

        # Output hasn't been written yet
        self.assertEqual(osc.get_value(0), 0.0)
        osc.delete()

    def test_get_value_out_of_range_returns_zero(self):
        """GetValue with out-of-range index returns 0."""
        osc = self.factory.new_ugen('oscils', 'a', 'iiio')
        self.assertIsNotNone(osc)

        osc.set_value(0, 1.0)
        osc.set_value(1, 440.0)
        osc.set_value(2, 0.0)
        self.assertEqual(osc.init(), 0)
        self.assertEqual(osc.perform(), 0)

        self.assertEqual(osc.get_value(99), 0.0)
        osc.delete()

    def test_set_value_writes_inputs_not_outputs(self):
        """SetValue writes to inputs; verify via get_in_var().get_value()."""
        osc = self.factory.new_ugen('oscils', 'a', 'iiio')
        self.assertIsNotNone(osc)

        osc.set_value(0, 0.75)
        osc.set_value(1, 880.0)
        osc.set_value(2, 0.25)

        # Read back inputs via UgenVar
        in0 = osc.get_in_var(0)
        in1 = osc.get_in_var(1)
        in2 = osc.get_in_var(2)
        self.assertIsNotNone(in0)
        self.assertIsNotNone(in1)
        self.assertIsNotNone(in2)
        self.assertAlmostEqual(in0.get_value(), 0.75, places=6)
        self.assertAlmostEqual(in1.get_value(), 880.0, places=6)
        self.assertAlmostEqual(in2.get_value(), 0.25, places=6)

        osc.delete()


if __name__ == '__main__':
    unittest.main()
