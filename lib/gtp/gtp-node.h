/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GTP_NODE_H
#define GTP_NODE_H

#include "gtp-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SETUP_GTP_NODE(__cTX, __gNODE) \
    do { \
        ogs_assert((__cTX)); \
        ogs_assert((__gNODE)); \
        (__cTX)->gnode = __gNODE; \
    } while(0)

/**
 * This structure represents the commonalities of GTP node such as MME, SGW,
 * PGW gateway. Some of members may not be used by the specific type of node */
typedef struct gtp_node_s {
    ogs_lnode_t     node;           /* A node of list_t */

    ogs_sockaddr_t  *addr;          /* Socket Address List */

    ogs_sock_t      *sock;          /* Socket instance */
    ip_t            ip;             /* Socket Address */
    ogs_sockaddr_t  conn;           /* Connected Address */

    ogs_list_t      local_list;    
    ogs_list_t      remote_list;   
} gtp_node_t;

int gtp_node_init(void);
int gtp_node_final(void);

gtp_node_t *gtp_node_new(ogs_sockaddr_t *addr);
void gtp_node_free(gtp_node_t *node);

gtp_node_t *gtp_node_add(
        ogs_list_t *list, gtp_f_teid_t *f_teid,
        uint16_t port, int no_ipv4, int no_ipv6, int prefer_ipv4);
void gtp_node_remove(ogs_list_t *list, gtp_node_t *node);
void gtp_node_remove_all(ogs_list_t *list);

gtp_node_t *gtp_node_find(ogs_list_t *list, gtp_f_teid_t *f_teid);

#ifdef __cplusplus
}
#endif

#endif /* GTP_NODE_H */
