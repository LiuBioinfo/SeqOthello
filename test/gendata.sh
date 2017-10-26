#!/bin/bash
../build/test/datagen -f 182 -k 15300 > genlog
tail -n 1 genlog > testTT.fa
tail -n 1 genlog | cut -c1-100 >> testTT.fa
tail -n 1 genlog | cut -c1-75 >> testTT.fa
tail -n 1 genlog | cut -c1-50 >> testTT.fa
tail -n 1 genlog | cut -c1-25 >> testTT.fa
for j in {1..50}; do rm -rf tmp; for i in A T G C; do echo $i >> tmp; done; echo `sort tmp -R | awk '{print}' ORS=''`; done  | awk '{print}' ORS='' >> testTT.fa
rm 16.Kmer
touch 16.Kmer
for i in *.Kmer; do
    ../build/bin/PreProcess --in=$i --out=$i.bin --k=20;
done
rm -rf raw bin.64
mkdir raw
mkdir bin.64
mv *.Kmer raw/
mv *.bin bin.64/
mv *.bin.xml bin.64/
rm -f flistA flistB flistC flistD
for i in {0..49}; do
    echo F$i.Kmer.bin >> flistA
done
for i in {50..99}; do
    echo F$i.Kmer.bin >> flistB
done

for i in {100..149}; do
    echo F$i.Kmer.bin >> flistC
done

for i in {150..181}; do
    echo F$i.Kmer.bin >> flistD
done

../build/test/datagen -f 100 -k 220 -u 1 > genlog.unique
tail -n 1 genlog.unique > testTT.fau
for i in *.Kmer; do 
   ../build/bin/PreProcess --in=$i --out=$i.bin --k=20;
done
rm -rf raw.unique bin.64.unique
mkdir raw.unique bin.64.unique
mv *.Kmer raw.unique/
mv *.bin bin.64.unique/
mv *.bin.xml bin.64.unique/
rm -rf flistUnique

