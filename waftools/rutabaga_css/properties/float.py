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

from rutabaga_css.prop import RutabagaStyleProperty
from rutabaga_css.parser import ParseError

all = [
    'RutabagaFloatProperty']

class RutabagaFloatProperty(RutabagaStyleProperty):
    def __init__(self, stylesheet, name, tokens):
        self.name  = name
        self.value = None

        tok = tokens[0]

        if tok.type == 'DIMENSION':
            if tok.unit != 'px':
                raise ParseError(tok, 'non-pixel units still need to be implemented')

            self.value = tok.value
        else:
            raise ParseError(tokens[0], 'expected dimension')

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_FLOAT,
\t\t\t\t\t.flt = {val}"""

    def c_repr(self):
        return self.c_repr_tpl.format(val=self.value)
