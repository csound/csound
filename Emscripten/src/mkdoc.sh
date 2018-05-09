#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
jsdoc $DIR/README.md $DIR/*.js -c $DIR/jsdoc.json -d $1
