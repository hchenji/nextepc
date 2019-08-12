#!/bin/sh

if ! grep "pgwtun" /proc/net/dev > /dev/null; then
    ip tuntap add name pgwtun mode tun
fi
ip addr del $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev pgwtun 2> /dev/null
ip addr add $NEPC_PGW_IP4/$NEPC_PGW_MASK4 dev pgwtun
ip addr del $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev pgwtun 2> /dev/null
ip addr add $NEPC_PGW_IP6/$NEPC_PGW_MASK6 dev pgwtun
ip link set pgwtun up
