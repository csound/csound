#!/bin/sh
for i in $@
do
    echo "formatting $i ..."
    clang-format -assume-filename=.clang-format -i $i
done


