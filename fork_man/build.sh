#!/bin/bash

SOURCE_FILES="writer.c util.c child.c"
STD_FLAGS="-std=c99"
#STD_FLAGS="-std=gnu99" # DON'T USE GNU99! IT DOES WRONG THINGS! GOOGLE IT
OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc $STD_FLAGS $SOURCE_FILES -o writer -lrt -D_POSIX_C_SOURCE=200809L
else
	gcc $STD_FLAGS $SOURCE_FILES -o writer
fi

