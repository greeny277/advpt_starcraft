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
md5=$(md5sum output)
if [ "$md5" = "6ad8e170d8a992e41e116e40cc72b743  output" ] ; then
    cat output
    exit 0
fi
if [ "$md5" != "3615fbc06fc359188b870c4b6d85d5fa  output" ] &&
   [ "$md5" != "3de8c4ed6a83599573766ac7279de8f5  output" ] &&
   [ "$md5" != "dae9f9d9ee2fcb38762070ad041f54c2  output" ] &&
   [ "$md5" != "201b304baf8f656b9b5a0071b156c711  output" ] &&
   [ "$md5" != "13f3c677dfdbff597bdff18c3d963ae7  output" ] &&
   [ "$md5" != "940cac5bb07b52e566b8daf278e17ee8  output" ] &&
   [ "$md5" != "7214ff14c7fd3fff410404558c8e3008  output" ] &&
   [ "$md5" != "68b0d8c6b198601f853761eab421d9e6  output" ] &&
   [ "$md5" != "547ed613e1a1d9491bcc02d0da5af036  output" ] &&
   [ "$md5" != "23bf0241ab80ddd02ed31f7ec26ea3b6  output" ] &&
   [ "$md5" != "dc4a80c057f9e337dbe1c6f0095989e9  output" ] &&
   [ "$md5" != "c6548e1b64d7492d2c47962ba3ed967f  output" ] &&
   [ "$md5" != "6b7a614590623bc1e8d05f29ecffbc6d  output" ] &&
   [ "$md5" != "b45804580227e02fbcbef6d3db0be2d5  output" ] &&
   [ "$md5" != "6ad8e170d8a992e41e116e40cc72b743  output" ] &&
   [ "$md5" != "6ad8e170d8a992e41e116e40cc72b743  output" ] &&
   [ "$md5" != "21d62767acb73c1bd807bbb52c793be5  output" ]; then
    cat output
    md5sum output
    exit 1
fi

cat output
exit 0
