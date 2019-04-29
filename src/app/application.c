#include "base/types.h"

#include "context.h"
#include "application.h"

#define DEFAULT_CONFIG_FILE_PATH SYSCONF_DIR "nextepc/nextepc.conf"
#define DEFAULT_RUNTIME_DIR_PATH LOCALSTATE_DIR "run/"

static int log_pid(const char *app_name, const char *pid_path);

int app_will_initialize(app_param_t *param)
{
    int rv;
    ogs_assert(param);

    context_init();

    if (param->name)
    {
        rv = log_pid(param->name, param->pid_path);
        if (rv != OGS_OK) return rv;
    }

    context_self()->config.path = param->config_path;
    if (context_self()->config.path == NULL)
        context_self()->config.path = DEFAULT_CONFIG_FILE_PATH;

    rv = context_read_file();
    if (rv != OGS_OK) return rv;

    rv = context_parse_config();
    if (rv != OGS_OK) return rv;

    context_self()->log.path = param->log_path;

    if (param->logfile_disabled == false &&
        context_self()->config.logger.file)
    {
        if (context_self()->log.path)
            context_self()->config.logger.file = context_self()->log.path;

        if (ogs_log_add_file(context_self()->config.logger.file) == NULL)
        {
            ogs_fatal("cannot open log file : %s",
                    context_self()->config.logger.file);
            ogs_assert_if_reached();
        }

        ogs_log_print(OGS_LOG_INFO, 
                "File Logging: '%s'\n", context_self()->config.logger.file);
    }
    if (context_self()->config.logger.level)
    {
        if (!strcasecmp(context_self()->config.logger.level, "none"))
            context_self()->log.level = OGS_LOG_NONE;
        else if (!strcasecmp(context_self()->config.logger.level, "fatal"))
            context_self()->log.level = OGS_LOG_FATAL;
        else if (!strcasecmp(context_self()->config.logger.level, "error"))
            context_self()->log.level = OGS_LOG_ERROR;
        else if (!strcasecmp(context_self()->config.logger.level, "warn"))
            context_self()->log.level = OGS_LOG_WARN;
        else if (!strcasecmp(context_self()->config.logger.level, "info"))
            context_self()->log.level = OGS_LOG_INFO;
        else if (!strcasecmp(context_self()->config.logger.level, "debug"))
            context_self()->log.level = OGS_LOG_DEBUG;
        else if (!strcasecmp(context_self()->config.logger.level, "trace"))
            context_self()->log.level = OGS_LOG_TRACE;
        else {
            ogs_error("Invalid LOG-LEVEL : %s\n",
                    context_self()->config.logger.level);
            return OGS_ERROR;
        }
        ogs_log_print(OGS_LOG_INFO, 
                "LOG-LEVEL: '%s'\n", context_self()->config.logger.level);
    }
    if (context_self()->config.logger.domain)
    {
        context_self()->log.domain = context_self()->config.logger.domain;
        ogs_log_print(OGS_LOG_INFO, 
                "LOG-DOMAIN: '%s'\n", context_self()->config.logger.domain);
    }

    if (param->log_level)
    {
        context_self()->log.level = param->log_level;
        context_self()->log.domain = param->log_domain;
    }

    rv = context_setup_log_module();
    if (rv != OGS_OK) return rv;

    if (param->db_disabled == false &&
        context_self()->config.db_uri)
    {
        /* Override configuration if DB_URI environment variable is existed */
        if (ogs_env_get("DB_URI"))
            context_self()->config.db_uri = ogs_env_get("DB_URI");

        rv = context_db_init(context_self()->config.db_uri);
        if (rv != OGS_OK) return rv;
        ogs_log_print(OGS_LOG_INFO, "MongoDB URI: '%s'\n",
                context_self()->config.db_uri);
    }
    ogs_log_print(OGS_LOG_INFO, "Configuration: '%s'\n",
            context_self()->config.path);

    return rv;
}

int app_did_initialize(void)
{
    return OGS_OK;
}

void app_will_terminate(void)
{
    if (context_self()->config.db_uri)
    {
        ogs_info("DB-Client try to terminate");
        context_db_final();
        ogs_info("DB-Client terminate...done");
    }
}

void app_did_terminate(void)
{
    context_final();
}

static int log_pid(const char *app_name, const char *pid_path)
{
    FILE *pid_file = NULL;
    pid_t mypid;
    char default_pid_path[MAX_FILEPATH_LEN];

    if (pid_path == NULL)
    {
        ogs_snprintf(default_pid_path, sizeof(default_pid_path),
                "%snextepc-%sd/pid", DEFAULT_RUNTIME_DIR_PATH, app_name);
        pid_path = default_pid_path;
    }

    mypid = getpid();
    pid_file = fopen(pid_path, "w");
    if (!pid_file)
    {
        ogs_error("CHECK PERMISSION of Installation Directory...");
        ogs_error("Cannot create PID file:`%s`", pid_path);
        ogs_assert_if_reached();
    }
    fprintf(pid_file, "%d\n", (int)mypid);
    fclose(pid_file);

    ogs_log_print(OGS_LOG_INFO, "PID[%d]: '%s'\n", (int)mypid, pid_path);

    return OGS_OK;
}

int app_logger_restart()
{
    ogs_log_cycle();

    return OGS_OK;
}
