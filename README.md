# ad

**ad** dumps the bytes of a file in hexadecimal
and also prints their correcponding ASCII characters
(for those bytes for which **isprint**(3) returns true)
to standard output.
For those bytes that are not printable,
prints '`.`' (dot) instead.

**ad** is similar to **hexdump**(1), **od**(1), and **xxd**(1)
except that **ad** can also search for
and highlight matching strings or numbers
similar to **grep**(1).

## Installation

The git repository contains only the necessary source code.
Things like `configure` are _derived_ sources and
[should not be included in repositories](http://stackoverflow.com/a/18732931).
If you have `autoconf`, `automake`, and `m4` installed,
you can generate `configure` yourself by doing:

    autoreconf -fiv
    
Then follow the generic installation instructions given in `INSTALL`.
