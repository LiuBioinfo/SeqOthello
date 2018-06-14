#!/bin/bash
GrpFiles=$@

for  i in $GrpFiles; do 
    if [[ "$i" == *grp ]]; then
        echo $i;
    fi
done > Grplist

mkdir -p map
Build --flist=Grplist --out-folder=map/
