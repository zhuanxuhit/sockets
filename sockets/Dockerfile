# Version: 0.0.1
FROM ubuntu:17.04
MAINTAINER zhuanxu "893051481@qq.com"

WORKDIR /opt
VOLUME ["/opt"]
RUN apt-get update && apt-get install -y \
		cmake \
		build-essential \
		man net-tools \ 
		libboost1.63-dev \
		manpages-posix-dev
#RUN apt-get autoclean
#RUN apt-get clean all
#RUN apt-get autoremove
