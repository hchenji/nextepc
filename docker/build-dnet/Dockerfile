FROM debian:stretch

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch=amd64

WORKDIR /root

COPY nextepc-core_0.5.0_${arch}.deb /root
COPY nextepc-epc_0.5.0_${arch}.deb /root

RUN dpkg -i /root/*.deb