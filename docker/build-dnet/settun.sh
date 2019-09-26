#!/bin/sh

mkdir -p /dev/net
mknod /dev/net/tun c 10 200

if ! grep "pgwtun" /proc/net/dev > /dev/null; then
    ip tuntap add name pgwtun mode tun
fi
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
iptables -I INPUT -i pgwtun -j ACCEPT
iptables -A FORWARD -i pgwtun -o eth0 -j ACCEPT
iptables -A FORWARD -i eth0 -o pgwtun -j ACCEPT
ip addr del $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev pgwtun 2> /dev/null
ip addr add $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev pgwtun
# ip addr del $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev pgwtun 2> /dev/null
# ip addr add $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev pgwtun
ip link set pgwtun up
