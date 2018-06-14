#!/bin/bash
# you need to update Jellyfish in the Jelyfish folder and run ./configure in it.
# then make here.
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER='g++'
make -j4
