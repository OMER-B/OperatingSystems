#!/bin/bash
BALANCE=$(grep "$1" "$2" | awk '{s+=$3} END { print s}')
if grep "$1" "$2"; then
echo "Total balance: ${BALANCE}"
fi