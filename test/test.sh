#!/bin/bash
rm -rf grp
mkdir grp

echo preprocess

../preprocess --flist=flist --folder=./bin.64.32/ --output=1.50.grp --from=1 --to=50 > processlog
../preprocess --flist=flist --folder=./bin.64.32/ --output=51.102.grp --from=51 --to=102 >> processlog

ls *.grp > grplist
mv *.grp grp/
echo Count
../countkey --flist=grplist --folder=./grp/ -e > keycountlog
rm -rf mapOut
mkdir mapOut
echo Build
../Build --fcnt 102 --flist=grplist --folder=./grp/ --out=mapOut/map > Buildlog
echo Query
../Query --map=mapOut/map --transcript=testTT.fa --noreverse --detail --output queryresult > querylog
../Query --map=mapOut/map --transcript=testTT.fa --noreverse --qthread=4 --output queryresultAgg > querylogAgg

