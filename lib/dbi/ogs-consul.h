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

#if !defined(OGS_DBI_INSIDE) && !defined(OGS_DBI_COMPILATION)
#error "This header cannot be included directly."
#endif

#ifndef OGS_CONSUL_H
#define OGS_CONSUL_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <consul_http.h>

typedef struct ogs_consul_s {
    bool initialized;
    const char *name;
    void *uri;
    void *client;
    void *database;
} ogs_consul_t;

int ogs_consul_init(const char *db_uri);
void ogs_consul_final(void);
ogs_consul_t *ogs_consul(void);

#ifdef __cplusplus
}
#endif

#endif /* OGS_CONSUL_H */
