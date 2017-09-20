#!/bin/bash
make -C ../
./clean.sh
./datagen -f 102 -k 15000 > genlog
tail -n 1 genlog > testTT.fa
tail -n 1 genlog | cut -c1-100 >> testTT.fa
tail -n 1 genlog | cut -c1-75 >> testTT.fa
tail -n 1 genlog | cut -c1-50 >> testTT.fa
tail -n 1 genlog | cut -c1-25 >> testTT.fa
for i in *.Kmer; do
    ../bin/Preprocess --in=$i --out=$i.bin --k=20;
done
mkdir raw
mkdir bin.64
mv *.Kmer raw/
mv *.bin bin.64/
rm -f flist
for i in {1..102}; do
    echo F$i.Kmer.bin >> flist
done

