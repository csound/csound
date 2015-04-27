#!/bin/sh
export CS_VERSION="6.05.0"
export RELEASE_DIR=csound-emscripten-${CS_VERSION}

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cp -R examples/* ${RELEASE_DIR}/

zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR}
