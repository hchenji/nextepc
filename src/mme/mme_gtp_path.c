#include "gtp/gtp_node.h"
#include "gtp/gtp_path.h"
#include "gtp/gtp_xact.h"

#include "mme_event.h"
#include "mme_gtp_path.h"
#include "mme_s11_build.h"
#include "mme_sm.h"

static void _gtpv2_c_recv_cb(short when, ogs_socket_t fd, void *data)
{
    int rv;
    mme_event_t *e = NULL;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(fd != INVALID_SOCKET);

    rv = gtp_recv(fd, &pkbuf);
    if (rv != OGS_OK)
    {
        ogs_log_message(OGS_LOG_WARN, ogs_socket_errno, "gtp_recv() failed");
        return;
    }

    e = mme_event_new(MME_EVT_S11_MESSAGE);
    ogs_assert(e);
    e->pkbuf = pkbuf;
    mme_event_send(e);
}

static ogs_sockaddr_t *pgw_addr_find_by_family(ogs_list_t *list, int family)
{
    mme_pgw_t *pgw = NULL;
    ogs_assert(list);

    ogs_list_for_each(list, pgw)
    {
        ogs_assert(pgw->gnode);
        ogs_sockaddr_t *addr = pgw->gnode->sa_list;
        while(addr)
        {
            if (addr->c_sa_family == family)
            {
                return addr;
            }
            addr = addr->next;
        }
    }

    return NULL;
}

int mme_gtp_open()
{
    int rv;
    ogs_socknode_t *snode = NULL;
    ogs_sock_t *sock = NULL;
    mme_sgw_t *sgw = NULL;

    ogs_list_for_each(&mme_self()->gtpc_list, snode)
    {
        rv = gtp_server(snode);
        ogs_assert(rv == OGS_OK);

        sock = snode->sock;
        ogs_assert(sock);

        snode->poll = ogs_pollset_add(mme_self()->pollset,
                OGS_POLLIN, sock->fd, _gtpv2_c_recv_cb, NULL);
        ogs_assert(snode->poll);
    }
    ogs_list_for_each(&mme_self()->gtpc_list6, snode)
    {
        rv = gtp_server(snode);
        ogs_assert(rv == OGS_OK);

        sock = snode->sock;
        ogs_assert(sock);

        snode->poll = ogs_pollset_add(mme_self()->pollset,
                OGS_POLLIN, sock->fd, _gtpv2_c_recv_cb, NULL);
        ogs_assert(snode->poll);
    }

    mme_self()->gtpc_sock = gtp_local_sock_first(&mme_self()->gtpc_list);
    mme_self()->gtpc_sock6 = gtp_local_sock_first(&mme_self()->gtpc_list6);
    mme_self()->gtpc_addr = gtp_local_addr_first(&mme_self()->gtpc_list);
    mme_self()->gtpc_addr6 = gtp_local_addr_first(&mme_self()->gtpc_list6);

    ogs_assert(mme_self()->gtpc_addr || mme_self()->gtpc_addr6);

    mme_self()->pgw_addr = pgw_addr_find_by_family(
            &mme_self()->pgw_list, AF_INET);
    mme_self()->pgw_addr6 = pgw_addr_find_by_family(
            &mme_self()->pgw_list, AF_INET6);
    ogs_assert(mme_self()->pgw_addr || mme_self()->pgw_addr6);

    ogs_list_for_each(&mme_self()->sgw_list, sgw)
    {
        rv = gtp_connect(
                mme_self()->gtpc_sock, mme_self()->gtpc_sock6, sgw->gnode);
        ogs_assert(rv == OGS_OK);
    }

    return OGS_OK;
}

int mme_gtp_close()
{
    ogs_socknode_t *snode = NULL;

    ogs_list_for_each(&mme_self()->gtpc_list, snode)
    {
        ogs_pollset_remove(snode->poll);
        ogs_sock_destroy(snode->sock);
    }
    ogs_list_for_each(&mme_self()->gtpc_list6, snode)
    {
        ogs_pollset_remove(snode->poll);
        ogs_sock_destroy(snode->sock);
    }

    return OGS_OK;
}

int mme_gtp_send_create_session_request(mme_sess_t *sess)
{
    int rv;
    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;
    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    mme_ue = sess->mme_ue;
    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_CREATE_SESSION_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_create_session_request(&pkbuf, h.type, sess);
    ogs_assert(rv == OGS_OK);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}


