MODULES = startup_proc_extension
EXTENSION = startup_proc_extension
DATA = startup_proc_extension--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
