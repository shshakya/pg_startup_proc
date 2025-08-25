#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "executor/spi.h"
#include "postmaster/bgworker.h"
#include "storage/ipc.h"
#include "utils/guc.h"
#include "utils/builtins.h"
#include <unistd.h>

PG_MODULE_MAGIC;

static char *proc_list = NULL;
static int timeout_ms = 10000;

void startup_proc_extension_init(void);
void _PG_init(void);
void startup_worker_main(Datum main_arg);

void _PG_init(void) {
    DefineCustomStringVariable("startup.proc_list",
                               "Comma-separated list of stored procedures to run after recovery",
                               NULL,
                               &proc_list,
                               "",
                               PGC_POSTMASTER,
                               0,
                               NULL,
                               NULL,
                               NULL);

    DefineCustomIntVariable("startup.timeout_ms",
                            "Maximum time in milliseconds to run startup procedures",
                            NULL,
                            &timeout_ms,
                            1000,
                            60000,
                            PGC_POSTMASTER,
                            0,
                            NULL,
                            NULL,
                            NULL);

    BackgroundWorker worker;
    memset(&worker, 0, sizeof(BackgroundWorker));
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
    worker.bgw_main = startup_worker_main;
    snprintf(worker.bgw_name, BGW_MAXLEN, "Startup Procedure Runner");
    worker.bgw_restart_time = BGW_NEVER_RESTART;
    worker.bgw_main_arg = (Datum) 0;
    RegisterBackgroundWorker(&worker);
}

void startup_worker_main(Datum main_arg) {
    if (proc_list == NULL || strlen(proc_list) == 0) {
        ereport(LOG, (errmsg("No startup procedures configured")));
        return;
    }

    BackgroundWorkerUnblockSignals();
    pqsignal(SIGTERM, SIG_DFL);
    pqsignal(SIGINT, SIG_DFL);

    if (!SPI_connect()) {
        ereport(ERROR, (errmsg("SPI_connect failed")));
        return;
    }

    char *raw = pstrdup(proc_list);
    List *procs = NIL;
    char *token = strtok(raw, ",");
    while (token != NULL) {
        procs = lappend(procs, pstrdup(token));
        token = strtok(NULL, ",");
    }

    TimestampTz start_time = GetCurrentTimestamp();
    foreach(ListCell *lc, procs) {
        char *proc_name = (char *) lfirst(lc);
        char query[256];
        snprintf(query, sizeof(query), "CALL %s();", proc_name);

        int ret = SPI_execute(query, false, 0);
        if (ret != SPI_OK_UTILITY) {
            ereport(WARNING, (errmsg("Failed to execute procedure: %s", proc_name)));
        }

        TimestampTz now = GetCurrentTimestamp();
        long secs;
        int usecs;
        TimestampDifference(start_time, now, &secs, &usecs);
        if ((secs * 1000 + usecs / 1000) > timeout_ms) {
            ereport(WARNING, (errmsg("Startup procedure execution exceeded timeout")));
            break;
        }
    }

    SPI_finish();
}
