-- SQL file to register the extension
CREATE FUNCTION startup_proc_extension_init() RETURNS void
    AS 'startup_proc_extension', 'startup_proc_extension_init'
    LANGUAGE C;
