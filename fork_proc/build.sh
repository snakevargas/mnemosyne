#!/bin/bash

OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc -std=c99 writer.c util.c -o writer -lrt
else
	gcc -std=c99 writer.c util.c -o writer
fi




