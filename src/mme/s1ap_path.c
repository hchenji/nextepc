#include "mme_event.h"

#include "nas_security.h"
#include "nas_path.h"

#include "s1ap_conv.h"
#include "s1ap_build.h"
#include "s1ap_path.h"

static int s1ap_server_list(ogs_list_t *list, int type);
static int s1ap_delete_list(ogs_list_t *list);

int s1ap_open(void)
{
    int rv;
#if HAVE_USRSCTP != 1
    int type = SOCK_STREAM;
#else
    int type = SOCK_SEQPACKET;
#endif

    rv = s1ap_server_list(&mme_self()->s1ap_list, type);
    ogs_assert(rv == OGS_OK);
    rv = s1ap_server_list(&mme_self()->s1ap_list6, type);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int s1ap_close()
{
    s1ap_delete_list(&mme_self()->s1ap_list);
    s1ap_delete_list(&mme_self()->s1ap_list6);

    return OGS_OK;
}

static int s1ap_server_list(ogs_list_t *list, int type)
{
    ogs_socknode_t *snode = NULL;

    ogs_assert(list);

    ogs_list_for_each(list, snode)
        s1ap_server(snode, type);

    return OGS_OK;
}

static int s1ap_delete_list(ogs_list_t *list)
{
    ogs_socknode_t *snode = NULL;

    ogs_list_for_each(list, snode)
        s1ap_delete(snode);

    return OGS_OK;
}

int s1ap_send_to_enb(mme_enb_t *enb, ogs_pkbuf_t *pkbuf, uint16_t stream_no)
{
    char buf[OGS_ADDRSTRLEN];
    int rv;

    ogs_assert(enb);
    ogs_assert(pkbuf);
    ogs_assert(enb->sock);

    ogs_debug("    IP[%s] ENB_ID[%d]",
            OGS_ADDR(enb->addr, buf), enb->enb_id);

    rv = s1ap_send(enb->sock, pkbuf,
            enb->sock_type == SOCK_STREAM ? NULL : enb->addr,
            stream_no);
    if (rv != OGS_OK)
    {
        ogs_error("s1_send error");
        ogs_pkbuf_free(pkbuf);
    }

    return OGS_OK;;
}

int s1ap_send_to_enb_ue(enb_ue_t *enb_ue, ogs_pkbuf_t *pkbuf)
{
    mme_enb_t *enb = NULL;
    mme_ue_t *mme_ue = NULL;

    ogs_assert(enb_ue);
    enb = enb_ue->enb;
    ogs_assert(enb);
    mme_ue = enb_ue->mme_ue;
    ogs_assert(mme_ue);

    return s1ap_send_to_enb(enb, pkbuf, mme_ue->ostream_id);
}

int s1ap_delayed_send_to_enb_ue(
        enb_ue_t *enb_ue, ogs_pkbuf_t *pkbuf, ogs_time_t duration)
{
    ogs_assert(enb_ue);
    ogs_assert(pkbuf);
        
    if (duration)
    {
        mme_event_t *e = NULL;

        e = mme_event_new(MME_EVT_S1AP_DELAYED_SEND);
        ogs_assert(e);
        e->timer = ogs_timer_add(mme_self()->timer_mgr, mme_event_timeout, e);
        ogs_assert(e->timer);
        e->pkbuf = pkbuf;
        e->enb_ue = enb_ue;

        ogs_timer_start(e->timer, duration);

        return OGS_OK;
    }
    else
    {
        mme_enb_t *enb = NULL;
        enb = enb_ue->enb;
        ogs_assert(enb);
        return s1ap_send_to_enb_ue(enb_ue, pkbuf);
    }
}

int s1ap_send_to_esm(mme_ue_t *mme_ue, ogs_pkbuf_t *esmbuf)
{
    mme_event_t *e = NULL;

    ogs_assert(mme_ue);
    ogs_assert(esmbuf);

    e = mme_event_new(MME_EVT_ESM_MESSAGE);
    ogs_assert(e);
    e->mme_ue = mme_ue;
    e->pkbuf = esmbuf;
    mme_event_send(e);

    return OGS_OK;
}
 
int s1ap_send_to_nas(enb_ue_t *enb_ue,
        S1AP_ProcedureCode_t procedureCode, S1AP_NAS_PDU_t *nasPdu)
{
    nas_security_header_t *sh = NULL;
    nas_security_header_type_t security_header_type;

    nas_emm_header_t *h = NULL;
    ogs_pkbuf_t *nasbuf = NULL;
    mme_event_t *e = NULL;

    ogs_assert(enb_ue);
    ogs_assert(nasPdu);

    /* The Packet Buffer(pkbuf_t) for NAS message MUST make a HEADROOM. 
     * When calculating AES_CMAC, we need to use the headroom of the packet. */
    nasbuf = ogs_pkbuf_alloc(NULL, NAS_HEADROOM+nasPdu->size);
    ogs_pkbuf_reserve(nasbuf, NAS_HEADROOM);
    ogs_pkbuf_put_data(nasbuf, nasPdu->buf, nasPdu->size);

    sh = nasbuf->data;
    ogs_assert(sh);

    memset(&security_header_type, 0, sizeof(nas_security_header_type_t));
    switch(sh->security_header_type)
    {
        case NAS_SECURITY_HEADER_PLAIN_NAS_MESSAGE:
            break;
        case NAS_SECURITY_HEADER_FOR_SERVICE_REQUEST_MESSAGE:
            security_header_type.service_request = 1;
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED:
            security_header_type.integrity_protected = 1;
            ogs_assert(ogs_pkbuf_pull(nasbuf, 6));
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHERED:
            security_header_type.integrity_protected = 1;
            security_header_type.ciphered = 1;
            ogs_assert(ogs_pkbuf_pull(nasbuf, 6));
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_NEW_SECURITY_CONTEXT:
            security_header_type.integrity_protected = 1;
            security_header_type.new_security_context = 1;
            ogs_assert(ogs_pkbuf_pull(nasbuf, 6));
            break;
        case NAS_SECURITY_HEADER_INTEGRITY_PROTECTED_AND_CIPHTERD_WITH_NEW_INTEGRITY_CONTEXT:
            security_header_type.integrity_protected = 1;
            security_header_type.ciphered = 1;
            security_header_type.new_security_context = 1;
            ogs_assert(ogs_pkbuf_pull(nasbuf, 6));
            break;
        default:
            ogs_error("Not implemented(securiry header type:0x%x)", 
                    sh->security_header_type);
            return OGS_ERROR;
    }

    if (enb_ue->mme_ue)
    {
        ogs_assert(nas_security_decode(
            enb_ue->mme_ue, security_header_type, nasbuf) == OGS_OK);
    }

    h = nasbuf->data;
    ogs_assert(h);
    if (h->protocol_discriminator == NAS_PROTOCOL_DISCRIMINATOR_EMM)
    {
        e = mme_event_new(MME_EVT_EMM_MESSAGE);
        ogs_assert(e);
        e->enb_ue = enb_ue;
        e->s1ap_code = procedureCode;
        e->nas_type = security_header_type.type;
        e->pkbuf = nasbuf;
        mme_event_send(e);
    }
    else if (h->protocol_discriminator == NAS_PROTOCOL_DISCRIMINATOR_ESM)
    {
        mme_ue_t *mme_ue = enb_ue->mme_ue;
        ogs_assert(mme_ue);
        s1ap_send_to_esm(mme_ue, nasbuf);
    }
    else
        ogs_assert_if_reached();

    return OGS_OK;
}

int s1ap_send_initial_context_setup_request(mme_ue_t *mme_ue)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(mme_ue);

    rv = s1ap_build_initial_context_setup_request(&s1apbuf, mme_ue, NULL);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = nas_send_to_enb(mme_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int s1ap_send_ue_context_release_command(
    enb_ue_t *enb_ue, S1AP_Cause_PR group, long cause,
    uint8_t action, uint32_t delay)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(action != S1AP_UE_CTX_REL_INVALID_ACTION);

    ogs_assert(enb_ue);
    enb_ue->ue_ctx_rel_action = action;

    ogs_debug("[MME] UE Context release command");
    ogs_debug("    ENB_UE_S1AP_ID[%d] MME_UE_S1AP_ID[%d]",
            enb_ue->enb_ue_s1ap_id, enb_ue->mme_ue_s1ap_id);
    ogs_debug("    Group[%d] Cause[%d] Action[%d] Delay[%d]",
            group, (int)cause, action, delay);

    rv = s1ap_build_ue_context_release_command(&s1apbuf, enb_ue, group, cause);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_delayed_send_to_enb_ue(enb_ue, s1apbuf, delay);
    ogs_assert(rv == OGS_OK);

    return OGS_OK;
}

int s1ap_send_mme_configuration_transfer(
        mme_enb_t *target_enb,
        S1AP_SONConfigurationTransfer_t *SONConfigurationTransfer)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(target_enb);
    ogs_assert(SONConfigurationTransfer);

    rv = s1ap_build_mme_configuration_transfer(
            &s1apbuf, SONConfigurationTransfer);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb(target_enb, s1apbuf, S1AP_NON_UE_SIGNALLING);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_path_switch_ack(mme_ue_t *mme_ue)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(mme_ue);

    rv = s1ap_build_path_switch_ack(&s1apbuf, mme_ue);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = nas_send_to_enb(mme_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);
    
    return OGS_OK;
}

