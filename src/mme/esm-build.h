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

#ifndef ESM_BUILD_H
#define ESM_BUILD_H

#include "mme-context.h"

#ifdef __cplusplus
extern "C" {
#endif

int esm_build_pdn_connectivity_reject(ogs_pkbuf_t **pkbuf,
                            mme_sess_t *sess, nas_esm_cause_t esm_cause);
int esm_build_information_request(ogs_pkbuf_t **pkbuf,
                            mme_bearer_t *bearer);
int esm_build_activate_default_bearer_context_request(
                            ogs_pkbuf_t **pkbuf, mme_sess_t *sess);
int esm_build_activate_dedicated_bearer_context_request(
                            ogs_pkbuf_t **pkbuf, mme_bearer_t *bearer);
int esm_build_modify_bearer_context_request(
        ogs_pkbuf_t **pkbuf, mme_bearer_t *bearer, int qos_presence, int tft_presence);
int esm_build_deactivate_bearer_context_request(
        ogs_pkbuf_t **pkbuf, mme_bearer_t *bearer, nas_esm_cause_t esm_cause);

#ifdef __cplusplus
}
#endif

#endif /* ESM_BUILD_H */
