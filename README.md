# pg_startup_proc

## Overview

`pg_startup_proc` is a PostgreSQL extension that allows you to run one or more stored procedures immediately after the database completes recovery. This is useful for initializing application state, recreating logical replication slots, or triggering automation workflows.

## Features

- Run multiple stored procedures after recovery
- Enforce timeout limits to prevent endless loops
- Execute before accepting client connections
- Configurable via GUC parameters

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/shshakya/pg_startup_proc.git
   cd pg_startup_proc
   ```

2. Build and install:
   ```bash
   make
   sudo make install
   ```

3. Update `postgresql.conf`:
   ```conf
   shared_preload_libraries = 'startup_proc_extension'
   startup.proc_list = 'proc_one,proc_two'
   startup.timeout_ms = 10000
   ```

4. Restart PostgreSQL.

## Compatibility

Tested with PostgreSQL 14 and above.

## License

MIT
