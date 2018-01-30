#!/bin/bash


#Create the group list for the binary files.
head -n 5 experiments_list.10.txt | sed "s/$/\.bin/g" \
  > tmp/binary_list.part00

tail -n 5 experiments_list.10.txt | sed "s/$/\.bin/g" \
  > tmp/binary_list.part01
#Build occurrence map for the group.
mkdir -p tmp/grp

../build/bin/Group \
--flist=tmp/binary_list.part00 \
--folder=tmp/kmer_bins/ \
--output=tmp/grp/Grp_00

../build/bin/Group \
--flist=tmp/binary_list.part01 \
--folder=tmp/kmer_bins/ \
--output=tmp/grp/Grp_01

echo Grp_00 > tmp/grp_list
echo Grp_01 >> tmp/grp_list

