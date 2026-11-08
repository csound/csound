This file contains information that has not yet been incorporated into the system documentation, mostly taken from mailing list messages.

===

fftlib.c  contains the pow-of-two fft code, mxfft.c the non-pow-of-two fft code and pvsanal.c contains
the pvsanal etc opcodes (which use the fft code).

===

1) We can create variables as the instrument is constructed by the
compiler (create_instrument()), this
adds the variables to the instrument pool correctly.
2) In the case of ksmps and kr, these were originally thought of as
global symbols, so when they appear
in local code, we need to trapped them, which we can do by checking if
they exist in the local pool (createArg())
3) In order to be able to access them more easily, we also have to
point their memBlock to the correct
slot in the pool, which we can do as the instrument is instantiated
(instance()).
4) ksmps and kr are originally set to the global values of ksmps and
kr, but in setksmps they are set to
whatever new value is requested. Also in the case of UDOs, we make
sure that they are set to local
values in case these are passed as the last (optional) argument to the opcode.

What took me a while to figure out was 3), because the current code
only dealt with arguments that were
explicitly used in the code, anything created internally was ignored
by instance(), and so the memBlock of
the internal variables was not being initialised to the slot in the
pool (but the slot was there as the pool
was correctly created).
