
#README 
_SeqOthello_ supports fast coverage query and containment query. 
##Compile
_SeqOthello_ is tested on linux platform with the following system settings. The performance is optimized for Intel CPUs with SSE4.2 support.
```
cmake >= 2.8.4
gcc >= 4.9.1
libz >= 1.2.3
```
To compile the SeqOthello tool chain, decompress the source code and execute the following commands in the source folder.
```
./compile.sh
```
Then you can find the compile resutls in the ```build/bin``` folder. 

##Build
To build the SeqOthello structure, please first prepare the Jellyfish-generated Kmer files. For test purpose, 
   please use the following command to generate a toy set of Kmers in the _kmer_ folder, and then we will build SeqOthello from it.
```
./genExample.sh
```
There are three steps in building SeqOthello: 1. Convert the Kmer files to binary files. 2. Group them together. 3. Execute the Build program. 
You can simply prepare the commands using the wrapper. Simply execute the tool to generate the scripts to build the SeqOthello map, by
```
./genBuildFromJellyfishKmers.sh
```
For all prompted input questions, just press Enter to use the default value for the example data set. Then three scripts will be generated and you can execute them.

```
./ConvertToBinary.sh
./MakeGroup.sh
./BuildSeqOthello.sh
```
And the generated SeqOthello file can be found in _out_ folder.

###Prepare the kmers in parallel
Each line of the generated scripts contains a command to prepare the files. The commands in each of the generated scripts can be executed in parallel. For example, with GNU Parallel, you can run 

``` cat ConvertToBinary.sh | parallel ```

##Query

Containment Query

```build/bin/Query --map-folder=out/ --transcript=test.fa --output=queryresult --lthread=16 --qthread=16> querylog```

Coverage Query

```build/bin/Query --map-folder=out/ --transcript=test.fa --detail --output=queryresult --lthread=16 > querylog```

##OnlineQuery
Use the following command to start a server on the machine, (e.g., on TCP port 3322). The service will run as a deamon.

```build/bin/Query --map-folder=out/ --start-server-port 3322```

In another terminal, run the Client program.
```build/bin/Client --transcript=kmer/test.fa --coverage --output=result --port=3333```


#License 
Please refer to LICENSE.TXT.

#Contact
Please contact us at SeqOthello@googlegroups.com

