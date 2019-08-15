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

#include "gtp-message.h"
#include "gtp-node.h"
#include "gtp-path.h"

#include "gtp-xact.h"

#define GTP_MIN_XACT_ID             1
#define GTP_MAX_XACT_ID             0x800000

#define GTP_T3_RESPONSE_DURATION     ogs_time_from_sec(3) /* 3 seconds */
#define GTP_T3_RESPONSE_RETRY_COUNT  3 
#define GTP_T3_DUPLICATED_DURATION \
    (GTP_T3_RESPONSE_DURATION * GTP_T3_RESPONSE_RETRY_COUNT) /* 9 seconds */
#define GTP_T3_DUPLICATED_RETRY_COUNT 1 

typedef enum {
    GTP_XACT_UNKNOWN_STAGE,
    GTP_XACT_INITIAL_STAGE,
    GTP_XACT_INTERMEDIATE_STAGE,
    GTP_XACT_FINAL_STAGE,
} gtp_xact_stage_t;

static int gtp_xact_initialized = 0;
static ogs_timer_mgr_t *g_timer_mgr = NULL;
static uint32_t g_xact_id = 0;

static OGS_POOL(pool, gtp_xact_t);

static gtp_xact_stage_t gtp_xact_get_stage(uint8_t type, uint32_t sqn);
static int gtp_xact_delete(gtp_xact_t *xact);

static void response_timeout(void *data);
static void holding_timeout(void *data);

int gtp_xact_init(ogs_timer_mgr_t *timer_mgr)
{
    ogs_assert(gtp_xact_initialized == 0);

    ogs_pool_init(&pool, base_self()->gtp.xact.pool);

    g_xact_id = 0;
    g_timer_mgr = timer_mgr;

    gtp_xact_initialized = 1;

    return OGS_OK;
}

int gtp_xact_final(void)
{
    ogs_assert(gtp_xact_initialized == 1);

    ogs_pool_final(&pool);

    gtp_xact_initialized = 0;

    return OGS_OK;
}

gtp_xact_t *gtp_xact_local_create(
        gtp_node_t *gnode, gtp_header_t *hdesc, ogs_pkbuf_t *pkbuf)
{
    int rv;
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_t *xact = NULL;

    ogs_assert(gnode);

    ogs_pool_alloc(&pool, &xact);
    ogs_assert(xact);
    memset(xact, 0, sizeof *xact);
    xact->index = ogs_pool_index(&pool, xact);

    xact->org = GTP_LOCAL_ORIGINATOR;
    xact->xid = NEXT_ID(g_xact_id, GTP_MIN_XACT_ID, GTP_MAX_XACT_ID);
    xact->gnode = gnode;

    xact->tm_response = ogs_timer_add(g_timer_mgr, response_timeout, xact);
    ogs_assert(xact->tm_response);
    xact->response_rcount = GTP_T3_RESPONSE_RETRY_COUNT;

    xact->tm_holding = ogs_timer_add(g_timer_mgr, holding_timeout, xact);
    ogs_assert(xact->tm_holding);
    xact->holding_rcount = GTP_T3_DUPLICATED_RETRY_COUNT;

    ogs_list_add(xact->org == GTP_LOCAL_ORIGINATOR ?  
            &xact->gnode->local_list : &xact->gnode->remote_list, xact);

    rv = gtp_xact_update_tx(xact, hdesc, pkbuf);
    ogs_assert(rv == OGS_OK);

    ogs_debug("[%d] %s Create  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            OGS_ADDR(&gnode->conn, buf),
            OGS_PORT(&gnode->conn));

    return xact;
}

gtp_xact_t *gtp_xact_remote_create(gtp_node_t *gnode, uint32_t sqn)
{
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_t *xact = NULL;

    ogs_assert(gnode);

    ogs_pool_alloc(&pool, &xact);
    ogs_assert(xact);
    memset(xact, 0, sizeof *xact);
    xact->index = ogs_pool_index(&pool, xact);

    xact->org = GTP_REMOTE_ORIGINATOR;
    xact->xid = GTP_SQN_TO_XID(sqn);
    xact->gnode = gnode;

    xact->tm_response = ogs_timer_add(g_timer_mgr, response_timeout, xact);
    ogs_assert(xact->tm_response);
    xact->response_rcount = GTP_T3_RESPONSE_RETRY_COUNT;

    xact->tm_holding = ogs_timer_add(g_timer_mgr, holding_timeout, xact);
    ogs_assert(xact->tm_holding);
    xact->holding_rcount = GTP_T3_DUPLICATED_RETRY_COUNT;

    ogs_list_add(xact->org == GTP_LOCAL_ORIGINATOR ?  
            &xact->gnode->local_list : &xact->gnode->remote_list, xact);

    ogs_debug("[%d] %s Create  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            OGS_ADDR(&gnode->conn, buf),
            OGS_PORT(&gnode->conn));

    return xact;
}

