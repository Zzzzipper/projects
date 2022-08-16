#!/bin/sh

cd $1
file="$1/$2/stamp.h"
echo "#pragma once" > $file
echo "" >> $file
A="#define GIT_COMMIT      "
B="\"$(git rev-parse --short=8 HEAD)\""
echo "$A $B" >> $file
C="#define GIT_COMMIT_TIME "
D="\"$(git show -s --format=\%ci $(git rev-parse --short=8 HEAD) | tr -d '\n')\""
echo "$C $D" >> $file
E="#define COMPILE_TIME    "
F="$(date +'"%Y-%m-%d %H:%M:%S %z"')"
echo "$E $F" >> $file
cd -
