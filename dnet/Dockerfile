FROM ubuntu:18.04

MAINTAINER Harsha Chenji <chenji@ohio.edu>

ARG arch
ENV arch $arch

WORKDIR /root

RUN apt-get update 
RUN apt-get install -y wget libidn11 libbson-1.0-0 libcurl4-openssl-dev libgnutls30 libmongoc-1.0-0 libssl1.1 libyaml-0-2 iproute2 iptables

RUN apt-get install -y iputils-ping dnsutils tcpdump nmap

# RUN apt-get install -y wget unzip cmake build-essential cmake flex bison libgnutls28-dev libgcrypt-dev libssl-dev libidn11-dev libmongoc-dev libbson-dev libyaml-dev git
# RUN apt-get install -y python3-pip ninja-build

# RUN wget https://github.com/hchenji/open5gs/archive/nextepc-consul.zip
# RUN unzip nextepc-consul.zip
# WORKDIR /root/open5gs-nextepc-consul

# COPY ../../open5gs /root/
# WORKDIR /root/open5gs-nextepc-consul

# RUN pip3 install meson
# RUN meson build
# RUN ninja -C build
# WORKDIR /root/open5gs-nextepc-consul/build
# RUN ninja install
# WORKDIR /root/
# RUN rm -rf open5gs*

COPY open5gs-container-epc_1.0.0_amd64.deb /root/
COPY *sctp*_amd64.deb /root/
RUN dpkg -i /root/*.deb 

RUN [ "/bin/bash", "-c", "mkdir /var/run/open5gs-{epc,mme,sgw,pgw,pcrf,hss}d" ]

COPY confs/*.yaml /etc/open5gs/
COPY freeDiameter/* /etc/open5gs/freeDiameter/
COPY settun.sh /root
