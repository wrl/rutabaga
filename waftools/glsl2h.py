#!/usr/bin/env python

# rutabaga: an OpenGL widget toolkit
# Copyright (c) 2013-2018 William Light.
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

import re

all = ["write_shader_header"]

comment_start = re.compile(r"\s*/\*.*$")
comment_end = re.compile(r"^.*\*/\s*")
inline_comment = re.compile(r"(\s*/\*.*\*/\s*)|(\s*//.*$)")
leading_whitespace = re.compile(r"(\s+)(.*)")

def process_shader(f):
    output = ""

    in_comment = False

    for line in f:
        line = line.rstrip()

        if len(line) == 0:
            output += "\n"
            continue

        if in_comment:
            if not comment_end.match(line):
                continue
            else:
                line = comment_end.sub("", line)
                in_comment = False

        ws_match = leading_whitespace.match(line)
        if ws_match:
            whitespace = ws_match.group(1)
            line = ws_match.group(2)
        else:
            whitespace = ""

        line = inline_comment.sub("", line)
        if comment_start.match(line):
            line = comment_start.sub("", line)
            in_comment = True

        if len(line) == 0:
            continue

        output += "\n\t{ws}\"{line}\\n\"".format(
                ws=whitespace,
                line=line.replace("\"", '\\\"'))

    return output

def write_shader_header(outfile, infiles):
    output = ""

    shname = outfile.name
    shname = shname[:shname.find(".glsl.h")].replace("-", "_").upper()

    for file in infiles:
        if file.name.find("vert") > 0:
            shtype = "VERT"
        elif file.name.find("frag") > 0:
            shtype = "FRAG"
        else:
            raise Exception("i have no idea what kind of shader \"{0}\" is.".format(file.name))

        output += "static const char *{0}_{1}_SHADER = ".format(shname, shtype)

        f = open(file.abspath())
        output += process_shader(f)
        output += ";\n\n"

    # get rid of the last newline
    output = output[:len(output) - 1]
    outfile.write(output)

if __name__ == '__main__':
    import sys
    sys.stdout.write(process_shader(sys.stdin))
    sys.stdout.write("\n\n")
