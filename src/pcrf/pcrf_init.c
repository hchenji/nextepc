#include "app/context.h"
#include "pcrf_context.h"
#include "pcrf_fd_path.h"

static int initialized = 0;

int pcrf_initialize(void)
{
    int rv;

    rv = pcrf_context_init();
    if (rv != OGS_OK) return rv;

    rv = pcrf_context_parse_config();
    if (rv != OGS_OK) return rv;

    rv = context_setup_log_module();
    if (rv != OGS_OK) return rv;

    rv = pcrf_db_init();
    if (rv != OGS_OK) return rv;

    rv = pcrf_fd_init();
    if (rv != OGS_OK) return OGS_ERROR;

    initialized = 1;

	return OGS_OK;
}

void pcrf_terminate(void)
{
    if (!initialized) return;

    pcrf_fd_final();

    pcrf_db_final();
    pcrf_context_final();
	
	return;
}
