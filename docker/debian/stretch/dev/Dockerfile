ARG dist=ubuntu
ARG tag=bionic
ARG username=wsrg
FROM ${username}/${dist}-${tag}-nextepc-base

MAINTAINER Sukchan Lee <acetcom@gmail.com>
MAINTAINER Harsha Chenji <chenji@ohio.edu>

RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends \
        git-buildpackage \
        debhelper \
        devscripts \
        osc \
        dput \
        dh-autoreconf \
        dh-systemd \
        manpages-dev \
        fakeroot \
        cscope \
        vim \
        sudo \
        iputils-ping \
        wireshark \
        net-tools && \
    apt-get clean

COPY setup.sh /root

ARG username=wsrg
RUN useradd -m --uid=1000 ${username} && \
    echo "${username} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${username} && \
    chmod 0440 /etc/sudoers.d/${username}

WORKDIR /home/${username}
