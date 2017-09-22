#!/bin/bash
../build/test/datagen -f 102 -k 15000 > genlog
tail -n 1 genlog > testTT.fa
tail -n 1 genlog | cut -c1-100 >> testTT.fa
tail -n 1 genlog | cut -c1-75 >> testTT.fa
tail -n 1 genlog | cut -c1-50 >> testTT.fa
tail -n 1 genlog | cut -c1-25 >> testTT.fa
for i in *.Kmer; do
    ../build/bin/PreProcess --in=$i --out=$i.bin --k=20;
done
rm -rf raw bin.64
mkdir raw
mkdir bin.64
mv *.Kmer raw/
mv *.bin bin.64/
mv *.bin.xml bin.64/
rm -f flistA flistB
for i in {1..50}; do
    echo F$i.Kmer.bin >> flistA
done
for i in {51..102}; do
    echo F$i.Kmer.bin >> flistB
done

