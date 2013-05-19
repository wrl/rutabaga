#!/usr/bin/env python

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

    pass
