#!/bin/bash

#Create the group list for the binary files.
ls -p tmp/kmer_bins/experiment_a*.bin | xargs -n 1 basename > tmp/binary_list.parta

#Build occurrence map for the group.

../build/bin/Group \
--flist=tmp/binary_list.parta \
--folder=tmp/kmer_bins/ \
--output=tmp/grp/Grp_a

cp tmp/grp_list tmp/grp_list_a
echo Grp_a >> tmp/grp_list_a

