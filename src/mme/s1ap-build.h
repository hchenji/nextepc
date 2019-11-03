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

#ifndef S1AP_BUILD_H
#define S1AP_BUILD_H

#include "mme-context.h"

#include "sbc-message.h"

#ifdef __cplusplus
extern "C" {
#endif

int s1ap_build_setup_rsp(ogs_pkbuf_t **pkbuf);
int s1ap_build_setup_failure(
    ogs_pkbuf_t **pkbuf, S1AP_Cause_PR group, long cause, long time_to_wait);

int s1ap_build_downlink_nas_transport(
    ogs_pkbuf_t **s1apbuf, enb_ue_t *enb_ue, ogs_pkbuf_t *emmbuf);

int s1ap_build_initial_context_setup_request(
    ogs_pkbuf_t **s1apbuf, mme_ue_t *mme_ue, ogs_pkbuf_t *emmbuf);
int s1ap_build_ue_context_modification_request(
    ogs_pkbuf_t **s1apbuf, mme_ue_t *mme_ue);
int s1ap_build_ue_context_release_command(
    ogs_pkbuf_t **s1apbuf, enb_ue_t *enb_ue, S1AP_Cause_PR group, long cause);

int s1ap_build_e_rab_setup_request(
    ogs_pkbuf_t **s1apbuf, mme_bearer_t *bearer, ogs_pkbuf_t *esmbuf);
int s1ap_build_e_rab_modify_request(
            ogs_pkbuf_t **s1apbuf, mme_bearer_t *bearer, ogs_pkbuf_t *esmbuf);
int s1ap_build_e_rab_release_command(ogs_pkbuf_t **s1apbuf,
    mme_bearer_t *bearer, ogs_pkbuf_t *esmbuf, S1AP_Cause_PR group, long cause);

int s1ap_build_paging(ogs_pkbuf_t **s1apbuf,
        mme_ue_t *mme_ue, S1AP_CNDomain_t cn_domain);

int s1ap_build_mme_configuration_transfer(
    ogs_pkbuf_t **s1apbuf, 
    S1AP_SONConfigurationTransfer_t *son_configuration_transfer);

int s1ap_build_path_switch_ack(
    ogs_pkbuf_t **s1apbuf, mme_ue_t *mme_ue);
int s1ap_build_path_switch_failure(ogs_pkbuf_t **s1apbuf,
    uint32_t enb_ue_s1ap_id, uint32_t mme_ue_s1ap_id,
    S1AP_Cause_PR group, long cause);

int s1ap_build_handover_command(
    ogs_pkbuf_t **s1apbuf, enb_ue_t *source_ue);
int s1ap_build_handover_preparation_failure(
    ogs_pkbuf_t **s1apbuf, enb_ue_t *source_ue, S1AP_Cause_t *cause);

int s1ap_build_handover_request(
    ogs_pkbuf_t **s1apbuf, mme_ue_t *mme_ue, enb_ue_t *target_ue,
    S1AP_ENB_UE_S1AP_ID_t *enb_ue_s1ap_id,
    S1AP_MME_UE_S1AP_ID_t *mme_ue_s1ap_id,
    S1AP_HandoverType_t *handovertype,
    S1AP_Cause_t *cause,
    S1AP_Source_ToTarget_TransparentContainer_t
        *source_totarget_transparentContainer);

int s1ap_build_handover_cancel_ack(
    ogs_pkbuf_t **s1apbuf, enb_ue_t *source_ue);

int s1ap_build_mme_status_transfer(ogs_pkbuf_t **s1apbuf,
    enb_ue_t *target_ue,
    S1AP_ENB_StatusTransfer_TransparentContainer_t
        *enb_statustransfer_transparentContainer);

int s1ap_build_error_indication(
    ogs_pkbuf_t **s1apbuf,
    S1AP_MME_UE_S1AP_ID_t *mme_ue_s1ap_id,
    S1AP_ENB_UE_S1AP_ID_t *enb_ue_s1ap_id,
    S1AP_Cause_PR group, long cause);

int s1ap_build_s1_reset(
    ogs_pkbuf_t **s1apbuf,
    S1AP_Cause_PR group, long cause,
    S1AP_UE_associatedLogicalS1_ConnectionListRes_t *partOfS1_Interface);

int s1ap_build_s1_reset_partial(
    ogs_pkbuf_t **s1apbuf,
    S1AP_Cause_PR group, long cause,
    S1AP_MME_UE_S1AP_ID_t *mme_ue_s1ap_id,
    S1AP_ENB_UE_S1AP_ID_t *enb_ue_s1ap_id);

int s1ap_build_s1_reset_ack(
    ogs_pkbuf_t **s1apbuf,
    S1AP_UE_associatedLogicalS1_ConnectionListRes_t *partOfS1_Interface);

int s1ap_build_write_replace_warning_request(
    ogs_pkbuf_t **s1apbuf, sbc_pws_data_t *sbc_pws);

int s1ap_build_kill_request(
    ogs_pkbuf_t **s1apbuf, sbc_pws_data_t *sbc_pws);

#ifdef __cplusplus
}
#endif

#endif /* S1AP_BUILD_H */
