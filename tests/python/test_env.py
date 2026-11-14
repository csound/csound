"""Csound.env() gives what csoundGetEnv() gives: None for a variable that is
not set, and '' for one set to an empty value, which CSOUND7RC gives a
meaning of its own. It took the empty value for None."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


class TestEnv(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()

    def test_an_empty_value_is_not_none(self):
        self.assertEqual(self.cs.set_option('--env:CSOUND_PY_ENV_TEST='), 0)
        self.assertEqual(self.cs.env('CSOUND_PY_ENV_TEST'), '')

    def test_a_value_and_no_value(self):
        self.assertEqual(self.cs.set_option('--env:CSOUND_PY_ENV_TEST=x'), 0)
        self.assertEqual(self.cs.env('CSOUND_PY_ENV_TEST'), 'x')
        self.assertIsNone(self.cs.env('CSOUND_PY_ENV_TEST_UNSET'))


if __name__ == '__main__':
    unittest.main()
