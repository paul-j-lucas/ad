Test File Format
================

Test files must be a single line in the following format:

*command* `|` *options* `|` *input* `|` *exit*

that is four fields separated by the pipe (`|`) character
(with optional whitespace)
where:

+ *command* = command to execute
+ *options* = command-line options or blank for none
+ *input*   = name of file to dump
+ *exit*    = expected exit status code
