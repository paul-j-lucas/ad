# ad

**ad** dumps the bytes of a file in hexadecimal
and also prints their correcponding ASCII
or UTF-8
characters
(for those bytes for which **isprint**(3) returns true)
to standard output.
For those bytes that are not printable,
prints '`.`' (dot) instead.

**ad** is similar to **hexdump**(1), **od**(1), and **xxd**(1)
except that **ad** can also search for
and highlight
matching strings or numbers
similar to **grep**(1).
Like **xxd**(1),
**ad** can also patch binary files.

## Installation

The git repository contains only the necessary source code.
Things like `configure` are _derived_ sources and
[should not be included in repositories](http://stackoverflow.com/a/18732931).
If you have
[`autoconf`](https://www.gnu.org/software/autoconf/),
[`automake`](https://www.gnu.org/software/automake/),
and
[`m4`](https://www.gnu.org/software/m4/)
installed,
you can generate `configure` yourself by doing:

    ./bootstrap

then follow the generic installation instructions
given in `INSTALL`.

If you would like to generate the developer documentation,
you will also need
[Doxygen](http://www.doxygen.org/);
then do:

    make doc                            # or: make docs
