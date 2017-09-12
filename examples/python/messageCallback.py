# -*- coding: utf-8 -*-
"""
This example shows how to set the message callback to receive Csound's messages
in python
"""

from __future__ import print_function

import csnd6

cs=csnd6.Csound()

def message_callback(message):
    print("__message__: "  + message, end="")

cs.SetMessageCallback(message_callback)

cs.Start()
cs.Stop()