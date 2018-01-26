#!/bin/bash
mkdir -p example
echo "Create example kmer files ..."
./genExample.sh
echo "Create scripts ..."
echo "You can accept all the default settings ..."
./genBuildFromJellyfishKmers.sh
