#!/bin/sh
export CS_VERSION="6.13.0"
export RELEASE_DIR=Csound${CS_VERSION}-Web

#remove backup files ending with ~
find . -name "*~" -exec rm {} \;

rm -rf $RELEASE_DIR
mkdir $RELEASE_DIR
cp -R examples ${RELEASE_DIR}/
rm ${RELEASE_DIR}/examples/update-scripts.sh
# documentation
./src/mkdoc.sh ${RELEASE_DIR}/docs

cp README-RELEASE.md ${RELEASE_DIR}/README.md
zip -r ${RELEASE_DIR}.zip ${RELEASE_DIR}
