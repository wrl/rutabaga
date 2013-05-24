#!/usr/bin/env python

# rutabaga: an OpenGL widget toolkit
# Copyright (c) 2013 William Light.
# All rights reserved.
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>

import sys
import argparse

all = ['write_hex_data']

preamble = """\
/**
 * file autogenerated with bin2h.py
 * you probably don't want to edit this.
 */

#include <stdint.h>
#include <stddef.h>
"""

def write_hex_data(file_in, file_out, line_wrap=79, line_start=''):
    raw_out = file_out
    nbytes = 0
    buf = ""

    byte = file_in.read(1)
    while byte:
        nbytes += 1
        hexbyte = "0x{0:02X}, ".format(ord(byte))

        if len(buf) + len(hexbyte) > line_wrap:
            file_out.write(line_start)
            file_out.write(buf.rstrip(' '))
            file_out.write("\n")
            buf = ""

        buf += hexbyte
        byte = file_in.read(1)

    if buf:
        file_out.write(line_start)
        file_out.write(buf.rstrip(' ,'))
        file_out.write("\n")

    return nbytes


def main():
    p = argparse.ArgumentParser(description='embed data into C headers')
    p.add_argument('-n', '--no-preamble',
            help="don't output a comment about the file being autogenerated or the #include lines",
            action='store_true')
    p.add_argument('-v', '--variable-name', metavar="VAR",
            help="specify the variable name of the converted output. (default is 'bin2h')")
    p.add_argument('-p', '--variable-prefix', metavar="PREFIX",
            help="variable prefix (e.g. `static`)")
    args = p.parse_args()

    varname = args.variable_name or "bin2h"

    if args.variable_prefix:
        prefix = args.variable_prefix + " "
    else:
        prefix = ""

    prefix += "const "

    if not args.no_preamble:
        print(preamble)

    print("{prefix}uint8_t {var}_DATA[] = {{".format(
        prefix=prefix, var=varname))

    nbytes = write_hex_data(
            file_in=sys.stdin.detach(),
            file_out=sys.stdout,
            line_start='\t',
            line_wrap=79-8)

    print("};")

    print("#define {0}_SIZE {1}".format(
        varname, nbytes, prefix=prefix))


if __name__ == '__main__':
    main()
