#!/usr/bin/env python3

# Run from any directory. With no argument, select the latest numbered stable
# release. A version selects that release; --pinned reproduces SOURCE.json.
# Requires curl and Python 3. No upstream build scripts are run.
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile

source_dir = Path(__file__).resolve().parent
if len(sys.argv) > 2 or (len(sys.argv) == 2 and sys.argv[1] == '--help'):
    print(f'Usage: {sys.argv[0]} [VERSION|--pinned]')
    sys.exit(0 if len(sys.argv) == 2 else 1)
selection = sys.argv[1] if len(sys.argv) == 2 else 'latest'

def fetch(url, destination):
    subprocess.run(['curl', '--fail', '--location', '--silent', '--show-error',
                    '--retry', '3', '--connect-timeout', '20', '--max-time', '120',
                    url, '-o', str(destination)], check=True)

with tempfile.TemporaryDirectory(prefix='csound-dssi-') as temporary:
    temporary = Path(temporary)
    recorded = None
    if selection == '--pinned':
        recorded = json.loads((source_dir / 'SOURCE.json').read_text())
        version = recorded['version']
    elif selection == 'latest':
        listing = temporary / 'releases.html'
        fetch('https://sourceforge.net/projects/dssi/files/dssi/', listing)
        versions = re.findall('/dssi/(\\d+(?:\\.\\d+)+)/', listing.read_text())
        if not versions:
            sys.exit('No stable release found; checked-in files have not changed')
        version = max(versions, key=lambda value: tuple(map(int, value.split('.'))))
    else:
        version = selection
    if not re.fullmatch(r'\d+(?:\.\d+)+', version):
        sys.exit('Expected a numbered release, for example 1.2.3')

    url = 'https://downloads.sourceforge.net/project/dssi/dssi/{version}/dssi-{version}.tar.gz'.format(version=version)
    archive = temporary / 'source.tar'
    fetch(url, archive)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    if recorded and digest != recorded['sha256']:
        sys.exit('Archive differs from SOURCE.json; checked-in files have not changed')
    files = {'dssi.h': 'dssi-{version}/dssi/dssi.h', 'LICENSE': 'dssi-{version}/COPYING'}
    staged = {}
    # Read only named files. Preserve upstream bytes, including license notices.
    with tarfile.open(archive) as package:
        for destination, member in files.items():
            entry = package.getmember(member.format(version=version))
            if not entry.isfile():
                sys.exit(f'Expected a file: {entry.name}')
            staged[destination] = package.extractfile(entry).read()
    metadata = {'version': version, 'url': url, 'sha256': digest,
                'files': {name: member.format(version=version) for name, member in files.items()}}
    staged['SOURCE.json'] = (json.dumps(metadata, indent=2) + '\n').encode()
    readme = f"""# DSSI

Unmodified files from [release {version}]({url}).

Only the DSSI API header is vendored.
The header uses LGPL-2.1-or-later; see [LICENSE](LICENSE) and its license notice.

[SOURCE.json](SOURCE.json) records the archive URL, SHA-256, and file paths.
Keep platform adapters in Csound's code, outside this directory.

Run `./update.py` for the latest stable release, `./update.py VERSION` for a
specific release, or `./update.py --pinned` to reproduce this copy and check its
archive hash. Downloads and extraction finish before checked-in files change.
The script requires curl and Python 3 and works from any directory.

After an update, review the source and license diff, rebuild the DSSI host on
Linux, and run `ctest --test-dir build -R '^DssiTests[.]' --output-on-failure`.
Commit the header, license, README, and SOURCE.json together.
"""
    staged['README.md'] = readme.encode()
    # A failed download or missing archive member leaves the old copy intact.
    for name, data in staged.items():
        destination = source_dir / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    print(f'Updated DSSI to {version}. Source: {url}')
