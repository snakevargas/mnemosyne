#!/bin/bash

#STD_FLAGS="-std=c99"
STD_FLAGS="-std=gnu99"
OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc $STD_FLAGS writer.c util.c -o writer -lrt
else
	gcc $STD_FLAGS writer.c util.c -o writer
fi