int s1ap_send_handover_command(enb_ue_t *source_ue)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(source_ue);

    rv = s1ap_build_handover_command(&s1apbuf, source_ue);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb_ue(source_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_handover_preparation_failure(
        enb_ue_t *source_ue, S1AP_Cause_t *cause)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(source_ue);
    ogs_assert(cause);

    rv = s1ap_build_handover_preparation_failure(&s1apbuf, source_ue, cause);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb_ue(source_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_handover_cancel_ack(enb_ue_t *source_ue)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(source_ue);

    rv = s1ap_build_handover_cancel_ack(&s1apbuf, source_ue);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb_ue(source_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return rv;
}


int s1ap_send_handover_request(
        mme_ue_t *mme_ue,
        mme_enb_t *target_enb,
        S1AP_ENB_UE_S1AP_ID_t *enb_ue_s1ap_id,
        S1AP_MME_UE_S1AP_ID_t *mme_ue_s1ap_id,
        S1AP_HandoverType_t *handovertype,
        S1AP_Cause_t *cause,
        S1AP_Source_ToTarget_TransparentContainer_t
            *source_totarget_transparentContainer)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    enb_ue_t *source_ue = NULL, *target_ue = NULL;

    ogs_debug("[MME] Handover request");
    
    ogs_assert(target_enb);

    ogs_assert(mme_ue);
    source_ue = mme_ue->enb_ue;
    ogs_assert(source_ue);
    ogs_assert(source_ue->target_ue == NULL);

    target_ue = enb_ue_add(target_enb);
    ogs_assert(target_ue);

    ogs_debug("    Source : ENB_UE_S1AP_ID[%d] MME_UE_S1AP_ID[%d]",
            source_ue->enb_ue_s1ap_id, source_ue->mme_ue_s1ap_id);
    ogs_debug("    Target : ENB_UE_S1AP_ID[Unknown] MME_UE_S1AP_ID[%d]",
            target_ue->mme_ue_s1ap_id);

    rv = source_ue_associate_target_ue(source_ue, target_ue);
    ogs_assert(rv == OGS_OK);

    rv = s1ap_build_handover_request(&s1apbuf, mme_ue, target_ue,
            enb_ue_s1ap_id, mme_ue_s1ap_id,
            handovertype, cause,
            source_totarget_transparentContainer);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb_ue(target_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_mme_status_transfer(
        enb_ue_t *target_ue,
        S1AP_ENB_StatusTransfer_TransparentContainer_t
            *enb_statustransfer_transparentContainer)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(target_ue);

    rv = s1ap_build_mme_status_transfer(&s1apbuf, target_ue,
            enb_statustransfer_transparentContainer);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb_ue(target_ue, s1apbuf);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_error_indication(
        mme_enb_t *enb,
        S1AP_MME_UE_S1AP_ID_t *mme_ue_s1ap_id,
        S1AP_ENB_UE_S1AP_ID_t *enb_ue_s1ap_id,
        S1AP_Cause_PR group, long cause)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(enb);

    rv = s1ap_build_error_indication(&s1apbuf,
            mme_ue_s1ap_id, enb_ue_s1ap_id, group, cause);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb(enb, s1apbuf, S1AP_NON_UE_SIGNALLING);
    ogs_assert(rv == OGS_OK);

    return rv;
}

int s1ap_send_s1_reset_ack(
        mme_enb_t *enb,
        S1AP_UE_associatedLogicalS1_ConnectionListRes_t *partOfS1_Interface)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;

    ogs_assert(enb);

    rv = s1ap_build_s1_reset_ack(&s1apbuf, partOfS1_Interface);
    ogs_assert(rv == OGS_OK && s1apbuf);

    rv = s1ap_send_to_enb(enb, s1apbuf, S1AP_NON_UE_SIGNALLING);
    ogs_assert(rv == OGS_OK);

    return rv;
}
