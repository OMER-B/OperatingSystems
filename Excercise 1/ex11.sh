#!/bin/bash
#omer barak
#313264053
#barakom
if [ -f "$1"/*.txt ]; then
	COUNT=$(find "$1"/*.txt | wc -l)
else
	COUNT=0
fi
echo "Number of files in the directory that end with .txt is: $COUNT"