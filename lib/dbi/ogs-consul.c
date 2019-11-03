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


#include "ogs-dbi.h"

#include <consul_http.h>

int __ogs_dbi_domain;

static ogs_consul_t self;

int ogs_consul_init(const char *db_uri) {

    if (!db_uri) {
        ogs_error("No DB_URI");
        return OGS_ERROR;
    }

    memset(&self, 0, sizeof(ogs_consul_t));

    //  db_name is the last few chars of db_uri after the final /
    const char *ptr = db_uri + strlen(db_uri);
    while (*(ptr--) != '/') {
    }

    //  ptr now points to one character behind the / before the db name
    //  d_print("%s\n", ptr+2);
    char *name = malloc(sizeof(char) * strlen(ptr+2));
    strcpy(name, ptr+2);
    self.name = name;


    char uri[1024];
    strcpy(uri, "http://");
    memcpy(uri+7, db_uri+9, (ptr-db_uri + 1 - 9));
    uri[(ptr-db_uri + 1 - 9 + 7)] = '\0';
//  d_print("context setting uri to %s\n", uri);
//  d_print("context setting dbname to %s\n", self.db_name);
    self.client = consul_client_init(uri, name);

    return OGS_OK;
}


void ogs_consul_final(void)
{
    self.name = NULL;
    self.client = NULL;
}

ogs_consul_t *ogs_consul(void)
{
    return &self;
}
