#!/usr/bin/env python2

import sys
def gen_test(n):
    print "CREATE TABLE t (a CHAR(%d));" % (n)
    print "--replace_regex /MariaDB/XYZ/ /MySQL/XYZ/"
    print "--error ER_UNSUPPORTED_EXTENSION"
    print "ALTER TABLE t CHANGE COLUMN a a BINARY(%d);" % (n)
    if n+1 < 256:
        print "--replace_regex /MariaDB/XYZ/ /MySQL/XYZ/"
        print "--error ER_UNSUPPORTED_EXTENSION"
        print "ALTER TABLE t CHANGE COLUMN a a CHAR(%d) BINARY;" % (n+1)
    print "DROP TABLE t;"

    print "CREATE TABLE t (a BINARY(%d));" % (n)
    print "--replace_regex /MariaDB/XYZ/ /MySQL/XYZ/"
    print "--error ER_UNSUPPORTED_EXTENSION"
    print "ALTER TABLE t CHANGE COLUMN a a CHAR(%d);" % (n)
    if n+1 < 256:
        print "--replace_regex /MariaDB/XYZ/ /MySQL/XYZ/"
        print "--error ER_UNSUPPORTED_EXTENSION"
        print "ALTER TABLE t CHANGE COLUMN a a CHAR(%d);" % (n+1)
    print "DROP TABLE t;"

def main():
    print "source include/have_tokudb.inc;"
    print "# this test is generated by change_char_binary.py"
    print "# test that char(X) <-> binary(X) is not hot"
    print "--disable_warnings"
    print "DROP TABLE IF EXISTS t,tt;"
    print "--enable_warnings"
    print "SET SESSION DEFAULT_STORAGE_ENGINE=\"TokuDB\";"
    print "SET SESSION TOKUDB_DISABLE_SLOW_ALTER=1;"
    for n in range(0,256):
        gen_test(n)
    return 0
sys.exit(main())
