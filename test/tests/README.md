Test File Format
================

Test files (ending in `.test`) must be a single line in the following format:

*command* `|` *options* `|` *input* `|` *outfile* `|` *exit*

that is five fields separated by the pipe (`|`) character
(with optional whitespace)
where:

+ *command* = command to execute (`ad`)
+ *options* = command-line options or blank for none
+ *input*   = name of file to dump
+ *outfile* = `outfile` if an output file should be used; otherwise blank
+ *exit*    = expected exit status code

Test scripts (ending in `.sh`) can alternatively be used
when additional commands need to be executed as part of the test.
Scripts are called with a single command-line argument
that is the full path of the test output file.

Test scripts must follow the normal Unix convention
of exiting with zero on success and non-zero on failure.

Note on Test Names
------------------

Care must be taken when naming files that differ only in case
because of case-insensitive (but preserving) filesystems like HFS+
used on Mac OS X.

For example, tests such as these:

    ad-B2-e1-m.test
    ad-B2-E1-m.test

that are the same test but differ only in 'e' vs. 'E' will work fine
on every other Unix filesystem but cause a collision on HFS+.

One solution (the one used here) is to append a distinct number:

    ad-B2-e1-m_01.test
    ad-B2-E1-m_02.test

thus making the filenames unique regardless of case.
