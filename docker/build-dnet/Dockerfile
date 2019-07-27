FROM debian:stretch

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch=amd64

WORKDIR /root

RUN apt-get update && apt-get install -y libbson-1.0-0 libcurl3 libgnutls30 libmongoc-1.0-0 libsctp1 libssl1.1 libyaml-0-2

COPY libogs*_${arch}.deb /root/
COPY nextepc-container-epc*_${arch}.deb /root/

RUN dpkg -i /root/libogs*.deb && dpkg -i /root/nextepc-container-epc*.deb 

RUN [ "/bin/bash", "-c", "mkdir /var/run/nextepc-{epc,mme,sgw,pgw,pcrf,hss}d" ]

COPY nextepc.conf /etc/nextepc/nextepc.conf
COPY setup.sh /root
