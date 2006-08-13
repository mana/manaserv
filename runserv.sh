#!/bin/bash
ulimit -c 100000000
while [ true ] ; do
    src/tmwserv --verbosity 2

    for CORE in `ls core*` ; do
        gdb -batch src/tmwserv $CORE > backtrace-`date +%Y%m%d-%H%M%S`
        mv $CORE usedcore
    done
done
