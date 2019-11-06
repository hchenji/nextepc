# builds the libsctp version using deb

FROM ubuntu:18.04

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch
ENV arch $arch

WORKDIR /root

RUN apt-get update 
RUN apt-get install -y wget libidn11 libbson-1.0-0 libcurl4-openssl-dev libgnutls30 libmongoc-1.0-0 libssl1.1 libyaml-0-2 iproute2 iptables

RUN apt-get install -y iputils-ping dnsutils tcpdump nmap libsctp1

COPY open5gs-container-epc_1.0.0_amd64.deb /root/
RUN dpkg -i /root/*.deb 

RUN [ "/bin/bash", "-c", "mkdir /var/run/open5gs-{epc,mme,sgw,pgw,pcrf,hss}d" ]

COPY confs/*.yaml /etc/open5gs/
COPY freeDiameter/* /etc/open5gs/freeDiameter/
COPY settun.sh /root
