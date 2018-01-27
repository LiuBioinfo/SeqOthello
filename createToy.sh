#!/bin/bash
mkdir -p example
echo "Create example set of kmer files ..."
./genExample.sh
echo "Create scripts ..."
echo "For the example set of kmer files, please use the default option for all the following settings ..."
./genBuildFromJellyfishKmers.sh
