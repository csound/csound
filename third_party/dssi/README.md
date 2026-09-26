# DSSI

Unmodified files from [release 1.1.1](https://downloads.sourceforge.net/project/dssi/dssi/1.1.1/dssi-1.1.1.tar.gz).

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
