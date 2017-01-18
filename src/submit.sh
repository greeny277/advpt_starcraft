#!/bin/sh

set -e
set -u
#set -o pipefail

zip submission.zip *.cpp *.h *.hpp build.sh forwardSim.sh optimize.sh *.csv CMakeLists.txt

curl 'https://www10.cs.fau.de/advptSC2/forwardSimulation/submission' --form 'groupName=Team Blau' --form terran=on --form protoss=on --form zerg=on --form code=@submission.zip --form comment= --compressed > /dev/null
submission=$(curl 'https://www10.cs.fau.de/advptSC2/submissions/queue' | xmllint --html --xpath 'string((//a[starts-with(@href, "details")])[1]/@href[1])' --nowarning --recover /dev/stdin 2> /dev/null)

while sleep 5 ; do
    res=$(curl "https://www10.cs.fau.de/advptSC2/submissions/$submission")
    if printf "%s" "$res" | grep -F -o "finished" ; then
        break
    fi
done
printf "%s" "$res" | elinks -dump -dump-color-mode 1 -no-references "file:///dev/stdin"
