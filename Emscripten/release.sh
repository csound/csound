#!/bin/sh
export CS_VERSION="6.11.0-beta"
export RELEASE_DIR=csound-web-${CS_VERSION}

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cp -R examples/* ${RELEASE_DIR}/
# documentation
jsdoc -d ${RELEASE_DIR}/docs src/csound.js src/CsoundObj.js src/CsoundScriptProcessorNode.js src/CsoundNode.js src/README.md

zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR}
