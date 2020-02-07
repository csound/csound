# Csound Testing

The csound6/tests folder contains various forms of tests for Csound. The tests are generally written to aid development, testing that new code functions as expected and that they handle errors correctly.  The following describes the various forms of tests.

## tests/commandline
This folder contains CSD's that get run by a python test runner(test.py).  These are generally used for testing the compiler and can be considered integration tests. These can be run from the CMake generated build file, i.e. "make csdtests". 

Note: Some tests in the tests/commandline folder are not added to the test suite.  These are generally ones that people have contributed to illustrate a bug, and were used during debugging.  It is useful to have these around and ideally we will extend the test suite to do runtime testing as well as compiler testing.


## tests/c 

This folder contains unit tests written in C, using the CUnit library.  Currently there are tests for various parts of the compiler and some API methods. These tests serve to help ensure we didn't break something moving forward, and also act as a documentation on how functions are used.  These can be run from the CMake generated tests using "make test" or calling "ctest". 

## tests/python 

This folder is intended for tests of the Csound API using Python.  The idea is that it would be useful to test from a host language to make sure our assumptions about the C API still work from a host language.

## tests/regression

A collection of previous bugs which should remain fixed

## tests/soak

A large test of most examples from the manual.  The scripts also check for changes sice previous run, using MD5sum for audio output and diff for text

