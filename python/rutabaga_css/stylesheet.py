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

from __future__ import print_function

import os
import sys
from collections import OrderedDict

from rutabaga_css.parser import *
from rutabaga_css.style import RutabagaStyle
from rutabaga_css.font import *

all = ["RutabagaStylesheet"]

####
# RutabagaStylesheet
####

def decl_dict(declarations):
    ret = {}

    for d in declarations:
        ret[d.name] = d.value

    return ret

class RutabagaStylesheet(object):
    def __init__(self, css_file, autoparse=False):
        self.css_file = css_file
        self.parser = RutabagaCSSParser()

        self.styles = OrderedDict()
        self.namespaces = {}

        self.embedded_assets = []
        self.external_assets = []

        self.fonts = {}

        if autoparse:
            self.parse()

    def parse_font_face(self, rule):
        decls = decl_dict(rule.declarations)

        try:
            family = decls["font-family"][0].value
        except KeyError:
            raise ParseError(rule,
                    '"font-family" is a required property')

        try:
            src = decls["src"][0].value
        except KeyError:
            raise ParseError(rule,
                    '"src" is a required property')

        if family not in self.fonts:
            self.fonts[family] = RutabagaFontFace(family)

        font = self.fonts[family]

        if "font-weight" in decls:
            weight = decls["font-weight"][0].value
        else:
            weight = "normal"

        font.add_weight(self, weight, src)

    def parse(self):
        self.in_namespace = None
        css = self.parser.parse_stylesheet(self.css_file.read())

        for rule in css.rules:
            if type(rule) == NamespaceRule:
                if rule.prefix:
                    self.namespaces[rule.prefix] = rule
                else:
                    self.in_namespace = rule

            elif type(rule) == FontFaceRule:
                self.parse_font_face(rule)

            else:
                sel = self.parse_selector(rule.selector)
                decls = decl_dict(rule.declarations)

                for s in sel:
                    if s.find(":") > 0:
                        s, state = s.split(":", 1)

                        try:
                            self.styles[s].add_state(state, decls)
                        except AttributeError as e:
                            raise ParseError(rule, str(e))
                    else:
                        self.styles[s] = RutabagaStyle(self, s, decls)

        for s in self.styles:
            self.styles[s].done_parsing()

        if css.errors:
            for error in css.errors:
                print(error, file=sys.stderr)
                sys.exit(1)

    def parse_selector(self, selector):
        def pop():         return selector.pop() if selector else None
        def peek(n=1):     return selector[-n] if selector else None
        def is_comma(tok): return tok.type == 'DELIM' and tok.value == ','
        def bail(tok):
            raise ParseError(tok, 'unexpected "{0}"'.format(tok.type))

        ret = []

        while True:
            tok = pop()
            if not tok:
                return ret

            if is_comma(tok):
                continue

            if tok.type != "IDENT":
                bail(tok)

            ns = self.in_namespace
            val = tok.value

            while True:
                ptok = peek()

                if not ptok or is_comma(ptok):
                    if ns:
                        ret.append(ns.namespace + "." + val.lstrip())
                    else:
                        ret.append(val.lstrip())
                    break
                elif ptok.type == 'IDENT':
                    val = pop().value + val

                elif ptok.type == 'DELIM' and ptok.value == '|':
                    # namespace
                    pop()
                    ns = self.namespaces[pop().value]

                elif ptok.type == 'DELIM' and ptok.value == '.':
                    pop()
                    val = "." + val

                elif ptok.type == ':':
                    pop()
                    ptok = peek()

                    if ptok.type == ':':
                        # we treat `selector::pseudo` as being
                        # convenience syntax for `selector.pseudo`

                        pop()
                        ptok = peek()

                        if ptok.type != 'IDENT':
                            bail(ptok)

                        val = pop().value + "." + val
                    else:
                        val = ":" + val

                elif ptok.type == 'S':
                    pop()
                    val = " " + val

                else:
                    bail(ptok)

    c_include_tpl = '#include "{header}"'

    def c_prelude(self):
        return "\n\n".join((
            "\n".join(
                [self.c_include_tpl.format(header=a.header_path)
                    for a in self.embedded_assets]),
            "\n".join(
                [self.fonts[face].c_repr() for face in self.fonts])))

    c_repr_tpl = """\
{{
{style_structs}
}};"""

    def c_repr(self, var_name):
        return self.c_repr_tpl.format(
            var_name=var_name,
            style_structs=",\n\n".join(
                [self.styles[s].c_repr() for s in self.styles]
                    + ["\t{NULL}"]))
