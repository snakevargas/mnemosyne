#!/bin/bash

OS_NAME=`uname`
if [ "$OS_NAME" == "Linux" ]; then
	gcc writer.c util.c -o writer -lrt
	gcc reader.c util.c -o reader -lrt
else
	gcc writer.c util.c -o writer
	gcc reader.c util.c -o reader
fi




