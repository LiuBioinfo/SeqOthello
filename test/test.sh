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
../build/bin/Build --flist=grplist --folder=./grp/ --out-folder=mapOut/ --count-only> Buildlog
../build/bin/Build --flist=grplist --folder=./grp/ --out-folder=mapOut/ > Buildlog.tt
echo Query
../build/bin/Query --map-folder=mapOut/ --transcript=testTT.fa --detail --output queryresult > querylog
../build/bin/Query --map-folder=mapOut/ --transcript=testTT.fa --qthread=4 --output queryresultAgg > querylogAgg
# rm ./mapOut/map.L2.7

../build/bin/Group --flist=flistA --folder=./bin.64.unique/ --output=GrpAU.grp > processlogU
../build/bin/Group --flist=flistB --folder=./bin.64.unique/ --output=GrpBU.grp >> processlogU
ls *U.grp > grplistU
rm -rf grpU
mkdir grpU
mv *U.grp grpU/
mv *U.grp.xml grpU/

rm -rf mapOutU
mkdir mapOutU
echo Build
../build/bin/Build --flist=grplistU --folder=./grpU/ --out-folder=mapOutU/ > Buildlog.ttU

echo Query
../build/bin/Query --map-folder=mapOutU/ --transcript=testTT.fau --detail --output queryresultU > querylogU
../build/bin/Query --map-folder=mapOutU/ --transcript=testTT.fau --qthread=4 --output queryresultAggU > querylogAggU

# ../build/bin/Query --map-folder=mapOut/ --start-server-port 3322 
