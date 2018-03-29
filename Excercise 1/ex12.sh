#!/bin/bash
#omer barak
#313264053
#barakom
for file in "$1"/*
do
	if [[ -d $file ]]; then
		echo "$file is a directory"
	elif [[ -f $file ]]; then
		echo "$file is a file" 
	fi
done