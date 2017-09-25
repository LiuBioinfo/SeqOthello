#!/bin/bash
rm -rf grp
mkdir grp

echo group

../build/bin/Group --flist=flistA --folder=./bin.64/ --output=GrpA.grp > processlog
../build/bin/Group --flist=flistB --folder=./bin.64/ --output=GrpB.grp >> processlog
../build/bin/Group --flist=flistC --folder=./bin.64/ --output=GrpC.grp >> processlog
../build/bin/Group --flist=flistD --folder=./bin.64/ --output=GrpD.grp >> processlog

ls *.grp > grplist
mv *.grp grp/
mv *.grp.xml grp/
#echo Count
#../countkey --flist=grplist --folder=./grp/ -e > keycountlog
rm -rf mapOut
mkdir mapOut
echo Build
../build/bin/Build --flist=grplist --folder=./grp/ --out=mapOut/map --count-only> Buildlog
../build/bin/Build --flist=grplist --folder=./grp/ --out=mapOut/map > Buildlog.tt
echo Query
../build/bin/Query --map=mapOut/map --transcript=testTT.fa --noreverse --detail --output queryresult > querylog
../build/bin/Query --map=mapOut/map --transcript=testTT.fa --noreverse --qthread=4 --output queryresultAgg > querylogAgg

