ARG dist=alpine
ARG tag=3.10
FROM ${dist}:${tag}

MAINTAINER Sukchan Lee <acetcom@gmail.com>
MAINTAINER Harsha Chenji <chenji@ohio.edu>

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        libtool \
        gcc \
        flex \
        bison \
        git \
        libsctp-dev \
        libgnutls28-dev \
        libgcrypt-dev \
        libssl-dev \
        libidn11-dev \
        libmongoc-dev \
        libbson-dev \
        libyaml-dev \
        libcurl4-openssl-dev \
        build-essential \
        iproute2 \
        ca-certificates \
        netbase \
        pkg-config && \
    apt-get clean
