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

#include "gtp/gtp-node.h"
#include "fd/fd-lib.h"
#include "fd/gx/gx-message.h"

#include "pgw-sm.h"
#include "pgw-context.h"
#include "pgw-event.h"
#include "pgw-gtp-path.h"
#include "pgw-s5c-handler.h"
#include "pgw-gx-handler.h"
#include "pgw-fd-path.h"

void pgw_state_initial(ogs_fsm_t *s, pgw_event_t *e)
{
    pgw_sm_debug(e);

    ogs_assert(s);

    OGS_FSM_TRAN(s, &pgw_state_operational);
}

void pgw_state_final(ogs_fsm_t *s, pgw_event_t *e)
{
    pgw_sm_debug(e);

    ogs_assert(s);
}

void pgw_state_operational(ogs_fsm_t *s, pgw_event_t *e)
{
    int rv;
    ogs_pkbuf_t *recvbuf = NULL;
    ogs_pkbuf_t *copybuf = NULL;
    uint16_t copybuf_len = 0;
    gtp_xact_t *xact = NULL;
    gtp_message_t *message = NULL;
    pgw_sess_t *sess = NULL;
    ogs_index_t sess_index;
    ogs_pkbuf_t *gxbuf = NULL;
    gx_message_t *gx_message = NULL;
    uint32_t xact_index;
    ogs_pkbuf_t *gtpbuf = NULL;

    pgw_sm_debug(e);

    ogs_assert(s);

    switch (e->id) {
    case OGS_FSM_ENTRY_SIG:
        rv = pgw_gtp_open();
        if (rv != OGS_OK)
            ogs_fatal("Can't establish PGW path");
        break;
    case OGS_FSM_EXIT_SIG:
        pgw_gtp_close();
        break;
    case PGW_EVT_S5C_MESSAGE:
        ogs_assert(e);
        recvbuf = e->gtpbuf;
        ogs_assert(recvbuf);

        copybuf_len = sizeof(gtp_message_t);
        copybuf = ogs_pkbuf_alloc(NULL, copybuf_len);
        ogs_pkbuf_put(copybuf, copybuf_len);
        message = copybuf->data;
        ogs_assert(message);

        rv = gtp_parse_msg(message, recvbuf);
        ogs_assert(rv == OGS_OK);

        if (message->h.teid == 0) {
            gtp_node_t *sgw = pgw_sgw_add_by_message(message);
            ogs_assert(sgw);
            sess = pgw_sess_add_by_message(message);
            SETUP_GTP_NODE(sess, sgw);
        } else {
            sess = pgw_sess_find_by_teid(message->h.teid);
        }
        ogs_assert(sess);

        rv = gtp_xact_receive(sess->gnode, &message->h, &xact);
        if (rv != OGS_OK) {
            ogs_pkbuf_free(recvbuf);
            ogs_pkbuf_free(copybuf);
            break;
        }

        switch(message->h.type) {
        case GTP_CREATE_SESSION_REQUEST_TYPE:
            pgw_s5c_handle_create_session_request(
                sess, xact, &message->create_session_request);
            pgw_gx_send_ccr(sess, xact, copybuf,
                GX_CC_REQUEST_TYPE_INITIAL_REQUEST);
            break;
        case GTP_DELETE_SESSION_REQUEST_TYPE:
            pgw_s5c_handle_delete_session_request(
                sess, xact, &message->delete_session_request);
            pgw_gx_send_ccr(sess, xact, copybuf,
                GX_CC_REQUEST_TYPE_TERMINATION_REQUEST);
            break;
        case GTP_CREATE_BEARER_RESPONSE_TYPE:
            pgw_s5c_handle_create_bearer_response(
                sess, xact, &message->create_bearer_response);
            ogs_pkbuf_free(copybuf);
            break;
        case GTP_UPDATE_BEARER_RESPONSE_TYPE:
            pgw_s5c_handle_update_bearer_response(
                sess, xact, &message->update_bearer_response);
            ogs_pkbuf_free(copybuf);
            break;
        case GTP_DELETE_BEARER_RESPONSE_TYPE:
            pgw_s5c_handle_delete_bearer_response(
                sess, xact, &message->delete_bearer_response);
            ogs_pkbuf_free(copybuf);
            break;
        default:
            ogs_warn("Not implmeneted(type:%d)", message->h.type);
            ogs_pkbuf_free(copybuf);
            break;
        }
        ogs_pkbuf_free(recvbuf);
        break;

    case PGW_EVT_GX_MESSAGE:
        ogs_assert(e);

        gxbuf = e->gxbuf;
        ogs_assert(gxbuf);
        gx_message = gxbuf->data;
        ogs_assert(gx_message);

        sess_index = e->sess_index;
        ogs_assert(sess_index);
        sess = pgw_sess_find(sess_index);

        switch(gx_message->cmd_code) {
        case GX_CMD_CODE_CREDIT_CONTROL:
            xact_index = e->xact_index;
            ogs_assert(xact_index);
            xact = gtp_xact_find(xact_index);
            ogs_assert(xact);

            gtpbuf = e->gtpbuf;
            ogs_assert(gtpbuf);
            message = gtpbuf->data;

            if (gx_message->result_code == ER_DIAMETER_SUCCESS) {
                switch(gx_message->cc_request_type) {
                case GX_CC_REQUEST_TYPE_INITIAL_REQUEST:
                    pgw_gx_handle_cca_initial_request(
                            sess, gx_message, xact, 
                            &message->create_session_request);
                    break;
                case GX_CC_REQUEST_TYPE_TERMINATION_REQUEST:
                    pgw_gx_handle_cca_termination_request(
                            sess, gx_message, xact,
                            &message->delete_session_request);
                    break;
                default:
                    ogs_error("Not implemented(%d)",
                            gx_message->cc_request_type);
                    break;
                }
            } else
                ogs_error("Diameter Error(%d)", gx_message->result_code);

            ogs_pkbuf_free(gtpbuf);
            break;
        case GX_CMD_RE_AUTH:
            pgw_gx_handle_re_auth_request(sess, gx_message);
            break;
        default:
            ogs_error("Invalid type(%d)", gx_message->cmd_code);
            break;
        }

        gx_message_free(gx_message);
        ogs_pkbuf_free(gxbuf);
        break;
    default:
        ogs_error("No handler for event %s", pgw_event_get_name(e));
        break;
    }
}
