FROM amazonlinux:latest
RUN yum -y install which unzip aws-cli parallel procps
RUN yum -y install stress
COPY build/bin/* /bin/
COPY build/test/comp /bin/                     
COPY dockerassets/* /usr/local/bin/

WORKDIR /tmp
# USER ec2-user
# ENTRYPOINT /usr/local/bin/fetch_and_run.sh
