#!/bin/bash
ulimit -c 100000000
while [ true ] ; do
    src/tmwserv

    for CORE in `ls core*` ; do
        gdb -batch src/tmwserv $CORE > backtrace-`date +%Y%m%d-%H%M%S`
        mv $CORE usedcore
    done
done
