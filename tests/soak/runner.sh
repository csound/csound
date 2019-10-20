#!/usr/bin/env bash

export CSOUND_BIN=$1
export OPCODE6DIR64=$2

if [ -n "$TRAVIS_BUILD_DIR" ]; then
    # dependency needed for the soak-tests
    # (installing directly from pypi doesn't work when python3 isn't compiled with openssl)

    git clone https://github.com/wolever/parameterized.git

    cd ./parameterized

    git checkout 85222cfd5f93ba026a74a4726ae98d6f89fff502

    cd ../

    python3.7 -m venv "$TRAVIS_BUILD_DIR/venv"

    "$TRAVIS_BUILD_DIR/venv"/bin/pip3.7 install ./parameterized

    cd "$TRAVIS_BUILD_DIR/tests/soak"

    # stage 1, prepare includes
    "$TRAVIS_BUILD_DIR/venv"/bin/python test_prepare.py

    # stage 2, run the checksum test
    "$TRAVIS_BUILD_DIR/venv"/bin/python -m unittest -v test_checksum

else # local (expects python3 and pip3)
    if [[ "$(python3 -V)" =~ "Python 3" ]]; then
        pip3 install -r requirements.txt
        python3 test_prepare.py
        python3 -m unittest -v test_checksum
    else
        echo "error: python3 was not found"
        exit 1
    fi
fi
