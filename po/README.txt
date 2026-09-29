Save all catalogs as UTF-8 so accents and non-Latin text remain intact.

Run ./po/update from a Git checkout to refresh every catalog from the current
source messages. This requires GNU gettext. Review fuzzy entries against the
new msgid before removing the fuzzy flag.
Gettext leaves fuzzy entries out of installed catalogs and uses the source
message until the translation is ready.

Wrap user-facing messages in Str("...") at the call site. For stored messages,
mark the literal with Str_noop("...") and call Str when displaying it. The
C++ plugin helpers init_error, perf_error, warning and message already translate
their arguments, so mark their literals with Str_noop. Keep complete sentences
together so translators can change the word order.

Do not translate opcode names, code examples, file contents, protocol strings,
user-supplied text or raw debug dumps.

Keep printf arguments (such as %s and %d), opcode names, parameter names and
command-line options intact. Preserve line breaks and spacing used for output
layout. Run ./po/tester to check the catalogs before submitting changes.

csound.po provides UK English; american.po provides US English. Add any new
locale to both LINGUAS and CMakeLists.txt so the build installs its catalog.
