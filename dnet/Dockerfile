FROM debian:buster-slim

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch
ENV arch $arch

WORKDIR /root

RUN apt-get update 
RUN apt-get install -y libidn11 libbson-1.0-0 libcurl4-openssl-dev libgnutls30 libmongoc-1.0-0 libsctp1 libssl1.1 libyaml-0-2 iproute2 iptables tcpdump nmap

RUN apt-get install -y iputils-ping dnsutils

COPY open5gs-container-epc_1.0.0_${arch}.deb /root/

RUN dpkg -i /root/open5gs-container-epc*.deb 

RUN [ "/bin/bash", "-c", "mkdir /var/run/open5gs-{epc,mme,sgw,pgw,pcrf,hss}d" ]

COPY confs/*.yaml /etc/open5gs/
COPY freeDiameter/* /etc/open5gs/freeDiameter/
COPY settun.sh /root