int mme_gtp_send_modify_bearer_request(
        mme_bearer_t *bearer, int uli_presence)
{
    int rv;

    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(bearer);
    mme_ue = bearer->mme_ue;
    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_MODIFY_BEARER_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_modify_bearer_request(
            &pkbuf, h.type, bearer, uli_presence);
    ogs_assert(rv == OGS_OK);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_delete_session_request(mme_sess_t *sess)
{
    int rv;
    ogs_pkbuf_t *s11buf = NULL;
    gtp_header_t h;
    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    ogs_assert(sess);
    mme_ue = sess->mme_ue;
    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_DELETE_SESSION_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_delete_session_request(&s11buf, h.type, sess);
    ogs_assert(rv == OGS_OK);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, s11buf);
    ogs_assert(xact);

    GTP_XACT_STORE_SESSION(xact, sess);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_delete_all_sessions(mme_ue_t *mme_ue)
{
    int rv;
    mme_sess_t *sess = NULL, *next_sess = NULL;

    ogs_assert(mme_ue);
    sess = mme_sess_first(mme_ue);
    while (sess != NULL)
    {
        next_sess = mme_sess_next(sess);

        if (MME_HAVE_SGW_S1U_PATH(sess))
        {
            mme_bearer_t *bearer = mme_default_bearer_in_sess(sess);
            ogs_assert(bearer);

            if (bearer &&
                OGS_FSM_CHECK(&bearer->sm, esm_state_pdn_will_disconnect))
            {
                ogs_warn("PDN will disconnect[EBI:%d]", bearer->ebi);
            }
            else
            {
                rv = mme_gtp_send_delete_session_request(sess);
                ogs_assert(rv == OGS_OK);
            }
        }
        else
        {
            mme_sess_remove(sess);
        }

        sess = next_sess;
    }

    return OGS_OK;
}

int mme_gtp_send_create_bearer_response(mme_bearer_t *bearer)
{
    int rv;

    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(bearer);
    mme_ue = bearer->mme_ue;
    ogs_assert(mme_ue);
    xact = bearer->xact;
    ogs_assert(xact);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_CREATE_BEARER_RESPONSE_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_create_bearer_response(&pkbuf, h.type, bearer);
    ogs_assert(rv == OGS_OK);

    rv = gtp_xact_update_tx(xact, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_update_bearer_response(mme_bearer_t *bearer)
{
    int rv;

    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(bearer);
    mme_ue = bearer->mme_ue;
    ogs_assert(mme_ue);
    xact = bearer->xact;
    ogs_assert(xact);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_UPDATE_BEARER_RESPONSE_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_update_bearer_response(&pkbuf, h.type, bearer);
    ogs_assert(rv == OGS_OK);

    rv = gtp_xact_update_tx(xact, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_delete_bearer_response(mme_bearer_t *bearer)
{
    int rv;

    gtp_xact_t *xact = NULL;
    mme_ue_t *mme_ue = NULL;

    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(bearer);
    mme_ue = bearer->mme_ue;
    ogs_assert(mme_ue);
    xact = bearer->xact;
    ogs_assert(xact);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_DELETE_BEARER_RESPONSE_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_delete_bearer_response(&pkbuf, h.type, bearer);
    ogs_assert(rv == OGS_OK);

    rv = gtp_xact_update_tx(xact, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_release_access_bearers_request(mme_ue_t *mme_ue)
{
    int rv;
    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;
    gtp_xact_t *xact = NULL;

    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_RELEASE_ACCESS_BEARERS_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_release_access_bearers_request(&pkbuf, h.type);
    ogs_assert(rv == OGS_OK);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_create_indirect_data_forwarding_tunnel_request(
        mme_ue_t *mme_ue)
{
    int rv;
    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;
    gtp_xact_t *xact = NULL;

    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_CREATE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    rv = mme_s11_build_create_indirect_data_forwarding_tunnel_request(
            &pkbuf, h.type, mme_ue);
    ogs_assert(rv == OGS_OK);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int mme_gtp_send_delete_indirect_data_forwarding_tunnel_request(
        mme_ue_t *mme_ue)
{
    int rv;
    gtp_header_t h;
    ogs_pkbuf_t *pkbuf = NULL;
    gtp_xact_t *xact = NULL;

    ogs_assert(mme_ue);

    memset(&h, 0, sizeof(gtp_header_t));
    h.type = GTP_DELETE_INDIRECT_DATA_FORWARDING_TUNNEL_REQUEST_TYPE;
    h.teid = mme_ue->sgw_s11_teid;

    pkbuf = ogs_pkbuf_alloc(NULL, TLV_MAX_HEADROOM);
    ogs_pkbuf_reserve(pkbuf, TLV_MAX_HEADROOM);

    xact = gtp_xact_local_create(mme_ue->gnode, &h, pkbuf);
    ogs_assert(xact);

    rv = gtp_xact_commit(xact);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}
