#!/bin/bash
KMER_DIR=example/kmer
CURR_DIR=`pwd`
mkdir -p $KMER_DIR 
cd $KMER_DIR
$CURR_DIR/build/test/datagen -f 182 -k 15300 > genlog
echo '>xxxxxxxxx|yyyyyyyyyy' > test.fa
tail -n 1 genlog >> test.fa
ls -m1 *.Kmer > flist
cd $CURR_DIR
