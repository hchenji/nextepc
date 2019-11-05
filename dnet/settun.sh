#!/bin/sh

mkdir -p /dev/net
mknod /dev/net/tun c 10 200

if ! grep "ogstun" /proc/net/dev > /dev/null; then
    ip tuntap add name ogstun mode tun
fi
iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
iptables -I INPUT -i ogstun -j ACCEPT
iptables -A FORWARD -i ogstun -o eth0 -j ACCEPT
iptables -A FORWARD -i eth0 -o ogstun -j ACCEPT
ip addr del $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev ogstun 2> /dev/null
ip addr add $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev ogstun
# ip addr del $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev ogstun 2> /dev/null
# ip addr add $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev ogstun
ip link set ogstun up
