#!/bin/sh

race=$(printf "%s" "$1" | awk -F- '{print $3}')
file="$2"
if ! build/SC2BuildSimulator forward "$race" "$file" 2> errors > output ; then
    cat output
    cat errors "$file"
    md5sum "$file"
    exit 1
fi
if ! [ -s output ] ; then
    cat errors "$file"
    exit 1
fi

if [ "$(head -n1 output)" != "{" ] ; then
    cat output
    exit 1
fi

cat output
exit 0
