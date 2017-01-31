#!/bin/sh

./build.sh && build/SC2BuildSimulator dump "$1" > "$1.dot" && dot -T pdf "$1.dot" > "$1.pdf"
