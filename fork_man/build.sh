#!/bin/bash

STD_FLAGS="-std=c99"
#STD_FLAGS="-std=gnu99" # DON'T USE GNU99! IT DOES WRONG THINGS! GOOGLE IT
OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc $STD_FLAGS writer.c util.c -o writer -lrt -D_POSIX_C_SOURCE=200809L
else
	gcc $STD_FLAGS writer.c util.c -o writer
fi

