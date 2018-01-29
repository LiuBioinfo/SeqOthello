# Manual
__SeqOthello__ toolchain includes the following modules:

* ``PreProcess`` converts _k_-mer files to __SeqOthello__
64 bit binary format.

* ``Group`` combines a subset of the 64 bit binary kmer files into
group.

* ``Build`` creates __SeqOthello__ mapping between the entire set
of _k_-mers and their experiment ids.

* ``


```
PreProcess {OPTIONS}

  Convert a Jellyfish output file to a sorted binary file.

OPTIONS:

    -h, --help                        Display this help menu
    --in=[string]                     filename for the input kmer file
    --out=[string]                    filename for the output binary kmer file
    --k=[integer]                     k, length of kmer
    --cutoff=[integer]                cutoff, minimal expression value for
                                      kmer to be included into the file.
    --histogram                       get histogram
```


```
Group {OPTIONS}

  Preprocess binary files to grouped files.
  Each line of the file must contain exactly one file name, e.g, xxxx.bin
  The file should be in 64bit kmer format, the xml file must present in the
  same folder. xxxx.bin.xml

OPTIONS:

    -h, --help                        Display this help menu
    --flist=[string]                  a file containing the filenames
    --folder=[string]                 where to find this file
    --output=[string]                 output file
    --limit=[integer]                 stop after getting this number of kmers.
                                      Note: for test small data set only.
    --group                           Create a group using some group files

```


```
Build {OPTIONS}

  Build SeqOthello!

OPTIONS:

    -h, --help                        Display this help menu
    --flist=[string]                  a file containing the filenames of Grp
                                      files, these Grp files should be created
                                      by the Preprocess tool. Each line should
                                      contain one file name.
    --folder=[string]                 where to find these Grp files. i.e. , a
                                      path that contains the Grp files.
    --out-folder=[string]             a folder to put the generated SeqOthello
                                      map.
    --estimate-limit=[int]            read this number of Kmers to estimate
                                      the distribution.
    --count-only                      only count the keys and the histogram,
                                      do not build the seqOthello.

```


```
Query {OPTIONS}

  Query SeqOthello!

OPTIONS:

    -h, --help                        Display this help menu
    --map-folder=[string]             the path contains SeqOthello mapping
                                      file.
    --transcript=[string]             file containing transcripts
    --output=[string]                 where to put the results
    --detail                          Show the detailed query results for the
                                      transcripts
    --noreverse                       do not use reverse complement
    --qthread=[int]                   how many threads to use for query,
                                      default = 1
    --start-server-port=[int]         start a SeqOthello Server at port
    --print-kmers-index=[int]         printout kmers that matches a sample
                                      with index.


```
