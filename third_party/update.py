#!/usr/bin/env python3

# Update the vendored DSSI dependencies. Each also has its own update.py.
from pathlib import Path
import subprocess
import sys

if sys.argv[1:] not in ([], ['--pinned']):
    sys.exit(f'Usage: {sys.argv[0]} [--pinned]')
source_dir = Path(__file__).resolve().parent
for dependency in ('dssi', 'ladspa'):
    subprocess.run([sys.executable, str(source_dir / dependency / 'update.py'),
                    *sys.argv[1:]], check=True)
