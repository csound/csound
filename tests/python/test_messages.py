"""Check the text passed from ctcsound to Csound's message buffer."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'languages', 'python'))
import ctcsound


class TestMessages(unittest.TestCase):

    def setUp(self):
        self.cs = ctcsound.Csound()
        self.cs.create_message_buffer(False)

    def tearDown(self):
        self.cs.destroy_message_buffer()
        self.cs.destroy()

    def test_message_percent_format(self):
        self.cs.message('value %d, %s\n', 42, '50% café')
        self.assertEqual(self.cs.first_message(), 'value 42, 50% café\n')

    def test_message_brace_format(self):
        self.cs.message('{} voices, {}\n', 3, '50% café')
        self.assertEqual(self.cs.first_message(), '3 voices, 50% café\n')

    def test_message_s_percent_format(self):
        self.cs.message_S(0, 'format %d\n', 42)
        self.assertEqual(self.cs.first_message(), 'format 42\n')
        self.assertEqual(self.cs.first_message_attr(), 0)

    def test_message_s_brace_format_preserves_attributes(self):
        attributes = 0x4000  # CSOUNDMSG_WARNING from msg_attr.h
        self.cs.message_S(attributes, '{} voices, {}\n', 3, '50% café')
        self.assertEqual(self.cs.first_message(), '3 voices, 50% café\n')
        self.assertEqual(self.cs.first_message_attr(), attributes)


if __name__ == '__main__':
    unittest.main()
