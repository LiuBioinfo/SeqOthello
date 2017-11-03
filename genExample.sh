#!/bin/bash
mkdir -p kmer
cd kmer
../build/test/datagen -f 182 -k 15300 > genlog
echo '>xxxxxxxxx|yyyyyyyyyy' > test.fa
tail -n 1 genlog >> test.fa
ls -m1 *.Kmer > flist
cd ..
