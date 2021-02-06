#!/bin/bash


dd if=/dev/zero bs=1 count=$(expr $1 - $(ls -l | grep $2 | cut -d ' ' -f 5)) >> $2

