#!/bin/bash

if [ "$#" -ne 2 ]; then
	echo "Incorrect number of arguments."
	echo "Usage: $0 <filesdir> <searchstr>"
	exit 1
fi

filesdir="$1"
searchstr="$2"

if [ ! -d "$filesdir" ]; then
	echo "$1 is not a directory"
	exit 1
fi

files=$(find "$filesdir" -type f -exec grep -l "$searchstr" {} +)

file_count=0
total_line_count=0

for file in $files; do
	((file_count++))

	line_count=$(grep -c "$searchstr" "$file")

	((total_line_count += line_count))
done

echo "The number of files are $file_count and the number of matching lines are $total_line_count"
