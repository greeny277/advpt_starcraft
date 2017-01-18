#!/bin/sh

set -x

cd build
file="$2"
[ "$(printf '%s' \"$file\" | cut -c 1)" = "/" ] || file="../$file"
exec ./SC2BuildSimulator forward "$file"
