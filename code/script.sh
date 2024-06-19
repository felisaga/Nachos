#! /bin/bash

# Check if the number of arguments is correct
if [ $# -ne 2 ]; then
    echo "Usage: $0 <arg1> <arg2>"
    exit 1
fi

arg1=$1
arg2=$2

make >/dev/null

(cd ./userland; make >/dev/null)

(cd ./vmem; ./nachos -m $arg1 -x ../userland/$arg2)