# Vendored DSSI dependencies

The Linux DSSI host uses these unmodified upstream headers:

| Dependency | Files | License |
| --- | --- | --- |
| [DSSI](dssi/README.md) | `dssi.h` | [LGPL-2.1-or-later](dssi/LICENSE) |
| [LADSPA](ladspa/README.md) | `ladspa.h` | [LGPL-2.1-or-later](ladspa/LICENSE) |

ALSA headers come from the system. No ALSA source is vendored.

Run `./update.py` to update both dependencies to their latest stable releases.
Run `./update.py --pinned` to reproduce the recorded releases and check their
archive hashes. Each directory also has an `update.py` that accepts a version.
The scripts need curl and Python 3 and work from any directory.

Review source and license changes and run the DSSI tests before committing an
update. Keep local changes outside the upstream headers.
