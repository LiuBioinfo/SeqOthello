#!/bin/bash
module add cmake/2.8.4
module add gcc/4.9.1
module add boost/1.46.1
sed '/test/d' -i CMakeLists.txt
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER='g++'
make -j4
cd ..
echo 'ADD_SUBDIRECTORY(test)' >> CMakeLists.txt
