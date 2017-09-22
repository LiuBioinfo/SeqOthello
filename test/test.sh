#!/bin/bash
rm -rf grp
mkdir grp

echo group

../build/bin/Group --flist=flistA --folder=./bin.64/ --output=GrpA.grp > processlog
../build/bin/Group --flist=flistB --folder=./bin.64/ --output=GrpB.grp >> processlog

#ls *.grp > grplist
#mv *.grp grp/
#echo Count
#../countkey --flist=grplist --folder=./grp/ -e > keycountlog
#rm -rf mapOut
#mkdir mapOut
#echo Build
#../Build --fcnt 102 --flist=grplist --folder=./grp/ --out=mapOut/map > Buildlog
#echo Query
#../Query --map=mapOut/map --transcript=testTT.fa --noreverse --detail --output queryresult > querylog
#../Query --map=mapOut/map --transcript=testTT.fa --noreverse --qthread=4 --output queryresultAgg > querylogAgg

