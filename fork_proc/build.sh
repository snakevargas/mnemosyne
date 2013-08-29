#!/bin/bash

OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc writer.c util.c -o writer -lrt
else
	gcc writer.c util.c -o writer
fi




