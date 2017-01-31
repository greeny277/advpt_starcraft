#!/bin/sh

./build.sh && build/SC2BuildSimulator dump "$1" > "$1.dot" && dot -T ps "$1.dot" > "$1.ps"
