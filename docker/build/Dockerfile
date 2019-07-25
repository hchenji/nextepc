ARG dist=ubuntu
ARG tag=bionic
ARG username=wsrg
FROM ${username}/${dist}-${tag}-nextepc-base

MAINTAINER Sukchan Lee <acetcom@gmail.com>
MAINTAINER Harsha Chenji <chenji@ohio.edu>


WORKDIR /root
COPY setup.sh /root

ARG USER=hchenji
ARG REPO=nextepc
ARG BRANCH=nextepc-consul
RUN	git clone --recursive https://github.com/$USER/$REPO
RUN	cd nextepc && git checkout nextepc-consul && git submodule update
ADD https://api.github.com/repos/$USER/$REPO/git/refs/heads/$BRANCH /root/nextepc-ver.json

RUN	cd nextepc && autoreconf -fiv && ./configure \
        --prefix=/usr \
        --sysconfdir=/etc \
        --localstatedir=/var && \
    make -j `nproc` install

COPY nextepc.conf /etc/nextepc/nextepc.conf