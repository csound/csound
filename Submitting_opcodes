Protocol for submitting new opcodes
===================================

The developers are happy to accept code for new opcodes (and other
parts) but in order to make the process simple and clear we have drawn
up this protocol in the hope that it will avoid embarrassing delays and
discussions.


Licence
-------

In general Csound is licenced with LGPL so submissions need to have that
licence or a compatible one.  Note that GPL is a problem.

Examples of suitable copyright and licence text can be found at the
start of most opcode files.

Names
-----

New opcodes should not have names that start with the letters i, k, f
or a.  This is to avoid name clashes with existing orchestras; and yes
it has happened in the past.

Coding
------

Please follow the general style; it is important not to have any
static variables which are not constant, while static fuctions are
encouraged.  Any use of external libraries which introduces new
dependencies is a potential problem and may be best discussed with the
core team.


Formatting/Indentation
----------------------

Ideally C code should not have any tab characters in it, and be
limitted to 85 characters wide.  Format should be similar to existing
code.  Also wrap any error strings with a call to Str to assist with
language translations.

Sample-accurate
---------------

If an opcode returns a audio-rate value or uses audio-rate input it
must include support for sample-accurate mode.  Again looking at
existing opcodes shows a template for this.

The template for arate perf-pass opcodes is:

    int perf_myopcode(CSOUND *csound, MYOPCODE *p)
    {
        uint32_t offset = p->h.insdshead->ksmps_offset; // delayed onset
        uint32_t early  = p->h.insdshead->ksmps_no_end; // early end of event
        uint32_t nsmps = CS_KSMPS;
        ...
        // Clear parts of the output outside event
        if (UNLIKELY(offset)) memset(p->res, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early))  {
          nsmps -= early;
          memset(&p->res[nsmps], '\0', early*sizeof(MYFLT));
        }
        for (n=offset; n<nsmps; n++) { // Only calculate inside event
            .....
            p->res[n] = ....
        }
        return OK;
    }


Core or Plugin?
---------------

New opcodes can be either in the core library, or available as
plugins.  Which is used depends on a number of factors.  If an opcode
has external dependencies it can ONLY be added to the Csound standard
codebase as a plugin. For platforms where plugins are not available,
special packages can be prepared to include an opcode and its
dependencies within the core library.  if the opcode is only useful on
one platform (like the Linux joystick opcode) then a plugin is to be
preferred.  For others it is a balance between code growth and
utility.


Documentation
-------------

All new opcodes or gens need a manual page, if only a minimal
one. Even better if it includes an example .csd file.  There is a
template in the manual tree opcodes/template.xml, and example files in
the examples directory.


Submission
----------

Probably easiest as a pull request on github, but direct mail to the
developers' mailing list or direct contact with a core developer are OK.

==JPff 2017 Dec 14