void gtp_xact_delete_all(gtp_node_t *gnode)
{
    gtp_xact_t *xact = NULL, *next_xact = NULL;

    ogs_list_for_each_safe(&gnode->local_list, next_xact, xact)
        gtp_xact_delete(xact);
    ogs_list_for_each_safe(&gnode->remote_list, next_xact, xact)
        gtp_xact_delete(xact);
}

int gtp_xact_update_tx(gtp_xact_t *xact,
        gtp_header_t *hdesc, ogs_pkbuf_t *pkbuf)
{
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_stage_t stage;
    gtp_header_t *h = NULL;
    
    ogs_assert(xact);
    ogs_assert(xact->gnode);
    ogs_assert(hdesc);
    ogs_assert(pkbuf);

    ogs_debug("[%d] %s UPD TX-%d  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            hdesc->type,
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    stage = gtp_xact_get_stage(hdesc->type, xact->xid);
    if (xact->org == GTP_LOCAL_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            ogs_assert(xact->step == 0);
            break;

        case GTP_XACT_INTERMEDIATE_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 2);
            break;

        default:
            ogs_assert_if_reached();
        }
    } else if (xact->org == GTP_REMOTE_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_INTERMEDIATE_STAGE:
        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 1);
            break;

        default:
            ogs_assert_if_reached();
        }
    } else
        ogs_assert_if_reached();


    ogs_pkbuf_push(pkbuf, GTPV2C_HEADER_LEN);
    h = pkbuf->data;
    ogs_assert(h);

    memset(h, 0, sizeof(gtp_header_t));
    h->version = 2;
    h->teid_presence = 1;
    h->type = hdesc->type;
    h->length = htons(pkbuf->len - 4);
    h->teid = htonl(hdesc->teid);
    h->sqn = GTP_XID_TO_SQN(xact->xid);

    /* Save Message type and packet of this step */
    xact->seq[xact->step].type = h->type;
    xact->seq[xact->step].pkbuf = pkbuf;

    /* Step */
    xact->step++;

    return OGS_OK;
}

