FROM amazonlinux:latest
RUN yum -y install which unzip aws-cli parallel procps
ADD convert_kmer_to_bin_S3.sh /usr/local/bin/convert_kmer_to_bin_S3.sh
RUN yum -y install stress
ADD build_grps.sh /usr/local/bin/build_grps.sh
ADD fetch_and_run.sh /usr/local/bin/fetch_and_run.sh
ADD echo.sh /usr/local/bin/echo.sh
ADD build_seqothello.sh /usr/local/bin/build_seqothello.sh
ADD query.sh /usr/local/bin/query.sh
ADD makexml.sh /usr/local/bin/makexml.sh
ADD runmeasure.py /usr/local/bin/runmeasure.py
ADD compress_aws_kmer.sh /usr/local/bin/compress_aws_kmer.sh
ADD compresult.sh /usr/local/bin/compresult.sh
ADD build_grps_grps.sh /usr/local/bin/build_grps_grps.sh
COPY build/bin/* /bin/
COPY build/test/comp /bin/                     

WORKDIR /tmp
# USER ec2-user
ENTRYPOINT /usr/local/bin/fetch_and_run.sh
