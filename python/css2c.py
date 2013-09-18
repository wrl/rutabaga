#!/usr/bin/env python

from __future__ import print_function

import os
import sys
import re
from collections import OrderedDict

from tinycss.parsing import ParseError, remove_whitespace
from tinycss_rutabaga import NamespaceRule, FontFaceRule, RutabagaCSSParser

all = [
    "ColorParseException",

    "RutabagaStyle",
    "RutabagaStylesheet"
]

sanitize_cvar = re.compile(r"[^A-Za-z0-9_]+")

class RutabagaStyleProperty(object):
    def __init__(self, stylesheet, name, tokens):
        raise NotImplementedError

    def c_repr(self):
        raise NotImplementedError

class RutabagaAsset(object):
    pass

class RutabagaExternalAsset(object):
    def __init__(self, path):
        self.path = path

    def __repr__(self):
        return '<{0.__class__.__name__} for {1}>'\
                .format(self, self.path)

class RutabagaEmbeddedAsset(object):
    def __init__(self, path, variable_name):
        self.path = path
        self.variable_name = variable_name
        self.header_path = None

    def __repr__(self):
        if self.header_path:
            return '<{0.__class__.__name__} embedding {1} as {2} (in {3})>'\
                    .format(self, self.path, self.variable_name,
                            self.header_path)
        else:
            return '<{0.__class__.__name__} embedding {1} as {2} (no header)>'\
                    .format(self, self.path, self.variable_name)

####
# RutabagaTextureProperty
####

class RutabagaTexture(object):
    def __init__(self, stylesheet, path):
        self.path = path
        self.stylesheet = stylesheet

class RutabagaExternalTexture(RutabagaTexture):
    def __init__(self, stylesheet, path):
        super(RutabagaExternalTexture, self).__init__(stylesheet, path)

        self.asset = RutabagaExternalAsset(path)
        self.stylesheet.external_assets.append(self.asset)

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_TEXTURE,

\t\t\t\t\t.texture = {{
\t\t\t\t\t\t.location = RTB_ASSET_EXTERNAL,
\t\t\t\t\t\t.compression = RTB_ASSET_UNCOMPRESSED,

\t\t\t\t\t\t.external.path = "{0}"}}"""

    def c_repr(self):
        return self.c_repr_tpl.format(self.asset.path)

class RutabagaEmbeddedTexture(RutabagaTexture):
    def __init__(self, stylesheet, path):
        super(RutabagaEmbeddedTexture, self).__init__(stylesheet, path)

        self.texture_var = sanitize_cvar.sub("_", path).upper()
        self.stylesheet.embedded_assets.append(
            RutabagaEmbeddedAsset(path, self.texture_var))

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_TEXTURE,

\t\t\t\t\t.texture = {{
\t\t\t\t\t\t.location = RTB_ASSET_EMBEDDED,
\t\t\t\t\t\t.compression = RTB_ASSET_UNCOMPRESSED,

\t\t\t\t\t\t.embedded.size = {0}_SIZE,
\t\t\t\t\t\t.embedded.base = {0}}}"""

    def c_repr(self):
        return self.c_repr_tpl.format(self.texture_var)

class RutabagaTextureProperty(RutabagaStyleProperty):
    default_method = RutabagaEmbeddedTexture
    method_map = {
        "embed": RutabagaEmbeddedTexture,
        "extern": RutabagaExternalTexture}

    def __init__(self, stylesheet, name, tokens):
        self.path = None
        self.method = RutabagaTextureProperty.default_method

        tok = tokens[0]

        if tok.type == "URI":
            self.path = tok.value

        elif tok.type == "FUNCTION":
            method_map = RutabagaTextureProperty.method_map
            self.path = tok.content[0].value

            try:
                self.method = method_map[tok.function_name]
            except KeyError:
                raise ParseError(tok,
                    "valid texture reference functions are {0}, and url()"\
                        .format(", ".join(
                            ["{0}()".format(f) for f in method_map])))

        self.texture = self.method(stylesheet, self.path)

    def c_repr(self):
        return self.texture.c_repr()

####
# RutabagaRGBAProperty
####

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

####
# RutabagaStyle
####

state_mapping = {
    "normal": "RTB_DRAW_NORMAL",
    "focus":  "RTB_DRAW_FOCUS",
    "hover":  "RTB_DRAW_HOVER",
    "active": "RTB_DRAW_ACTIVE"
}

available_states = {
    "normal": "RTB_STYLE_NORMAL",
    "focus":  "RTB_STYLE_FOCUS",
    "hover":  "RTB_STYLE_HOVER",
    "active": "RTB_STYLE_ACTIVE"
}

prop_mapping = {
    "color": RutabagaRGBAProperty,
    "background-color": RutabagaRGBAProperty,
    "background-image": RutabagaTextureProperty
}

class RutabagaStyle(object):
    def __init__(self, stylesheet, type, normal_props):
        self.stylesheet = stylesheet

        self.type = type
        self.states = {}

        self.add_state("normal", normal_props)

    def __repr__(self):
        return ('<{0.__class__.__name__} for {0.type}>'.format(self))

    def add_state(self, state, styles):
        self.states[state] = OrderedDict()

        for s in styles:
            try:
                self.states[state][s] = \
                    prop_mapping[s](self.stylesheet, s, styles[s])
            except KeyError:
                raise ParseError(styles[s][0],
                        'unknown property "{0}"'.format(s))

    c_state_repr = """\
\t\t\t[{state}] = (struct rtb_style_property_definition []) {{
{styles},

\t\t\t\t{{NULL}}
\t\t\t}}"""

    c_style_repr = """\
\t{{"{type}",
\t\t{styles},

\t\t.properties = {{
{states}
\t\t}}
\t}}"""

    c_prop_repr = """\
\t\t\t\t{{"{0}",
{1}}}"""

    def c_repr(self):
        states = [self.c_state_repr.format(
            state=state_mapping[state],
            styles=",\n\n".join(
                [self.c_prop_repr.format(
                    prop_name,
                    self.states[state][prop_name].c_repr())
                for prop_name in self.states[state]]))
            for state in self.states]

        return self.c_style_repr.format(
            type=self.type,
            styles=' | '.join([available_states[s] for s in self.states]),
            states=",\n".join(states))


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

        if autoparse:
            self.parse()

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
                pass

            else:
                sel = self.parse_selector(rule.selector)
                decls = decl_dict(rule.declarations)

                for s in sel:
                    if s.find(":") > 0:
                        s, state = s.split(":", 1)
                        if state not in state_mapping.keys():
                            raise ParseError(rule, 'unknown state "{0}"'.format(state))

                        self.styles[s].add_state(state, decls)
                    else:
                        self.styles[s] = RutabagaStyle(self, s, decls)

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

    c_repr_tpl = """\
{includes}

static struct rtb_style {var_name}[] = {{
{style_structs}
}};"""

    def c_repr(self, var_name="default_style"):
        return self.c_repr_tpl.format(
            includes="\n".join(
                [self.c_include_tpl.format(header=a.header_path)
                    for a in self.embedded_assets]),
            var_name=var_name,
            style_structs=",\n\n".join(
                [self.styles[s].c_repr() for s in self.styles]
                    + ["\t{NULL}"]))


def main():
    css = RutabagaStylesheet(sys.stdin)
    css.parse()

    print(prelude)
    print(css.c_repr(), file=sys.stdout)

if __name__ == "__main__":
    main()
