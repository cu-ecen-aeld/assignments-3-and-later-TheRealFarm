#!/bin/bash

if [ "$#" -ne 2 ]; then
	echo "Invalid number of arguments"
	echo "Usage: $0 <writefile> <writestr>"
	exit 1
fi

writefile=$1
writestr=$2
writedir=$(dirname "$writefile")

if [ ! -d "$writedir" ]; then
	mkdir -p "$writedir"
fi

if touch "$writefile"; then
	echo "$writestr" > "$writefile"
else
	echo "Error creating file"
fi

