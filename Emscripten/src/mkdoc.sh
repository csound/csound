#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR
jsdoc $DIR/README.md -c $DIR/jsdoc.json -d $1
