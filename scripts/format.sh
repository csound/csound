#!/bin/sh
for i in $@
do
    echo "formatting $i ..."
    clang-format -assume-filename=.clang_format -i $i
done



