#!/bin/bash
bname=`basename $1`;
aws s3 cp $1 .
tar zcvf ${bname}.tar.gz ${bname}
aws s3 cp ${bname}.tar.gz $2
rm -rf ${bname}*

