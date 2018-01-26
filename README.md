# README 
_SeqOthello_ supports fast coverage query and containment query. 

## Compile
### System Requirements
_SeqOthello_ is tested on linux platform with the following system settings. The performance is optimized for Intel CPUs with SSE4.2 support.
```
cmake >= 2.8.4
gcc >= 4.9.1
libz >= 1.2.3
```
These tools can be installed on Ubuntu using apt
```
 sudo apt install cmake build-essential zlib1g-dev
```
For Fedora please use yum
```
 sudo yum install gcc-c++ cmake zlib-dev
```
_SeqOthello_ is also tested on mac OS 10.12. Please install cmake using [brew](https://brew.sh/)
```
brew install cmake 
```

To compile the SeqOthello tool chain, execute the following commands in the source folder.
```
./compile.sh
```
Then you can find the compile resutls in the ```build/bin``` folder. 

## Build
To build the SeqOthello structure, please first prepare the Jellyfish-generated Kmer files. For test purpose, 
 please use the following command to generate an example set of Kmers that consists of 182 samples in the _kmer_ folder, and then we will build SeqOthello from it.
```
./genExample.sh
```
There are three steps in building SeqOthello: 1. Convert the Kmer files to binary files. 2. Group them together. 3. Execute the Build program. 
You can simply prepare the commands using the wrapper. Simply execute the tool to generate the scripts to build the SeqOthello map, by
```
./genBuildFromJellyfishKmers.sh
```
For all prompted input questions, just press Enter to use the default value for the example data set. Then three scripts will be generated. To construct the SeqOthello map, just execute the three scripts one by one. 

```
./ConvertToBinary.sh && ./MakeGroup.sh && ./BuildSeqOthello.sh
```
And the generated SeqOthello file can be found in _out_ folder.

### Prepare the Kmers in parallel
Each line of the generated scripts contains a command to prepare the files. The commands in each of the generated scripts can be executed in parallel. For example, with GNU Parallel, you can run 

``` cat ConvertToBinary.sh | parallel ```

## Query
To execute the query please use the _Query_ command. For test purpose, we prepared a set of transcripts in _test/testTT.fa_. To execute the query, run the folloiwng command in the source folder.

Containment Query
```build/bin/Query --map-folder=out/ --transcript=kmer/test.fa --output=queryResultContainment --qthread=8 > querylog```
Coverage Query
```build/bin/Query --map-folder=out/ --transcript=kmer/test.fa --detail --output=queryResultCoverage --qthread=8 > querylog```

## Online Query (Client / Server Mode)
Use the following command to start a server on the machine, (e.g., on TCP port 3322). The service will run as a deamon.

```build/bin/Query --map-folder=out/ --start-server-port 3322```

In another terminal, run the Client program.
```build/bin/Client --transcript=kmer/test.fa --coverage --output=result --port=3322```

# License 
Please refer to LICENSE.TXT.

# Contact
Please contact us at SeqOthello@googlegroups.com

# Known Issues
1. Usually, on Linux systems, there is a limit on the number of files that can be opened simutaiously. The _Build_ program of SeqOthello may hit such limit. To edit this limit, set ulimit to larger numbers. e.g, 
```
ulimit -nS 4096
```
