FROM amazonlinux:latest
RUN yum -y install which unzip aws-cli parallel procps
# RUN yum -y install stress

WORKDIR /tmp
ENV BUILD_DEP 'zlib-devel gcc gcc-c++ automake libtool diffutils pkgconfig gettext-devel glibc-static'
RUN yum makecache fast && yum -y install wget bzip2 file 
RUN yum -y install zlib-devel gcc gcc-c++ automake libtool diffutils pkgconfig gettext-devel glibc-static && \
    cd /tmp && wget https://github.com/samtools/htslib/releases/download/1.8/htslib-1.8.tar.bz2 && tar jxf htslib-1.8.tar.bz2 && cd /tmp/htslib-1.8 && autoreconf -i && ./configure --disable-bz2 --disable-lzma && make -j8 && make install && \
    cd /tmp && wget https://github.com/gmarcais/Jellyfish/releases/download/v2.2.10/jellyfish-2.2.10.tar.gz && tar zxf jellyfish-2.2.10.tar.gz && cd jellyfish-2.2.10 && export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig &&  autoreconf -i && ./configure && make -j8 && make install && \
    yum -y remove zlib-devel gcc gcc-c++ automake libtool diffutils gettext-devel glibc-static && yum -y autoremove && yum -y autoremove

COPY build/bin/* /bin/
COPY build/test/comp /bin/                     
COPY dockerassets/* /usr/local/bin/

# gem RUN gem install yaggo
# COPY Jellyfish /tmp/
# ncurses-devel ncurses libtool file bzip2-devel xz-devel 
# download htslib, yaggo
# RUN ./configure --disable-bz2 --disable-lzma && make -j8 && make install #/usr/local/lib/pkgconfig/htslib.pc

# RUN wget https://github.com/samtools/samtools/releases/download/1.8/samtools-1.8.tar.bz2 && tar jxf samtools-1.8.tar.bz2 
# WORKDIR /tmp/samtools-1.8
# RUN yum install gcc gcc-c++
# RUN ./configure && make -j4 && make install
# WORKDIR /tmp

# USER ec2-user
# ENTRYPOINT /usr/local/bin/fetch_and_run.sh
