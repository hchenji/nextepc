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

#ifndef APP_APPLICATION_H
#define APP_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct app_param_s {
    const char *name;

    const char *config_path;
    const char *log_path;
    const char *pid_path;

    bool logfile_disabled;
    bool db_disabled;

    ogs_log_level_e log_level;
    const char *log_domain;
} app_param_t;

int app_will_initialize(app_param_t *param);
int app_did_initialize(void);
void app_will_terminate(void);
void app_did_terminate(void);

int config_initialize(const char *config_path);
void config_terminate(void);

int app_logger_restart(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_APPLICATION_H */
