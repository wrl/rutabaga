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

from rutabaga_css.prop import RutabagaStyleProperty

all = [
    "ColorParseException",
    "RutabagaRGBAProperty"]

class ColorParseException(Exception):
    pass

def parse_hex(h):
    if h[0] == '#':
        h = h[1:]

    if len(h) == 3:
        h = int(h, 16)
        r,g,b = [((h & (0xF << x)) >> x) / 15.0 for x in range(8, -1, -4)]
    elif len(h) == 6:
        h = int(h, 16)
        r,g,b = [((h & (0xFF << x)) >> x) / 255.0 for x in range(16, -1, -8)]
    else:
        raise ColorParseException()

    return (r,g,b)

def parse_rgba_func(args):
    args = [x for x in args if x.type in ['NUMBER', 'HASH']]

    if args[0].type == 'HASH':
        if len(args) != 2:
            raise ColorParseException()

        rgba = [0, 0, 0, args[1].value]
        rgba[:3] = parse_hex(args[0].value)
        return rgba

    else:
        if len(args) != 4:
            raise ColorParseException()

        return [x.value for x in args]

class RutabagaRGBAProperty(RutabagaStyleProperty):
    def __init__(self, stylesheet, name, tokens):
        self.rgba = [0,0,0,1.0]
        self.name = name

        tok = tokens[0]

        if tok.type == 'HASH':
            try:
                self.rgba[:3] = parse_hex(tok.value)
            except ColorParseException:
                raise ParseError(tok, "couldn't parse hex color")

        elif tok.type == 'FUNCTION' and tok.function_name == 'rgba':
            try:
                self.rgba = parse_rgba_func(tok.content)
            except ColorParseException:
                raise ParseError(tok,
                        "function rgba() takes either 4 numeric"
                        "arguments or a hex color and alpha")

        else:
            raise ParseError(tokens[0], "expected hex color or rgba()")

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_COLOR,
\t\t\t\t\t.color = {{
\t\t\t\t\t\t.r = {rgba[0]},
\t\t\t\t\t\t.g = {rgba[1]},
\t\t\t\t\t\t.b = {rgba[2]},
\t\t\t\t\t\t.a = {rgba[3]}}}"""

    def c_repr(self):
        return self.c_repr_tpl.format(rgba=self.rgba)