int gtp_xact_update_rx(gtp_xact_t *xact, uint8_t type)
{
    int rv = OGS_OK;
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_stage_t stage;

    ogs_debug("[%d] %s UPD RX-%d  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            type,
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    stage = gtp_xact_get_stage(type, xact->xid);
    if (xact->org == GTP_LOCAL_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_INTERMEDIATE_STAGE:
            if (xact->seq[1].type == type) {
                ogs_pkbuf_t *pkbuf = NULL;

                ogs_assert(xact->step == 2 || xact->step == 3);

                pkbuf = xact->seq[2].pkbuf;
                if (pkbuf) {
                    if (xact->tm_holding)
                        ogs_timer_start(
                                xact->tm_holding, GTP_T3_DUPLICATED_DURATION);

                    ogs_warn("[%d] %s Request Duplicated. Retransmit!"
                            " for step %d type %d peer [%s]:%d",
                            xact->xid,
                            xact->org == GTP_LOCAL_ORIGINATOR ?
                                "LOCAL " : "REMOTE",
                            xact->step, type,
                            OGS_ADDR(&xact->gnode->conn,
                                buf),
                            OGS_PORT(&xact->gnode->conn));
                    rv = gtp_sendto(xact->gnode, pkbuf);
                    ogs_assert(rv == OGS_OK);
                } else {
                    ogs_warn("[%d] %s Request Duplicated. Discard!"
                            " for step %d type %d peer [%s]:%d",
                            xact->xid,
                            xact->org == GTP_LOCAL_ORIGINATOR ?
                                "LOCAL " : "REMOTE",
                            xact->step, type,
                            OGS_ADDR(&xact->gnode->conn,
                                buf),
                            OGS_PORT(&xact->gnode->conn));
                }

                return OGS_RETRY;
            }

            ogs_assert(xact->step == 1);

            if (xact->tm_holding)
                ogs_timer_start(
                        xact->tm_holding, GTP_T3_DUPLICATED_DURATION);

            break;

        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 1);
            break;

        default:
            ogs_assert_if_reached();
        }
    } else if (xact->org == GTP_REMOTE_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            if (xact->seq[0].type == type) {
                ogs_pkbuf_t *pkbuf = NULL;

                ogs_assert(xact->step == 1 || xact->step == 2);

                pkbuf = xact->seq[1].pkbuf;
                if (pkbuf) {
                    if (xact->tm_holding)
                        ogs_timer_start(
                                xact->tm_holding, GTP_T3_DUPLICATED_DURATION);

                    ogs_warn("[%d] %s Request Duplicated. Retransmit!"
                            " for step %d type %d peer [%s]:%d",
                            xact->xid,
                            xact->org == GTP_LOCAL_ORIGINATOR ?
                                "LOCAL " : "REMOTE",
                            xact->step, type,
                            OGS_ADDR(&xact->gnode->conn,
                                buf),
                            OGS_PORT(&xact->gnode->conn));
                    rv = gtp_sendto(xact->gnode, pkbuf);
                    ogs_assert(rv == OGS_OK);
                } else {
                    ogs_warn("[%d] %s Request Duplicated. Discard!"
                            " for step %d type %d peer [%s]:%d",
                            xact->xid,
                            xact->org == GTP_LOCAL_ORIGINATOR ?
                                "LOCAL " : "REMOTE",
                            xact->step, type,
                            OGS_ADDR(&xact->gnode->conn,
                                buf),
                            OGS_PORT(&xact->gnode->conn));
                }

                return OGS_RETRY;
            }

            ogs_assert(xact->step == 0);
            if (xact->tm_holding)
                ogs_timer_start(
                        xact->tm_holding, GTP_T3_DUPLICATED_DURATION);

            break;

        case GTP_XACT_INTERMEDIATE_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 2);

            /* continue */
            break;

        default:
            ogs_assert_if_reached();
        }
    } else
        ogs_assert_if_reached();

    if (xact->tm_response)
        ogs_timer_stop(xact->tm_response);

    /* Save Message type of this step */
    xact->seq[xact->step].type = type;

    /* Step */
    xact->step++;

    return OGS_OK;
}


int gtp_xact_commit(gtp_xact_t *xact)
{
    int rv;
    char buf[OGS_ADDRSTRLEN];

    uint8_t type;
    ogs_pkbuf_t *pkbuf = NULL;
    gtp_xact_stage_t stage;
    
    ogs_assert(xact);
    ogs_assert(xact->gnode);

    ogs_debug("[%d] %s Commit  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    type = xact->seq[xact->step-1].type;
    stage = gtp_xact_get_stage(type, xact->xid);

    if (xact->org == GTP_LOCAL_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            ogs_assert(xact->step == 1);

            if (xact->tm_response)
                ogs_timer_start(
                        xact->tm_response, GTP_T3_RESPONSE_DURATION);

            break;

        case GTP_XACT_INTERMEDIATE_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 2 || xact->step == 3);
            if (xact->step == 2) {
                gtp_xact_delete(xact);
                return OGS_OK;
            }

            break;

        default:
            ogs_assert_if_reached();
        }
    } else if (xact->org == GTP_REMOTE_ORIGINATOR) {
        switch (stage) {
        case GTP_XACT_INITIAL_STAGE:
            ogs_assert_if_reached();

        case GTP_XACT_INTERMEDIATE_STAGE:
            ogs_assert(xact->step == 2);
            if (xact->tm_response)
                ogs_timer_start(
                        xact->tm_response, GTP_T3_RESPONSE_DURATION);

            break;

        case GTP_XACT_FINAL_STAGE:
            ogs_assert(xact->step == 2 || xact->step == 3);
            if (xact->step == 3) {
                gtp_xact_delete(xact);
                return OGS_OK;
            }

            break;

        default:
            ogs_assert_if_reached();
        }
    } else
        ogs_assert_if_reached();

    pkbuf = xact->seq[xact->step-1].pkbuf;
    ogs_assert(pkbuf);

    rv = gtp_sendto(xact->gnode, pkbuf);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

