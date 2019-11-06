# Soak tests

The soak tests are a series of reproduceable tests for a continuous integration coverting (hopefully) of all core opcodes and operators.

For the soak tests to have any meaning the cmake flag USE\_COMPILER\_OPTIMIZATIONS must be 0 or OFF, which should disable -ffast-math and keep floating point math (-mfpmath) deterministic, as well as setting the optimization level to 0 (-O0).

## opcodes

The opcodes directory is organized by opcode name (with the exception on operators and statements) and test. For a test to be included in the test-coverage, the directory name needs to match with the name of the opcode being tested.

The tests are based on the manual examples, but many have been modified for reproduceability and for brevity (test should preferable not exceed 5-10 seconds). A test in the opcodes directory must produce audio to make sense, since these tests are based on checksum match of the .wav output and an expected checksum.

To add or modify a test, make sure to include a foo.csd file and a matching foo.csd.json, where foo.csd.json is a json map containing 3 keys `"sha256"` for the expected checksum, `"os"` for the operating system the expected sha was generated from and `"git_commit"` for the git revision of the csound build used to produce the audio binary.

## dryruns [wip]

Dryruns is a collection of opcodes organized in the same way as the opcodes directory but containing opcodes and operators that are better tested in other ways than the audio they may or may nor manipulate. A good example of these are array and disk i/o operators.


## resources and include

The include directory contains all binary artifacts needed to run the test suite except for .pvx binaries. They need to be generated each time csound is changed. To do this, open the include directory in the terminal and run the following:

```
csound -U pvanal flute.aiff flute.pvx
csound -U pvanal mary.wav mary.pvx
csound -U pvanal beats.wav beats.pvx
```

And make sure to reference this directory when running a soak test which includes an include. For example:

```
SSDIR=../../include INCDIR=../../include SADIR=../../include csound loscil.csd -o loscil.wav
```


## helpful commands(dev)

- replace the sha256sum with bash and npm's json package quickly (install json with `npm i json -g`)

```bash
file=vbap8move && sha256sum $(echo $file.wav) | cut -d " " -f 1 | cat | { read sha256 ; json -I -f $file.csd.json -e "this.sha256="'"'$sha256'"';}
```

- quickly test the sha256sum and see if multiple invocations produce the same sha

```bash
file=0dbfs && SSDIR=../../include INCDIR=../../include SADIR=../../include csound $file.csd -o $file.wav --logfile=null -W -m0 && sha256sum $(echo $file.wav) | cut -d " " -f 1
```
