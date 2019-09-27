FROM debian:buster-slim

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch=amd64

WORKDIR /root

RUN apt-get update 
RUN apt-get install -y libidn11 libbson-1.0-0 libcurl4-openssl-dev libgnutls30 libmongoc-1.0-0 libsctp1 libssl1.1 libyaml-0-2 iproute2 iptables tcpdump nmap

RUN apt-get install -y iputils-ping dnsutils

COPY libogs*1.2.1_${arch}.deb /root/
COPY nextepc-container-epc*0.5.2_${arch}.deb /root/

RUN dpkg -i /root/libogs*.deb && dpkg -i /root/nextepc-container-epc*.deb 

RUN [ "/bin/bash", "-c", "mkdir /var/run/nextepc-{epc,mme,sgw,pgw,pcrf,hss}d" ]

COPY nextepc.allin1.conf /etc/nextepc/nextepc.conf
COPY confs/*.conf /etc/nextepc/
COPY freeDiameter/* /etc/nextepc/freeDiameter/
COPY settun.sh /root
