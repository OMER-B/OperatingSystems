#!/bin/bash
if [[ $# > 1 ]]; then
	echo "error: only one argument is allowed"
	exit 1
fi
if [[ ! -e $1 ]]; then
	echo "error: there is no such file"
	exit 1
fi
if [[ ! -d ./safe_rm_dir ]]; then
	mkdir -p ./safe_rm_dir;
fi
cp $1 ./safe_rm_dir
rm $1

echo "done!"