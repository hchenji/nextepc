#!/bin/sh

if ! grep "pgwtun" /proc/net/dev > /dev/null; then
    ip tuntap add name pgwtun mode tun
fi
ip addr del 172.30.10.1/24 dev pgwtun 2> /dev/null
ip addr add 172.30.10.1/24 dev pgwtun
ip addr del ca10::1/64 dev pgwtun 2> /dev/null
ip addr add ca10::1/64 dev pgwtun
ip link set pgwtun up