static void response_timeout(void *data)
{
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_t *xact = data;
    
    ogs_assert(xact);
    ogs_assert(xact->gnode);

    ogs_debug("[%d] %s Response Timeout "
            "for step %d type %d peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            xact->step, xact->seq[xact->step-1].type,
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    if (--xact->response_rcount > 0) {
        ogs_pkbuf_t *pkbuf = NULL;

        if (xact->tm_response)
            ogs_timer_start(xact->tm_response, GTP_T3_RESPONSE_DURATION);

        pkbuf = xact->seq[xact->step-1].pkbuf;
        ogs_assert(pkbuf);

        if (gtp_sendto(xact->gnode, pkbuf) != OGS_OK) {
            ogs_error("gtp_sendto() failed");
            goto out;
        }
    } else {
        ogs_warn("[%d] %s No Reponse. Give up! "
                "for step %d type %d peer [%s]:%d",
                xact->xid,
                xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
                xact->step, xact->seq[xact->step-1].type,
                OGS_ADDR(&xact->gnode->conn, buf),
                OGS_PORT(&xact->gnode->conn));
        gtp_xact_delete(xact);
    }

    return;

out:
    gtp_xact_delete(xact);
}

static void holding_timeout(void *data)
{
    char buf[OGS_ADDRSTRLEN];
    gtp_xact_t *xact = data;
    
    ogs_assert(xact);
    ogs_assert(xact->gnode);

    ogs_debug("[%d] %s Holding Timeout "
            "for step %d type %d peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            xact->step, xact->seq[xact->step-1].type,
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    if (--xact->holding_rcount > 0) {
        if (xact->tm_holding)
            ogs_timer_start(xact->tm_holding, GTP_T3_DUPLICATED_DURATION);
    } else {
        ogs_debug("[%d] %s Delete Transaction "
                "for step %d type %d peer [%s]:%d",
                xact->xid,
                xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
                xact->step, xact->seq[xact->step-1].type,
                OGS_ADDR(&xact->gnode->conn, buf),
                OGS_PORT(&xact->gnode->conn));
        gtp_xact_delete(xact);
    }
}


int gtp_xact_receive(
        gtp_node_t *gnode, gtp_header_t *h, gtp_xact_t **xact)
{
    char buf[OGS_ADDRSTRLEN];
    int rv;
    gtp_xact_t *new = NULL;

    ogs_assert(gnode);
    ogs_assert(h);

    new = gtp_xact_find_by_xid(gnode, h->type, GTP_SQN_TO_XID(h->sqn));
    if (!new)
        new = gtp_xact_remote_create(gnode, h->sqn);
    ogs_assert(new);

    ogs_debug("[%d] %s Receive peer [%s]:%d",
            new->xid,
            new->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            OGS_ADDR(&gnode->conn, buf),
            OGS_PORT(&gnode->conn));

    rv = gtp_xact_update_rx(new, h->type);
    if (rv != OGS_OK) {
        return rv;
    }

    *xact = new;
    return OGS_OK;
}

gtp_xact_t *gtp_xact_find(ogs_index_t index)
{
    ogs_assert(index);
    return ogs_pool_find(&pool, index);
}

static gtp_xact_stage_t gtp_xact_get_stage(uint8_t type, uint32_t xid)
{
    gtp_xact_stage_t stage = GTP_XACT_UNKNOWN_STAGE;

    switch (type) {
    case GTP_CREATE_SESSION_REQUEST_TYPE:
    case GTP_MODIFY_BEARER_REQUEST_TYPE:
    case GTP_DELETE_SESSION_REQUEST_TYPE:
    case GTP_MODIFY_BEARER_COMMAND_TYPE:
    case GTP_DELETE_BEARER_COMMAND_TYPE:
    case GTP_BEARER_RESOURCE_COMMAND_TYPE:
    case GTP_RELEASE_ACCESS_BEARERS_REQUEST_TYPE:
    case GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
    case GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE:
    case GTP_DOWNLINK_DATA_NOTIFICATION_TYPE:
        stage = GTP_XACT_INITIAL_STAGE;
        break;
    case GTP_CREATE_BEARER_REQUEST_TYPE:
    case GTP_UPDATE_BEARER_REQUEST_TYPE:
    case GTP_DELETE_BEARER_REQUEST_TYPE:
        if (xid & GTP_MAX_XACT_ID)
            stage = GTP_XACT_INTERMEDIATE_STAGE;
        else
            stage = GTP_XACT_INITIAL_STAGE;
        break;
    case GTP_CREATE_SESSION_RESPONSE_TYPE:
    case GTP_MODIFY_BEARER_RESPONSE_TYPE:
    case GTP_DELETE_SESSION_RESPONSE_TYPE:
    case GTP_MODIFY_BEARER_FAILURE_INDICATION_TYPE:
    case GTP_DELETE_BEARER_FAILURE_INDICATION_TYPE:
    case GTP_BEARER_RESOURCE_FAILURE_INDICATION_TYPE:
    case GTP_CREATE_BEARER_RESPONSE_TYPE:
    case GTP_UPDATE_BEARER_RESPONSE_TYPE:
    case GTP_DELETE_BEARER_RESPONSE_TYPE:
    case GTP_RELEASE_ACCESS_BEARERS_RESPONSE_TYPE:
    case GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
    case GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_RESPONSE_TYPE:
    case GTP_DOWNLINK_DATA_NOTIFICATION_ACKNOWLEDGE_TYPE:
        stage = GTP_XACT_FINAL_STAGE;
        break;

    default:
        ogs_error("Not implemented GTPv2 Message Type(%d)", type);
        break;
    }

    return stage;
}

gtp_xact_t *gtp_xact_find_by_xid(
        gtp_node_t *gnode, uint8_t type, uint32_t xid)
{
    char buf[OGS_ADDRSTRLEN];

    ogs_list_t *list = NULL;
    gtp_xact_t *xact = NULL;

    ogs_assert(gnode);

    switch (gtp_xact_get_stage(type, xid)) {
    case GTP_XACT_INITIAL_STAGE:
        list = &gnode->remote_list;
        break;
    case GTP_XACT_INTERMEDIATE_STAGE:
        list = &gnode->local_list;
        break;
    case GTP_XACT_FINAL_STAGE:
        if (xid & GTP_MAX_XACT_ID)
            list = &gnode->remote_list;
        else
            list = &gnode->local_list;
        break;
    default:
        ogs_assert_if_reached();
    }

    ogs_assert(list);
    ogs_list_for_each(list, xact) {
        if (xact->xid == xid) {
            ogs_debug("[%d] %s Find    peer [%s]:%d",
                    xact->xid,
                    xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
                    OGS_ADDR(&gnode->conn, buf),
                    OGS_PORT(&gnode->conn));
            break;
        }
    }

    return xact;
}

void gtp_xact_associate(gtp_xact_t *xact1, gtp_xact_t *xact2)
{
    ogs_assert(xact1);
    ogs_assert(xact2);

    ogs_assert(xact1->assoc_xact == NULL);
    ogs_assert(xact2->assoc_xact == NULL);

    xact1->assoc_xact = xact2;
    xact2->assoc_xact = xact1;
}

void gtp_xact_deassociate(gtp_xact_t *xact1, gtp_xact_t *xact2)
{
    ogs_assert(xact1);
    ogs_assert(xact2);

    ogs_assert(xact1->assoc_xact != NULL);
    ogs_assert(xact2->assoc_xact != NULL);

    xact1->assoc_xact = NULL;
    xact2->assoc_xact = NULL;
}

static int gtp_xact_delete(gtp_xact_t *xact)
{
    char buf[OGS_ADDRSTRLEN];

    ogs_assert(xact);
    ogs_assert(xact->gnode);

    ogs_debug("[%d] %s Delete  peer [%s]:%d",
            xact->xid,
            xact->org == GTP_LOCAL_ORIGINATOR ? "LOCAL " : "REMOTE",
            OGS_ADDR(&xact->gnode->conn, buf),
            OGS_PORT(&xact->gnode->conn));

    if (xact->seq[0].pkbuf)
        ogs_pkbuf_free(xact->seq[0].pkbuf);
    if (xact->seq[1].pkbuf)
        ogs_pkbuf_free(xact->seq[1].pkbuf);
    if (xact->seq[2].pkbuf)
        ogs_pkbuf_free(xact->seq[2].pkbuf);

    if (xact->tm_response)
        ogs_timer_delete(xact->tm_response);
    if (xact->tm_holding)
        ogs_timer_delete(xact->tm_holding);

    if (xact->assoc_xact)
        gtp_xact_deassociate(xact, xact->assoc_xact);

    ogs_list_remove(xact->org == GTP_LOCAL_ORIGINATOR ?
            &xact->gnode->local_list : &xact->gnode->remote_list, xact);
    ogs_pool_free(&pool, xact);

    return OGS_OK;
}

