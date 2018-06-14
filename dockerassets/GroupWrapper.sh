#!/bin/bash
GrpName=`echo $@ | md5sum | cut -c1-9`
echo $GrpName
BinFiles=$@
Binlist=${GrpName}.binlist
echo $BinFiles

for  i in $BinFiles; do 
    if [[ "$i" == *bin ]]; then
        echo $i;
    fi
done > ${Binlist}
cat ${Binlist}

echo Group --flist=${Binlist} --output=${GrpName}.grp
Group --flist=${Binlist} --output=${GrpName}.grp
